#include "cli.hpp"
#include <stdexcept>
#include <string>

// "50M" / "512K" / "2G" / "1048576" -> bytes
static long long parse_size(const std::string &s)
{
    if (s.empty())
        throw std::runtime_error("empty size");

    char suffix = s.back();
    long long mult = 1;
    std::string num = s;

    if (suffix == 'K' || suffix == 'k') mult = 1024;
    else if (suffix == 'M' || suffix == 'm') mult = 1024LL * 1024;
    else if (suffix == 'G' || suffix == 'g') mult = 1024LL * 1024 * 1024;

    if (mult != 1)
        num = s.substr(0, s.size() - 1);

    return std::stoll(num) * mult;
}

// grab the value after a flag, erroring if it's missing
static const char *value_for(const std::string &flag, int &i, int argc, char **argv)
{
    if (++i >= argc)
        throw std::runtime_error(flag + " needs a value");
    return argv[i];
}

//static as this is only used in this file
static RunCmd parse_run_cmd(int argc, char **argv)
{
    // argv[0] = dabba, argv[1] = run, flags and command start at 2
    RunCmd r;
    int i = 2;

    for (; i < argc; i++)
    {
        std::string a = argv[i];
        if (a == "--memory")      r.limits.memory_bytes = parse_size(value_for(a, i, argc, argv));
        else if (a == "--cpu")    r.limits.cpu_percent  = std::stoi(value_for(a, i, argc, argv));
        else if (a == "--pids")   r.limits.max_pids     = std::stoi(value_for(a, i, argc, argv));
        else if (a == "--rootfs") r.rootfs              = value_for(a, i, argc, argv);
        else if (a == "--net")    r.network             = true;
        else break;   // first non-flag is the start of the command
    }

    r.argv.assign(argv + i, argv + argc);
    if (r.argv.empty())
        throw std::runtime_error("no command given");

    return r;
}

static ExecCmd parse_exec_cmd(int argc, char **argv){
    if (argc < 4)
        throw std::runtime_error("usage: dabba exec <id> <cmd> [args...]");
    ExecCmd e;
    e.id = argv[2];
    e.argv.assign(argv + 3, argv + argc);
    return e;
}

static KillCmd parse_kill_cmd(int argc, char **argv){
    if (argc < 3)
        throw std::runtime_error("usage: dabba kill <id>");
    KillCmd k;
    k.id = argv[2];
    return k;
}

Command parse_args(int argc, char **argv)
{
    if (argc < 2)
        throw std::runtime_error("usage: dabba <run|ps|exec> ...");
    std::string sub = argv[1];
    if (sub == "run")  return parse_run_cmd(argc, argv);
    if (sub == "ps")   return PsCmd{};
    if (sub == "exec") return parse_exec_cmd(argc, argv);
    if (sub == "kill") return parse_kill_cmd(argc, argv);
    throw std::runtime_error("unknown command: " + sub);
}