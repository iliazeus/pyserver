#include "daemon_cli.h"

#include <cstdlib>
#include <iostream>
#include <system_error>

int main(int argc, char **argv, char **environ)
{
    const char *socketPath = std::getenv("DAEMON_CLI_SOCKET");
    if (socketPath == nullptr) socketPath = "./.cli.sock";

    int exitcode;
    auto ec = daemon_cli::RunClient(socketPath, argc, argv, environ, &exitcode);
    if (ec) { std::cerr << ec.message() << std::endl; return -1; }
    return exitcode;
}
