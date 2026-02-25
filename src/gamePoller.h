#pragma once

#include <filesystem>
#include <string>
#include <thread>
#include <mutex>

#include "httplib.h"

#include "game.h"
#include "lcuClient.h"
#include "parser.h"
#include "poll.h"
#include "log.h"

class GamePoller
{
public:
    GamePoller() = default;

    void handleClosedState(LCUClient& lcuC, poll& poller, std::atomic<gameState>& gameState,
                           std::atomic<bool>& running, std::string&, bool& printedWaitingForClient);

    void handleLobbyState(std::atomic<gameState>& gameState, poll& poller,
                          std::vector<std::string>& ranks, std::vector<PlayerInfo>& players,
                          std::atomic<bool>& playersLoaded, std::atomic<bool>& practicetool,
                          std::atomic<float>& csPerMin);

    void handleInGameState(LCUClient& lcuC, std::vector<PlayerInfo>& players,
                           std::vector<std::string>& ranks, poll& poller,
                           std::atomic<float>& csPerMin, std::atomic<gameState>& gameState,
                           std::atomic<bool>& playersLoaded, std::atomic<bool>& practicetool,
                           std::string& playerName, std::mutex& dataMutex);

    void connectToLCU(LCUInfo&);
    void getPlayerName(std::atomic<gameState>&, LCUClient&, poll&, std::string&);
    void getSessionPlayers(std::vector<PlayerInfo>&, std::vector<std::string>&, poll&, LCUClient&);

    void getCSPM(std::atomic<float>& csPerMin, int currentCS, float time, float gold);

    void resetInGameCache(std::vector<std::string>& ranks, std::vector<PlayerInfo>& players,
                          std::atomic<bool>& playersLoaded, std::atomic<bool>& practicetool,
                          std::atomic<float>& csPerMin);

private:
    bool _inLobby = false;
};