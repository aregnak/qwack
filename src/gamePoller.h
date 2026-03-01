#pragma once

#include <chrono>
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

    void handleLobbyState(LCUClient& lcuC, std::atomic<gameState>& gameState,
                          std::atomic<bool>& practicetool, std::atomic<float>& csPerMin);

    void handleInGameState(LCUClient& lcuC, std::atomic<float>& csPerMin,
                           std::atomic<gameState>& gameState, std::atomic<bool>& practicetool);

    void resetPlayerName();
    void setPrintedWaitingForClient(bool state);

    const bool isRanksReady();
    std::vector<std::string> getRanks();

    const bool isItemDiffReady();
    std::vector<int> getItemGoldDiff();

private:
    void connectToLCU(LCUInfo&);
    void getPlayerName(std::atomic<gameState>& gameState, LCUClient& lcuC);
    void getSessionPlayers(std::vector<PlayerInfo>& newPlayers, LCUClient& lcuC);

    void getCSPM(std::atomic<float>& csPerMin, int currentCS, float time, float gold);

    void resetInGameCache(std::atomic<bool>& practicetool, std::atomic<float>& csPerMin);

    bool _playersLoaded = false;
    bool _inLobby = false;
    bool _printedWaitingForClient = false;

    std::string _playerName;

    poll _poller;
    std::vector<PlayerInfo> _players;
    std::vector<std::string> _ranks;

    std::mutex _dataMutex;
    std::atomic<bool> _ranksReady;

    std::atomic<bool> _itemDiffReady;
    std::vector<int> _itemGoldDiff;

    // :(
    std::chrono::time_point<std::chrono::steady_clock> _now = std::chrono::steady_clock::now();
    std::chrono::time_point<std::chrono::steady_clock> _lastPoll = std::chrono::steady_clock::now();
};