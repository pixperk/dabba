#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <sys/wait.h>

int main(int argc, char** argv) {
   if (argc < 3 || std ::string(argv[1]) != "run") {
      std::cerr << "usage: " << argv[0] << " run <command> [args...]" << std::endl;
      return 1;
   }

   //everything after "run" is the command to execute
   std::vector<std::string> cmd(argv + 2, argv + argc);

   pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }

    if (pid == 0){
        //child
        sethostname("dabba", 5);

        std::vector<char*>cargv;
        //execvp expects a null-terminated array of char*, so we need to convert the vector of strings to that format
        for (auto &s : cmd) cargv.push_back(s.data());
        cargv.push_back(nullptr);

        execvp(cargv[0], cargv.data());
        perror("execvp"); //reaches if execvp fails
        _exit(1);
    }
    waitpid(pid, nullptr, 0); //wait for child to finish
    return 0;
}