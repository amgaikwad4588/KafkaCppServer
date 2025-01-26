// debug.h
#pragma once

#include <iostream>
#define DEBUG_MODE // Ensure this is defined for debug builds

#ifdef DEBUG_MODE
#define DEBUG(message)                                   \
    do                                                   \
    {                                                    \
        std::cout << "[DEBUG] " << message << std::endl; \
    } while (0)
#else
#define DEBUG(message) \
    do                 \
    {                  \
    } while (0)
#endif
