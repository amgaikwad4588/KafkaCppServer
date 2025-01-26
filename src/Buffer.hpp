#pragma once
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <iostream>
#include <arpa/inet.h>

class Buffer
{
private:
    std::vector<std::byte> buffer;
    template <typename T>
    T extractIntFromBuffer(size_t offset);

public:
    explicit Buffer(size_t size) : buffer(size) {}

    std::byte *dataPtr() { return buffer.data(); }
    size_t size() const { return buffer.size(); }

    int32_t extractCorrelationId(size_t offset);
    int16_t extractApiVersion(size_t offset);
    int16_t extractApiKey(size_t offset);
};