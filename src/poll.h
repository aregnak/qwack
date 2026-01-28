#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <string>
#include <iostream>

#include "parser.h"
#include "httplib.h"
#include "json.hpp"

class poll
{
public:
    poll();

    bool update(); // Live Client Data API

    std::string getPlayerName(const LCUInfo&);

    float getGameTime();
    int getcs(const std::string& playerName);
    float getGold();

private:
    httplib::SSLClient cli;
    httplib::Result res;
};

inline std::string base64(const std::string& in)
{
    static const char b64_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (uint8_t c : in)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            out.push_back(b64_table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        out.push_back(b64_table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4)
        out.push_back('=');
    return out;
}
