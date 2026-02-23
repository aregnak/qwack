#include "gamePoller.h"
#include "game.h"
#include "lcuClient.h"
#include "playerInfo.h"
#include "debugPrints.h"

namespace lcuPoller
{

void handleClosedState(LCUClient& lcuC, poll& poller, std::atomic<gameState>& gameState,
                       std::atomic<bool>& running, std::string& playerName,
                       bool& printedWaitingForClient)
{
    if (gameState.load() == gameState::CLOSED)
    {
        LCUInfo lcu;

        while (running.load() && lcu.port == 0)
        {
            if (!printedWaitingForClient)
            {
                LCU_LOG("Waiting for League client (open it...)");
                printedWaitingForClient = true;
            }

            lcuPoller::connectToLCU(lcu);
        }

        lcuC.connect(lcu);

        // Get player name
        while (running.load() && playerName.empty())
        {
            lcuPoller::getPlayerName(gameState, lcuC, poller, playerName);
        }

        gameState.store(gameState::LOBBY);
    }
}

void handleLobbyState(std::atomic<gameState>& gameState, poll& poller)
{
    if (poller.update())
    {
        // Default gametime before actually loading in is 0.01810079999268055. Yeah idk either.
        // This makes the INGAME state really mean loaded into the game, not just loading screen.
        if (poller.getGameTime() > 0.5f)
        {
            gameState.store(gameState::INGAME);
        }
    }
}

void handleInGameState(LCUClient& lcuC, std::vector<PlayerInfo>& players,
                       std::vector<std::string>& ranks, poll& poller, std::atomic<float>& csPerMin,
                       std::atomic<gameState>& gameState, std::atomic<bool>& playersLoaded,
                       std::atomic<bool>& practicetool, std::string& playerName,
                       std::mutex& dataMutex)
{
    static int currentCS = 0;

    if (poller.update())
    {
        if (!playersLoaded.load())
        {
            std::vector<PlayerInfo> newPlayers(10);
            std::vector<std::string> newRanks;
            LCU_LOG("Polling Player Info...");

            poller.getSessionInfo(lcuC, newPlayers);

            if (newPlayers.empty())
            {
                practicetool.store(true);
                QWACK_LOG("Gamemode is practice tool, skipping player info");
            }

            if (!practicetool.load())
            {
                lcuPoller::getSessionPlayers(newPlayers, newRanks, poller, lcuC);
            }
            playersLoaded.store(true);

            std::lock_guard<std::mutex> lock(dataMutex);

            players = std::move(newPlayers);
            ranks = std::move(newRanks);
        }

        currentCS = poller.getcs(playerName);
        float gold = poller.getGold();
        float time = poller.getGameTime();

        LCU_LOG("Current cs: " << currentCS << " time: " << time << " Gold: " << gold);
        // Minons spawn after 30 seconds, no need to measure anything before that.
        lcuPoller::getCSPM(csPerMin, currentCS, time, gold);
        // The cs/min counter will always be an approximation because the API updates the number
        // every 10 cs, this algorithm will somewhat smoothen that out, but any

        // currentGold.store(gold, std::memory_order_relaxed);
        // gameTime.store(time, std::memory_order_relaxed);

        // Item price polling
        // if (!practicetool)
        // {
        //     auto now = std::chrono::steady_clock::now();

        //     std::lock_guard<std::mutex> lock(dataMutex);

        //     if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPoll)
        //             .count() > 2)
        //     {
        //         for (size_t i = 0; i < players.size() / 2; i++)
        //         {
        //             PlayerInfo& currentPlayer = players[i];
        //             PlayerInfo& laneOpponent = players[i + 5];

        //             poller.getPlayerItems(currentPlayer);
        //             poller.getPlayerItems(laneOpponent);

        // poller.getPlayerItemSum(currentPlayer);
        // poller.getPlayerItemSum(laneOpponent);

        // itemGoldDiff[i] =
        //     (currentPlayer.itemsPrice - laneOpponent.itemsPrice);
        //         }
        //         lastPoll = now;
        //     }
        // }
    }
    else
    {
        gameState.store(gameState::LOBBY);
    }
}

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
        QWACK_LOG("Summoner found: " << playerName);
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

void getCSPM(std::atomic<float>& csPerMin, int currentCS, float time, float gold)
{
    static int lastCS = 0;
    static int estimatedCS = 0;
    static int totalCS = 0;
    static float lastGold = 0.0f;

    if (time >= 30.0f)
    {
        // CS counter updates every 10 CS, this algorithm will help estimate through gold delta.
        if (lastCS == currentCS)
        {
            if (gold - lastGold > 14.0f)
            {
                estimatedCS++;
            }
            // Get gold difference twice per second, we only want the delta IF there is a change of 14 or higher during poll.
            lastGold = gold;
        }
        else
        {
            lastCS = currentCS;
            estimatedCS = 0;
        }

        totalCS = estimatedCS + currentCS;

        // This is really just to make it a slight bit more accurate in case
        // something triggers a lot of additional "cs" but in reality it is something else.
        if (totalCS - currentCS > 10)
        {
            estimatedCS--;
        }
        csPerMin.store(totalCS / (time / 60.0f), std::memory_order_relaxed);
    }
}

} // namespace lcuPoller