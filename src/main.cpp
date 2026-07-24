#include <csignal>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sched.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <variant>
#include <vector>

#include "cgroup.hpp"
#include "child_stack.hpp"
#include "cli.hpp"
#include "fs_setup.hpp"
#include "id.hpp"
#include "net_setup.hpp"
#include "state.hpp"
#include "subprocess.hpp"
#include "util.hpp"

struct ChildArgs
{
    std::filesystem::path rootfs;
    std::vector<std::string> cmd;
    int start_rd;
    int start_wr;
};

constexpr const char *kHostname = "dabba";

static int child_fn(void *arg)
try
{
    auto &args = *static_cast<ChildArgs *>(arg);

    close(args.start_wr);
    char c;
    read(args.start_rd, &c, 1);
    close(args.start_rd);

    checked(sethostname(kHostname, std::strlen(kHostname)), "sethostname");
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

static int do_run(const RunCmd &rc)
{
    uint16_t id = make_id();
    std::string idh = id_hex(id);

    Cgroup cg("dabba-" + idh);
    ChildStack stack;

    int flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS
              | CLONE_NEWNET | CLONE_NEWIPC | SIGCHLD;

    int pipefd[2];
    checked(pipe(pipefd), "pipe");

    ChildArgs args{rc.rootfs, rc.argv, pipefd[0], pipefd[1]};
    pid_t pid = checked(clone(child_fn, stack.top(), flags, &args), "clone");

    close(pipefd[0]);

    apply_limits(cg, rc.limits, pid);

    std::optional<Network> net;
    if (rc.network)
        net.emplace(pid, id);

    // record this container so ps/exec can find it. removed on scope exit.
    StateFile state(idh, pid, rc.argv.empty() ? "" : rc.argv[0]);

    close(pipefd[1]);

    int status = 0;
    checked(waitpid(pid, &status, 0), "waitpid");

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return 1;
}

static int do_ps(const PsCmd &)
{
    std::printf("%-8s %-8s %s\n", "ID", "PID", "COMMAND");
    for (const auto &s : list_states())
        std::printf("%-8s %-8d %s\n", s.id.c_str(), s.pid, s.cmd.c_str());
    return 0;
}

static int do_exec(const ExecCmd &e)
{
    ContainerInfo info = read_state(std::filesystem::path("/run/dabba") / e.id);
    if (info.pid == 0)
        throw std::runtime_error("no such container: " + e.id);

    // enter all of the container's namespaces, then run the command inside
    std::vector<std::string> cmd = {"nsenter", "-t", std::to_string(info.pid), "-a"};
    for (const auto &a : e.argv)
        cmd.push_back(a);

    run_cmd(cmd);
    return 0;
}

static int do_kill(const KillCmd &k)
{
    ContainerInfo info = read_state(std::filesystem::path("/run/dabba") / k.id);
    if (info.pid == 0)
        throw std::runtime_error("no such container: " + k.id);

    // SIGKILL the container's init. killing pid 1 of the namespace tears down
    // the whole namespace, so every process inside dies. SIGKILL because a
    // namespace init ignores signals it has no handler for, but never SIGKILL.
    checked(::kill(info.pid, SIGKILL), "kill");
    return 0;
}

int main(int argc, char **argv)
try
{
    Command cmd = parse_args(argc, argv);
    return std::visit([](auto &&c) -> int
    {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, RunCmd>)       return do_run(c);
        else if constexpr (std::is_same_v<T, PsCmd>)   return do_ps(c);
        else if constexpr (std::is_same_v<T, ExecCmd>) return do_exec(c);
        else if constexpr (std::is_same_v<T, KillCmd>) return do_kill(c);
    }, cmd);
}
catch (const std::exception &e)
{
    std::cerr << "dabba: " << e.what() << "\n";
    return 1;
}
