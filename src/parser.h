// This file is not needed for now, I misunderstood the LCU API at first.

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#include "debugPrints.h"

struct LCUInfo
{
    int port = 0;
    std::string password;
};

inline LCUInfo parseLockfile()
{
    std::ifstream file("C:\\Riot Games\\League of Legends\\lockfile");

    if (!file.is_open())
    {
        //LCU_LOG("Failed to open lockfile");
        return { 0, "" };
    }
    std::string line;
    LCUInfo info{ 0, "" };

    if (std::getline(file, line))
    {
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ':'))
        {
            parts.push_back(part);
        }
        if (parts.size() >= 5)
        { // safety
            info.port = std::stoi(parts[2]);
            info.password = parts[3];
        }
    }

    LCU_LOG("Parsed port: " << info.port);
    LCU_LOG("Parsed password: " << info.password);

    return info;
}

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
