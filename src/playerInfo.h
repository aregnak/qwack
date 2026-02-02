#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include "json.hpp"

struct PlayerInfo
{
    std::string puuid;
    std::string riotID;
    std::string rank;
    std::string role;
    std::string team;
};

// In-game info from /allgamedata
// struct StaticPlayer
// {
//     std::string riotID;
//     std::string team;
//     std::string role;
// };