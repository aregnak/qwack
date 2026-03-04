#pragma once

#include <iostream>

// Debug prints

// If build using MSVC debug config
#ifdef _DEBUG

// LCU specific messages
#define LCU_LOG(x) std::cerr << "[LCU] " << x << "\n"

// Qwack specific messages
#define QWACK_LOG(x) std::cerr << "[Qwack] " << x << "\n"

#else

// When built in release, to not have unnecessary print instructions we
// define our LOG macros to do nothing.
#define LCU_LOG(x)                                                                                 \
    do                                                                                             \
    {                                                                                              \
    } while (0)

#define QWACK_LOG(x)                                                                               \
    do                                                                                             \
    {                                                                                              \
    } while (0)

#endif // _DEBUG
