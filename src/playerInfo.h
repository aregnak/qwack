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

// // In-game info from /allgamedata
// struct StaticPLayer
// {
//     std::string riotID;
//     std::string team;
//     std::string role;
// };

// std::unordered_map<std::string, StaticPLayer> roleLookup;

// void cacheRoles(const json& allGameData)
// {
//     roleLookup.clear();

//     for (const auto& p : allGameData["allPlayers"])
//     {
//         std::string riotId = NormalizeRiotID(p["riotIdGameName"], p["riotIdTagLine"]);

//         roleLookup[riotId] = { riotId, p["team"], p["position"] };
//     }
// }

// static const std::vector<std::string> ROLE_ORDER = { "TOP", "JUNGLE", "MIDDLE", "BOTTOM",
//                                                      "UTILITY" };

// int roleIndex(const std::string& role)
// {
//     auto it = std::find(ROLE_ORDER.begin(), ROLE_ORDER.end(), role);
//     return it == ROLE_ORDER.end() ? 99 : std::distance(ROLE_ORDER.begin(), it);
// }

// void sortPlayers(std::vector<PlayerInfo>& players)
// {
//     std::sort(players.begin(), players.end(),
//               [&](const PlayerInfo& a, const PlayerInfo& b)
//               {
//                   const auto& A = roleLookup.at(a.riotID);
//                   const auto& B = roleLookup.at(b.riotID);

//                   // ORDER team first
//                   if (A.team != B.team)
//                       return A.team == "ORDER";

//                   // Role order inside team
//                   return RoleIndex(A.role) < RoleIndex(B.role);
//               });
// }
