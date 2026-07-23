#include <cstring>
#include <iostream>
#include <sched.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "util.hpp"
#include "child_stack.hpp"
#include <filesystem>
#include "fs_setup.hpp"

constexpr const char *kRootfs = "/var/lib/dabba/rootfs";

struct ChildArgs
{
    std::filesystem::path rootfs;
    std::vector<std::string> cmd;
};

constexpr const char *kHostname = "dabba";

// this runs inside the child inside whatever namespace clone gave it
static int child_fn(void *arg)
try
{
      auto &args = *static_cast<ChildArgs *>(arg);

    checked(sethostname(kHostname, std::strlen(kHostname)), "sethostname");
    // setup the root filesystem for the child process
    setup_rootfs(args.rootfs);

    std::vector<char *> cargv;
    for (auto &s : args.cmd)
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

    // we create a stack for the child process to use. 
    ChildStack stack;

    // SIGCHLD is not a namespace, it is the signal sent to us when the child dies
       int flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS
              | CLONE_NEWNET | CLONE_NEWIPC | SIGCHLD;

    ChildArgs args{kRootfs, cmd};
    pid_t pid = checked(clone(child_fn, stack.top(), flags, &args), "clone");

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
