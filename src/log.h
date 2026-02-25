#pragma once

// Debug prints
// LCU specific messages
#define LCU_LOG(x) std::cerr << "[LCU] " << x << std::endl

// Qwack specific messages
#define QWACK_LOG(x) std::cout << "[Qwack] " << x << std::endl

// If you're reading this you know what this does.
#define NEWLINE std::cout << "\n";