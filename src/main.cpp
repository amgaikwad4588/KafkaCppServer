#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

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

    if (listen(server_fd, 5) != 0) {
        close(server_fd);
        std::cerr << "listen failed" << std::endl;
        return 1;
    }

    std::cout << "Server is running on port 9092...\n";

    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
    if (client_fd < 0) {
        std::cerr << "Failed to accept a client connection." << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Client connected\n";

    while (true) {
        char buffer[1024];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            std::cerr << "Failed to read request or client disconnected" << std::endl;
            break;
        }

        if (bytes_read < 12) { // Minimum request size check
            std::cerr << "Invalid request size" << std::endl;
            break;
        }

        int32_t raw_corr_id;
        memcpy(&raw_corr_id, buffer + 8, sizeof(raw_corr_id));
        int32_t correlation_id = ntohl(raw_corr_id);

        int16_t request_api_version;
        memcpy(&request_api_version, buffer + 6, sizeof(request_api_version));
        request_api_version = ntohs(request_api_version);

        int16_t error_code = 0;
        if (request_api_version < 0 || request_api_version > 4) {
            error_code = 35;
        }

        int16_t be_error_code = htons(error_code);
        uint8_t api_keys_length = 0x02;
        int16_t be_api_key = htons(18);
        int16_t be_min_version = htons(0);
        int16_t be_max_version = htons(4);
        int32_t be_throttle_time_ms = htonl(0);
        uint8_t no_tags = 0x00;
        uint8_t api_key_tags = 0x00;

        int32_t message_size = htonl(19);
        int32_t be_correlation_id = htonl(correlation_id);

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

        std::cout << "Response sent for correlation ID: " << correlation_id << "\n";
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
