#pragma once

#include <filesystem>
#include <string>
#include <thread>

#include "lcuClient.h"
#include "parser.h"
#include "poll.h"
#include "debugPrints.h"

enum class gameState
{
    CLOSED,
    LOBBY,
    INGAME
};

inline bool handleLauncherState(std::atomic<gameState>& gameState)
{
    if (!std::filesystem::exists("C:\\Riot Games\\League of Legends\\lockfile"))
    {
        if (gameState.load() != gameState::CLOSED)
        {
            gameState.store(gameState::CLOSED);

            NEWLINE;
            LCU_LOG("Lockfile not found. League is closed (keep it closed pls).");
        }
        return true;
    }
    else
    {
        return false;
    }
}

inline void connectToLCU(LCUInfo& lcu)
{
    lcu = parseLockfile();

    if (lcu.port == 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

inline void getPlayerName(std::atomic<gameState>& gameState, LCUClient& lcuC, poll& poller,
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