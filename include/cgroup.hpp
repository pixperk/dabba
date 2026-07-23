#pragma once
#include "limits.hpp"

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

inline void apply_limits(const Cgroup &cg, const Limits &lim, int child_pid){
     if (lim.memory_bytes)
        cg.write("memory.max", std::to_string(lim.memory_bytes));

    if (lim.cpu_percent)
    {
        // quota within a 100000us period. 50% -> "50000 100000"
        int quota = lim.cpu_percent * 1000;
        cg.write("cpu.max", std::to_string(quota) + " 100000");
    }

    if (lim.max_pids)
        cg.write("pids.max", std::to_string(lim.max_pids));

    cg.add_process(child_pid);
}