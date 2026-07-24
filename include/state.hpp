#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

// what we know about a running container, read back from /run/dabba/<id>
struct ContainerInfo
{
    std::string id;
    int pid = 0;
    std::string cmd;
};

//writes /run/dabba/<id> file with container info
class StateFile
{
    fs::path path_;

public:
    StateFile(const std::string &id, int pid, const std::string &cmd)
     : path_(fs::path("/run/dabba") / id)
     {
        fs::create_directories("/run/dabba");
        std::ofstream f(path_);
        f << "pid=" <<pid << "\n";
        f << "cmd=" << cmd << "\n";
        if (!f)
            throw std::runtime_error("could not write state file " + path_.string());
     }

     StateFile(const StateFile &) = delete;
    StateFile &operator=(const StateFile &) = delete;

    ~StateFile()
    {
        std::error_code ec;
        fs::remove(path_, ec);   // never throw from a dtor
    }
};

inline ContainerInfo read_state(const fs::path &p)
{
    ContainerInfo info;
    info.id = p.filename();
    std::ifstream f(p);
    std::string line;
    while (std::getline(f, line))
    {
        auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key == "pid")      info.pid = std::stoi(val);
        else if (key == "cmd") info.cmd = val;
    }
    return info;
}

//reads state files from /run/dabba and returns a vector of ContainerInfo
inline std::vector<ContainerInfo> list_states()
{
    std::vector<ContainerInfo> out;
    std::error_code ec;
    for (const auto &entry : fs::directory_iterator("/run/dabba", ec))
        out.push_back(read_state(entry.path()));
    return out;
}