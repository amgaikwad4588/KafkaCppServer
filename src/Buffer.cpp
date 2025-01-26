#include "Buffer.hpp"
#include "debug.hpp"
#define CORRELATION_ID_OFFSET 8
#define MESSAGE_SIZE_OFFSET 0
#define API_KEY_OFFSET 4
#define API_VERSION_OFFSET 6
template <typename T>
T Buffer::extractIntFromBuffer(size_t offset)
{
    static_assert(std::is_integral<T>::value, "T must be an integral type.");
    static_assert(sizeof(T) <= 8, "T size must be 8 bytes or less.");

    if (offset + sizeof(T) > buffer.size())
    {
        throw std::runtime_error("Buffer is too small for the requested integer.");
    }

    T value = 0;
    std::memcpy(&value, buffer.data() + offset, sizeof(T));
    // Handle endian conversion if needed (assumes network byte order is big-endian)
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

    return value;
}

int32_t Buffer::extractCorrelationId(size_t offset)
{
    int32_t correlation_id = 0;
    try
    {
        correlation_id = extractIntFromBuffer<int32_t>(offset);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return correlation_id;
}

int16_t Buffer::extractApiVersion(size_t offset)
{
    int32_t api_version = 0;
    try
    {
        api_version = extractIntFromBuffer<int16_t>(offset);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return api_version;
}

int16_t Buffer::extractApiKey(size_t offset)
{
    int32_t api_version = 0;
    try
    {
        api_version = extractIntFromBuffer<int16_t>(offset);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return api_version;
}