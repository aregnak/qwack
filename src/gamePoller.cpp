#include "gamePoller.h"
#include "lcuClient.h"
#include "playerInfo.h"

namespace lcuPoller
{
void connectToLCU(LCUInfo& lcu)
{
    lcu = parseLockfile();

    if (lcu.port == 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void getPlayerName(std::atomic<gameState>& gameState, LCUClient& lcuC, poll& poller,
                   std::string& playerName)
{
    playerName = poller.getCurrentSummoner(lcuC);

    if (playerName.empty())
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    else
    {
        gameState.store(gameState::LOBBY);
    }
}

void getSessionPlayers(std::vector<PlayerInfo>& newPlayers, std::vector<std::string>& newRanks,
                       poll& poller, LCUClient& lcuC)
{
    // Unfortunately my understanding of the LCU API led me here,
    // to get players' ranks, we need the puuid, but to get their in game
    // stats, we need the live API (yes, different).
    // this code is very messy for now.
    for (auto& p : newPlayers)
    {
        p.riotID = poller.getPlayerName(lcuC, p.puuid);
        p.rank = poller.getPlayerRank(lcuC, p.puuid);

        p.champ = poller.getChampionNameById(p.champID);
        poller.getPlayerRoleAndTeam(p);
    }

    sortPlayers(newPlayers);

    for (const auto& p : newPlayers)
    {
        char rankLetter = p.rank[0];
        int tierNumber = romanToInt(p.rank.substr(p.rank.find(' ') + 1));

        if (tierNumber != -1)
        {
            std::ostringstream oss;
            oss << rankLetter;

            // If rank doesn't contain tiers (Master+).
            if (tierNumber != 0)
            {
                oss << tierNumber;
            }
            newRanks.push_back(oss.str());
        }
        else
        {
            newRanks.push_back("");
        }

        std::cout << "puuid: " << p.puuid << " riotID: " << p.riotID << " rank: " << p.rank
                  << " role: " << p.role << " team: " << p.team << std::endl;
    }
    QWACK_LOG("Successfully loaded players.");
}
} // namespace lcuPoller