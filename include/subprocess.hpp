#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include "util.hpp"

// if a command exits non-zero we throw an exception
// it is the basic fork setup
inline void run_cmd(const std::vector<std::string> &args)
{
    std::vector<char *> c;
    for (const auto &a : args)
        c.push_back(const_cast<char *>(a.c_str()));
    c.push_back(nullptr);

    pid_t pid = checked(fork(), "fork");
    if (pid == 0)
    {
        execvp(c[0], c.data());
        _exit(127); // only reached if execvp failed
    }

    int status = 0;
    checked(waitpid(pid, &status, 0), "waitpid");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        throw std::runtime_error("command failed: " + args[0]);
}
