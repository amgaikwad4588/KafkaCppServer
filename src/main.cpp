#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <chrono>
#include <thread>
#include "TCPServer.hpp"
#include "debug.hpp"

#define MAX_BUFFER_SIZE 1024
#define SERVER_PORT 9092

int main(int argc, char *argv[])
{
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    TCPServer server(SERVER_PORT);
    server.run();

    return 0;
}