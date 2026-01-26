#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <string>
#include <iostream>

#include "parser.h"
#include "httplib.h"
#include "json.hpp"

#define LCU_LOG(x) std::cout << "[LCU] " << x << std::endl

class poll
{
public:
    poll();

    bool update();
    std::string getPlayerName();
    float getcs(const std::string& playerName);

private:
    httplib::SSLClient cli;
    httplib::Result res;
};

// This function is needed for Lockfile parsing. Not yet needed
//
//
// inline std::string base64(const std::string& in)
// {
//     static const char b64_table[] =
//         "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//     std::string out;
//     int val = 0, valb = -6;
//     for (uint8_t c : in)
//     {
//         val = (val << 8) + c;
//         valb += 8;
//         while (valb >= 0)
//         {
//             out.push_back(b64_table[(val >> valb) & 0x3F]);
//             valb -= 6;
//         }
//     }
//     if (valb > -6)
//         out.push_back(b64_table[((val << 8) >> (valb + 8)) & 0x3F]);
//     while (out.size() % 4)
//         out.push_back('=');
//     return out;
// }
