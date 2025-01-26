#include "TCPServer.hpp"
#include "Buffer.hpp"

TCPServer::TCPServer(int port)
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        throw std::runtime_error("Socket creation failed.");
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        close(server_fd);
        throw std::runtime_error("setsockopt failed: ");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    address_len = sizeof(address);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        throw std::runtime_error("Bind failed.");
    }

    if (listen(server_fd, 3) < 0)
    {
        throw std::runtime_error("Listen failed.");
    }
    DEBUG("Server: Listening on port " + std::to_string(port));
}

TCPServer::~TCPServer()
{
    DEBUG("Server: Closing connection.");
    if (client_fd >= 0)
    {
        close(client_fd);
    }
    close(server_fd);
}
void TCPServer::sendResponse(KafkaResponse response)
{
    int bytes_sent = send(client_fd, &response, sizeof(response), 0);
}

void TCPServer::run()
{
    DEBUG("Starting server...");
    while (1)
    {
        try
        {
            int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&address_len);
            if (client_fd < 0)
            {
                throw std::runtime_error("Accept failed.");
            }

            DEBUG("Server: Client connected.");

            // Create a new thread to handle this specific connection
            std::thread th(&TCPServer::processRequest, this, client_fd);
            th.detach(); // Allow the thread to run independently
        }
        catch (const std::exception &e)
        {
            std::cerr << "Connection error: " << e.what() << std::endl;
        }
    }
}

void TCPServer::acceptConnection()
{
    client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&address_len);
    if (client_fd < 0)
    {
        throw std::runtime_error("Accept failed.");
    }
    DEBUG("Server: Client connected.");
}

void TCPServer::processRequest(int client_fd)
{
    while (1)
    {
        /* code */

        Buffer buffer(MAX_BUFFER_SIZE);
        int bytes_read = recv(client_fd, buffer.dataPtr(), buffer.size(), 0);
        std::cout << "Received " << bytes_read << " bytes: " << buffer.dataPtr() << std::endl;

        if (bytes_read <= 0)
        {
            std::cerr << "Error reading from socket" << std::endl;
            break;
        }

        int16_t api_key = buffer.extractApiKey(4);
        int16_t api_version = buffer.extractApiVersion(6);
        int32_t correlation_id = buffer.extractCorrelationId(8);

        DEBUG("Request: API Key: " + std::to_string(api_key) + ", API Version: " + std::to_string(api_version) + ", Correlation ID: " + std::to_string(correlation_id));

        KafkaResponse response;
        response.message_size = bytes_read;
        response.correlation_id = correlation_id;
        DEBUG("Correlation ID: " + std::to_string(response.correlation_id));

        if (api_key == 18)
        {
            if (api_version < 0 || api_version > 4)
            {
                response.appendErrorCode(ErrorCode::UNSUPPORTED_VERSION);
            }
            else
            {
                response.appendErrorCode(ErrorCode::NONE);
            }
        }

        // TODO: Clean up this mess
        int16_t min_version = 0; // Minimum supported version
        int16_t max_version = 4; // Maximum supported version

        int16_t num_entries = 3; // Number of API key entries
        response.appendVarInt(num_entries);
        response.appendApiVersion(APIVersions::API_KEY_API_VERSIONS, min_version, max_version);
        response.appendApiVersion(APIVersions::API_KEY_DESCRIBE_TOPIC_PARTITIONS, min_version, min_version);

        int32_t throttle_time_ms = 0;
        response.appendToBody(throttle_time_ms);
        response.appendToBody((int8_t)0);

        response.message_size = sizeof(response.correlation_id) + response.body.size();

        auto serialized_response = response.serialize();
        send(client_fd, serialized_response.data(), serialized_response.size(), 0);
    }
    close(client_fd);
}
void TCPServer::closeServer()
{
    close(server_fd);
    DEBUG("Server: Server closed.");
}