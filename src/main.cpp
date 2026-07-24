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
#include "cgroup.hpp"

constexpr const char *kRootfs = "/var/lib/dabba/rootfs";

struct ChildArgs
{
    std::filesystem::path rootfs;
    std::vector<std::string> cmd;
    int start_rd; //child blocks until parent applies limits
    int start_wr; //child has to close its inherited write end of the pipe so parent can detect EOF
};

constexpr const char *kHostname = "dabba";

// this runs inside the child inside whatever namespace clone gave it
static int child_fn(void *arg)
try
{
      auto &args = *static_cast<ChildArgs *>(arg);

      //wait unti the parent has put us in cgroup and applied limits
      close(args.start_wr); //close the write end of the pipe so parent can detect EOF
      char c;
      read(args.start_rd, &c, 1); //block until parent closes the write end of the pipe
      close(args.start_rd);

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

    // create the cgroup before clone so it outlives the child and rmdirs on scope exit
    Cgroup cg("dabba");
    Limits lim;
    lim.max_pids = 20;

    ChildStack stack;

    // SIGCHLD is not a namespace, it is the signal sent to us when the child dies
    int flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS
              | CLONE_NEWNET | CLONE_NEWIPC | SIGCHLD;

    int pipefd[2];
    checked(pipe(pipefd), "pipe");

    ChildArgs args{kRootfs, cmd, pipefd[0], pipefd[1]};
    pid_t pid = checked(clone(child_fn, stack.top(), flags, &args), "clone");

    close(pipefd[0]); // child has the read end, we only write

    // apply limits, THEN release the child. it is blocked on the pipe read
    // until this close, so it cannot fork before it is in the cgroup
    apply_limits(cg, lim, pid);
    close(pipefd[1]); // signal the child to continue

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
