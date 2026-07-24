#pragma once

#include "limits.hpp"
#include <string>
#include <vector>

struct RunCmd{
    std::string rootfs = "/var/lib/dabba/rootfs";
    Limits limits;
    bool network = false;
    std::vector<std::string> argv;//command and its args

};

RunCmd parse_run_cmd(int argc, char **argv);