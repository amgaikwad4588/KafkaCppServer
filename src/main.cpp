#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
int main(int argc, char* argv[]) {
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
        std::cerr << "listen failed" << std::endl;
        return 1;
    }
    std::cout << "Waiting for a client to connect...\n";
    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
    if (client_fd < 0) {
        std::cerr << "Failed to accept a client connection." << std::endl;
        close(server_fd);
        return 1;
    }
    std::cout << "Client connected\n";
    char buffer[1024];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) {
        std::cerr << "Failed to read request or client disconnected" << std::endl;
        close(client_fd);
        close(server_fd);
        return 1;
    }
    // Extract correlation_id from the request (at offset 8)
    int32_t raw_corr_id;
    memcpy(&raw_corr_id, buffer+8, sizeof(raw_corr_id));
    int32_t correlation_id = ntohl(raw_corr_id); // Convert from network to host order
    // Extract the request_api_version (for debug/error check)
    int16_t request_api_version;
    memcpy(&request_api_version, buffer+6, sizeof(request_api_version));
    request_api_version = ntohs(request_api_version);
    int16_t error_code = 0;
    if (request_api_version < 0 || request_api_version > 4) {
        // some error code if needed
        error_code = 35; // but must also be in network order later
    }
    // We must respond with ApiVersionsResponse v3 (flexible), same structure for v4.
    // Flexible version: fields:
    // error_code (INT16)
    // api_keys (COMPACT_ARRAY)
    //   To encode a compact array of 1 element: length = element_count + 1 = 2, so write 0x02 as the length.
    //   For that single element: api_key=18 (0x12), min_version=0, max_version=4
    // throttle_time_ms (INT32)
    // tagged_fields (0x00 for no tags)
    // Let's choose just 1 ApiKey entry to simplify:
    int16_t be_error_code = htons(error_code);
    uint8_t api_keys_length = 0x02; // compact array length for 1 element
    int16_t be_api_key = htons(18);
    int16_t be_min_version = htons(0);
    int16_t be_max_version = htons(4);
    int32_t be_throttle_time_ms = htonl(0);
    uint8_t no_tags = 0x00; // no tagged fields
    uint8_t api_key_tags = 0x00; // no tags for this ApiKey entry
    // Calculate message_size: correlation_id(4) + error_code(2) + api_keys_length(1)
    // + (api_key+min_version+max_version=6 bytes) + throttle_time_ms(4) + no_tags(1)
    // = 4 + 2 + 1 + 6 + 4 + 1 = 18 bytes total after the length field
    int32_t message_size = htonl(19);
    // Send response:
    // Note: correlation_id must be sent back in network order
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
    std::cout << "Response sent\n" << std::endl;
    close(client_fd);
    close(server_fd);
    return 0;
}