#pragma once

#include <string>
#include <unordered_map>

#include "json.hpp"

struct PlayerInfo
{
    std::string puuid;
    std::string riotID;
    // std::string role;
    std::string rank;
};

// In-game info from /allgamedata
struct StaticPLayer
{
    std::string riotID;
    std::string team;
    std::string role;
};

std::unordered_map<std::string, StaticPLayer> roleLookup;

void CacheRoles(const json& allGameData)
{
    roleLookup.clear();

    for (const auto& p : allGameData["allPlayers"])
    {
        std::string riotId = NormalizeRiotID(p["riotIdGameName"], p["riotIdTagLine"]);

        roleLookup[riotId] = { riotId, p["team"], p["position"] };
    }
}