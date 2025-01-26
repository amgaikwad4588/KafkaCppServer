#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <chrono>
#include <thread>
#include "ErrorCode.hpp"
#include "debug.hpp"
#include "KafkaResponse.hpp"
#include "APIVersions.hpp"

#define MAX_BUFFER_SIZE 1024
#define SERVER_PORT 9092

struct KafkaRequest
{
    int32_t api_key{};
    int32_t api_version{};
    int32_t correlation_id{};
    char *client_id{};

    std::string to_string()
    {
        return "API Key: " + std::to_string(api_key) + ", API Version: " + std::to_string(api_version) + ", Correlation ID: " + std::to_string(correlation_id) + ", Client ID: " + std::string(client_id);
    }
};
class TCPServer
{
public:
    TCPServer(int port);
    ~TCPServer();
    int setupSocket();
    void acceptConnection();
    void closeServer();
    void processRequest(int client_fd);
    void sendResponse(KafkaResponse response);
    std::vector<std::byte> receive();
    void run();

private:
    int server_fd;
    int client_fd;
    struct sockaddr_in address;
    int address_len;
};