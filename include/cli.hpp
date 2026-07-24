#pragma once

#include "limits.hpp"
#include <string>
#include <variant>
#include <vector>

struct RunCmd{
    std::string rootfs = "/var/lib/dabba/rootfs";
    Limits limits;
    bool network = false;
    std::vector<std::string> argv;//command and its args

};

struct PsCmd{};

struct ExecCmd{
    std::string id;
    std::vector<std::string> argv;
};

//command can be one of run, ps, exec
using Command = std::variant<RunCmd, PsCmd, ExecCmd>;

Command parse_args(int argc, char **argv);