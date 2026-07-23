#include <cstring>
#include <iostream>
#include <sched.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "util.hpp"

constexpr const char *kHostname = "dabba";

// this runs inside the child inside whatever namespace clone gave it
static int child_fn(void *arg)
try
{
    auto &cmd = *static_cast<std::vector<std::string> *>(arg);

    checked(sethostname(kHostname, std::strlen(kHostname)), "sethostname");

    std::vector<char *> cargv;
    for (auto &s : cmd)
        cargv.push_back(s.data());
    cargv.push_back(nullptr);

    execvp(cargv[0], cargv.data());
    perror("execvp");
    return 1;
}
catch (const std::exception &e)
{
    std::cerr << "dabba: " << e.what() << "\n";
    return 1;
}

int main(int argc, char **argv)
try
{
    if (argc < 3 || std::string(argv[1]) != "run")
    {
        std::cerr << "usage: " << argv[0] << " run <command> [args...]\n";
        return 1;
    }

    // everything after "run" is the command to execute
    std::vector<std::string> cmd(argv + 2, argv + argc);

    // stacks grow down, so clone gets the top. 16 byte aligned or aarch64 faults
    alignas(16) static char stack[1024 * 1024];

    // SIGCHLD is not a namespace, it is the signal sent to us when the child dies
    int flags = CLONE_NEWUTS | SIGCHLD;

    pid_t pid = checked(clone(child_fn, stack + sizeof(stack), flags, &cmd), "clone");

    int status = 0;
    checked(waitpid(pid, &status, 0), "waitpid");

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return 1;
}
catch (const std::exception &e)
{
    std::cerr << "dabba: " << e.what() << std::endl;
    return 1;
}
