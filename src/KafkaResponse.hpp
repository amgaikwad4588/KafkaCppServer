#include <cstdint>
#include <vector>
#include <cstring>
#include <type_traits>
#include <iostream>
#include <arpa/inet.h>
#include "debug.hpp"
#include "APIVersions.hpp"
struct KafkaResponse
{
    int32_t message_size{};      // Total size of the response (excluding this field).
    int32_t correlation_id{};    // Matches the request's correlation ID.
    std::vector<std::byte> body; // Response body (e.g., error code, API keys).

    std::vector<std::byte> serialize() const
    {
        size_t total_size = sizeof(message_size) + sizeof(correlation_id) + body.size();

        std::vector<std::byte> buffer(total_size);
        size_t offset = 0;

        int32_t net_message_size = htonl(message_size);
        std::memcpy(buffer.data() + offset, &net_message_size, sizeof(net_message_size));
        offset += sizeof(net_message_size);

        int32_t net_correlation_id = htonl(correlation_id);
        std::memcpy(buffer.data() + offset, &net_correlation_id, sizeof(net_correlation_id));
        offset += sizeof(net_correlation_id);

        std::memcpy(buffer.data() + offset, body.data(), body.size());

        return buffer;
    }

    template <typename T>
    void appendToBody(T value)
    {
        static_assert(std::is_integral<T>::value || std::is_enum<T>::value, "T must be an integral or enum type.");
        if constexpr (sizeof(T) == 2)
        {
            value = ntohs(value);
        }
        else if constexpr (sizeof(T) == 4)
        {
            value = ntohl(value);
        }
        else if constexpr (sizeof(T) == 8)
        {
            value = be64toh(value); // Big-endian to host for 64-bit integers
        }

        auto bytes = reinterpret_cast<std::byte *>(&value);
        body.insert(body.end(), bytes, bytes + sizeof(T));
    }
    void appendVarInt(int32_t value)
    {

        if (value < 0)
        {
            throw std::runtime_error("Cannot write negative varint");
        }

        while (value >= 0x80)
        {
            body.emplace_back(static_cast<std::byte>((value & 0x7F) | 0x80));
            value >>= 7;
        }

        body.emplace_back(static_cast<std::byte>(value));
    }

    void appendApiVersion(APIVersions apiVersion, int16_t minApiVersion, int16_t maxApiVersion)
    {
        appendToBody((int16_t)apiVersion);
        appendToBody(minApiVersion);
        appendToBody(maxApiVersion);
        appendToBody((int8_t)0);
    }
    void appendRawToBody(const std::vector<std::byte> &data)
    {
        body.insert(body.end(), data.begin(), data.end());
    }

    void appendErrorCode(ErrorCode error)
    {
        int16_t errorCode = static_cast<std::underlying_type_t<ErrorCode>>(error);
        appendToBody(errorCode);
    }
};