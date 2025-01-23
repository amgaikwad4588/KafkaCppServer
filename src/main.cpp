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
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket: " << std::endl;
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        std::cerr << "setsockopt failed: " << std::endl;
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

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!\n";

    // Uncomment this block to pass the first stage

    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);

    if (client_fd < 0) {
        std::cerr << "Failed to accept client connection" << std::endl;
        close(server_fd);
        return 1;
    }
    std::cout << "Client connected\n";
    
    // int32_t correlation_id = htonl(7); // Hardcoded correlation_id = 7
    // char response[8];
    // memcpy(response, &message_size, 4);
    // memcpy(response + 4, &correlation_id, 4);
    // // Send the response to the client
    // ssize_t bytes_sent = send(client_fd, response, sizeof(response), 0);
    // if (bytes_sent < 0) {
    //     std::cerr << "Failed to send response to client" << std::endl;
    // } else {
    //     std::cout << "Response sent to client\n";
    // }
    
    // Read request from the client
    char request_buffer[1024];
    ssize_t bytes_received = recv(client_fd, request_buffer, sizeof(request_buffer), 0);
    if (bytes_received < 0) {
        std::cerr << "Failed to read request from client" << std::endl;
        close(client_fd);
        close(server_fd);
        return 1;
    }
    // Extract correlation_id and request_api_version from the request
    // Extract request_api_version (offset for message_size + api_key)
	if (bytes_received >= 12) { // Ensure enough bytes for header v2
    	int16_t request_api_version;
    	memcpy(&request_api_version, request_buffer + 4 + 2, 2); // Offset for message_size (4) + api_key (2)
    	request_api_version = ntohs(request_api_version); // Convert to host byte order
    	int32_t correlation_id;
    	memcpy(&correlation_id, request_buffer + 4 + 2 + 2, 4); // Offset for message_size + api_key + api_version
    	correlation_id = ntohl(correlation_id); // Convert to host byte order
    	int16_t error_code = 0; // Default to no error
    	if (request_api_version > 4) {
        	error_code = 35; // UNSUPPORTED_VERSION
    	}
    	// Prepare response: message_size (4 bytes) + correlation_id (4 bytes) + error_code (2 bytes)
    	int32_t message_size = htonl(6); // 4 bytes for correlation_id + 2 bytes for error_code
    	int32_t response_correlation_id = htonl(correlation_id);
    	int16_t response_error_code = htons(error_code);
    	char response[10];
    	memcpy(response, &message_size, 4);
    	memcpy(response + 4, &response_correlation_id, 4);
    	memcpy(response + 8, &response_error_code, 2);
    	// Send the response to the client
    	ssize_t bytes_sent = send(client_fd, response, sizeof(response), 0);
    	if (bytes_sent < 0) {
        	std::cerr << "Failed to send response to client" << std::endl;
    	} else {
        	std::cout << "Response sent to client\n";
    	}
	} else {
    	std::cerr << "Invalid request received from client" << std::endl;
	}

    close(client_fd);
    close(server_fd);
    return 0;
}