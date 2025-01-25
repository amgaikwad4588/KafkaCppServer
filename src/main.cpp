#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

void handle_client(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read;

    while ((bytes_read = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        if (bytes_read < 12) {
            std::cerr << "Invalid request received (too short)." << std::endl;
            break;
        }

        // Extract correlation_id from the request (at offset 8)
        int32_t raw_corr_id;
        memcpy(&raw_corr_id, buffer + 8, sizeof(raw_corr_id));
        int32_t correlation_id = ntohl(raw_corr_id); // Convert from network to host order

        // Extract the request_api_version (for debug/error check)
        int16_t request_api_version;
        memcpy(&request_api_version, buffer + 6, sizeof(request_api_version));
        request_api_version = ntohs(request_api_version);

        int16_t error_code = 0;
        if (request_api_version < 0 || request_api_version > 4) {
            error_code = 35; // Some error code if needed
        }

        // Flexible version response
        int16_t be_error_code = htons(error_code);
        uint8_t api_keys_length = 0x02; // Compact array length for 1 element
        int16_t be_api_key = htons(18);
        int16_t be_min_version = htons(0);
        int16_t be_max_version = htons(4);
        int32_t be_throttle_time_ms = htonl(0);
        uint8_t no_tags = 0x00; // No tagged fields
        uint8_t api_key_tags = 0x00; // No tags for this ApiKey entry

        // Calculate message_size: correlation_id(4) + error_code(2) + api_keys_length(1)
        // + (api_key+min_version+max_version=6 bytes) + throttle_time_ms(4) + no_tags(1)
        int32_t message_size = htonl(19);
        int32_t be_correlation_id = htonl(correlation_id);

        // Send response
        send(client_fd, &message_size, sizeof(message_size), 0);
        send(client_fd, &be_correlation_id, sizeof(be_correlation_id), 0);
        send(client_fd, &be_error_code, sizeof(be_error_code), 0);
        send(client_fd, &api_keys_length, sizeof(api_keys_length), 0);
        send(client_fd, &be_api_key, sizeof(be_api_key), 0);
        send(client_fd, &be_min_version, sizeof(be_min_version), 0);
        send(client_fd, &be_max_version, sizeof(be_max_version), 0);
        send(client_fd, &api_key_tags, sizeof(api_key_tags), 0);
        send(client_fd, &be_throttle_time_ms, sizeof(be_throttle_time_ms), 0);
        send(client_fd, &no_tags, sizeof(no_tags), 0);
        std::cout << "Response sent to client with correlation_id: " << correlation_id << std::endl;
    }

    if (bytes_read == 0) {
        std::cout << "Client disconnected." << std::endl;
    } else if (bytes_read < 0) {
        std::cerr << "Failed to read request or client disconnected." << std::endl;
    }

    close(client_fd);
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket." << std::endl;
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        std::cerr << "setsockopt failed." << std::endl;
        return 1;
    }

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9092);

    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        close(server_fd);
        std::cerr << "Failed to bind to port 9092" << std::endl;
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        close(server_fd);
        std::cerr << "Listen failed" << std::endl;
        return 1;
    }

    std::cout << "Server is running on port 9092, waiting for clients..." << std::endl;

    while (true) {
        struct sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
        if (client_fd < 0) {
            std::cerr << "Failed to accept a client connection." << std::endl;
            continue;
        }

        std::cout << "Client connected." << std::endl;
        std::thread(handle_client, client_fd).detach(); // Handle each client in a new thread
    }

    close(server_fd);
    return 0;
}
