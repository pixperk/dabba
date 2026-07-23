#include <cstring>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "util.hpp"

constexpr const char* kHostname = "dabba";

int main(int argc, char** argv) try{
    if (argc < 3 || std::string(argv[1]) != "run") {
        std::cerr << "usage: " << argv[0] << " run <command> [args...]\n";
        return 1;
    }

    // everything after "run" is the command to execute
    std::vector<std::string> cmd(argv + 2, argv + argc);

    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }

    if (pid == 0) {
        // throws EPERM without sudo instead of failing silently
        checked(sethostname(kHostname, std::strlen(kHostname)), "sethostname");

        // execvp wants a null terminated char*[], so point into cmd's strings
        std::vector<char*> cargv;
        for (auto& s : cmd) cargv.push_back(s.data());
        cargv.push_back(nullptr);

        execvp(cargv[0], cargv.data());
        perror("execvp");  // only reached if exec failed
        _exit(1);
    }

int status = 0;
checked(waitpid(pid, &status, 0), "waitpid");

if (WIFEXITED(status))   return WEXITSTATUS(status);
if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
return 1;
}catch(const std::exception &e){
    std::cerr<<"dabba: "<<e.what()<<std::endl; 
    return 1;
}
