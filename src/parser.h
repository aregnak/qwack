// This file is not needed for now, I misunderstood the LCU API at first.

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

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
