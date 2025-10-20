#include "daemon_cli.h"

#include <cstdlib>
#include <future>
#include <iostream>
#include <vector>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    const char *socketPath = std::getenv("DAEMON_CLI_SOCKET");
    if (socketPath == nullptr) socketPath = "./.cli.sock";
    daemon_cli::CDeferGuard deferUnlinkSocketPath = [&] { ::unlink(socketPath); };

    static std::promise<void> onStop;
    auto sighandler = +[](int) { onStop.set_value(); };
    ::signal(SIGINT, sighandler);

    auto ec = daemon_cli::ServerListen(socketPath,
                                       onStop.get_future(),
                                       [] (auto fds, auto argv, auto env)
    {
        int ret;

        ::pid_t pid = ::fork();
        if (pid < 0) std::exit(1);

        if (pid > 0)
        {
            for (int i = 0; i < 3; i++) ::close(fds[i]);

            int status;
            ::waitpid(pid, &status, /* options */ 0);
            return WEXITSTATUS(status);
        }

        for (int i = 0; i < 3; i++)
        {
            ret = ::dup2(fds[i], i);
            if (ret < 0) std::exit(-1);
            ::close(fds[i]);
        }

        std::vector<char *> argvp;
        argvp.reserve(argv.size() + 1);
        for (auto &s : argv) argvp.push_back(s.data());
        argvp.push_back(nullptr);

        std::vector<char *> envp;
        envp.reserve(env.size() + 1);
        for (auto &s : env) envp.push_back(s.data());
        envp.push_back(nullptr);

        ret = ::execvpe("cat", argvp.data(), envp.data());
        std::exit(-1);
    });
    if (ec)
    {
        std::cerr << ec.message() << std::endl;
        return 1;
    }

    return 0;
}
