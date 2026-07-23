#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

class Cgroup{
    fs::path path_;

    public:
     explicit Cgroup(const std::string &name)
      : path_(fs::path("/sys/fs/cgroup") / name)
      {
        fs::create_directories(path_);
      }

      //prevent copy as we do not want double rmdir
        Cgroup(const Cgroup &) = delete;
        Cgroup &operator=(const Cgroup &) = delete;

    void write(const std::string &file, const std::string& value)const {
        std::ofstream f(path_ / file);
        f << value;
        if (!f) {
            throw std::system_error(errno, std::generic_category(), "file");
        }
    }

        void add_process(int pid) const{
            write("cgroup.procs", std::to_string(pid));
        }

        ~Cgroup(){
            //we dont throw in destructor because it can be called during stack unwinding
            std::error_code ec;
            fs::remove(path_, ec);
        }
}