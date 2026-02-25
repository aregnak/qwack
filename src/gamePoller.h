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
    GamePoller();

    void handleClosedState(LCUClient& lcuC, std::atomic<gameState>& gameState,
                           std::atomic<bool>& running);

    void handleLobbyState(std::atomic<gameState>& gameState, std::vector<std::string>& ranks,
                          std::atomic<bool>& practicetool, std::atomic<float>& csPerMin);

    void handleInGameState(LCUClient& lcuC, std::vector<std::string>& ranks,
                           std::atomic<float>& csPerMin, std::atomic<gameState>& gameState,
                           std::atomic<bool>& practicetool, std::mutex& dataMutex);

    void connectToLCU(LCUInfo&);
    void getPlayerName(std::atomic<gameState>& gameState, LCUClient& lcuC);
    void getSessionPlayers(std::vector<std::string>& newRanks, LCUClient& lcuC);

    void getCSPM(std::atomic<float>& csPerMin, int currentCS, float time, float gold);

    void resetInGameCache(std::vector<std::string>& ranks, std::atomic<bool>& practicetool,
                          std::atomic<float>& csPerMin);

    void resetPlayerName();
    void setPrintedWaitingForClient(bool state);

private:
    bool _playersLoaded = false;
    bool _inLobby = false;
    bool _printedWaitingForClient = false;

    std::string _playerName;

    poll _poller;
    std::vector<PlayerInfo> _players;
};