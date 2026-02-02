#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "json.hpp"

struct PlayerInfo
{
    std::string puuid;
    std::string riotID;
    std::string rank;
    std::string role;
    std::string team;
};

// Sort players based on the role order right below.
// Needed for gold difference, ranks, etc...
inline void sortPlayers(std::vector<PlayerInfo>& players)
{
    std::unordered_map<std::string, int> roleOrder = {
        { "TOP", 0 }, { "JUNGLE", 1 }, { "MIDDLE", 2 }, { "BOTTOM", 3 }, { "UTILITY", 4 }
    };

    std::unordered_map<std::string, int> teamOrder = { { "ORDER", 0 }, { "CHAOS", 1 } };

    std::stable_sort(players.begin(), players.end(),
                     [&roleOrder, &teamOrder](const PlayerInfo& a, const PlayerInfo& b)
                     {
                         if (a.team != b.team)
                         {
                             return teamOrder.at(a.team) < teamOrder.at(b.team);
                         }

                         return roleOrder.at(a.role) < roleOrder.at(b.role);
                     });
}