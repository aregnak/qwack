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

    void handleClosedState(LCUClient& lcuC, std::atomic<leagueState>& leagueState,
                           std::atomic<bool>& running);

    void handleLobbyState(LCUClient& lcuC, std::atomic<leagueState>& leagueState,
                          std::atomic<bool>& practicetool, std::atomic<float>& csPerMin);

    void handleInGameState(LCUClient& lcuC, std::atomic<float>& csPerMin,
                           std::atomic<leagueState>& leagueState, std::atomic<bool>& practicetool);

    void resetPlayerName();
    void setPrintedWaitingForClient(bool state);

    const bool isRanksReady();
    std::vector<std::string> getRanks();

    const bool isItemDiffReady();
    std::vector<int> getItemGoldDiff();

    const std::string getGameMode();

private:
    void getPlayerName(std::atomic<leagueState>& leagueState, LCUClient& lcuC);
    void getAllPlayersInfo(std::vector<PlayerInfo>& newPlayers, LCUClient& lcuC);
    void getAndSortSessionPlayers(LCUClient& lcuC, std::atomic<bool>& practicetool);

    void getCSPM(std::atomic<float>& csPerMin, int currentCS, float time, float gold);
    void pollGoldDiff();

    void resetInGameCache(std::atomic<bool>& practicetool, std::atomic<float>& csPerMin);

    bool _playersLoaded = false;
    bool _inLobby = false;
    bool _printedWaitingForClient = false;

    int _currentCS = 0;
    int _lastCS = 0;
    int _estimatedCS = 0;
    int _totalCS = 0;
    float _lastGold = 0.0f;

    std::string _playerName;

    std::string _gameMode;

    poll _poller;
    std::vector<PlayerInfo> _players;
    std::vector<std::string> _ranks;

    bool _playerInfoFailed = false;

    std::mutex _dataMutex;
    std::atomic<bool> _ranksReady;

    std::atomic<bool> _itemDiffReady;
    std::vector<int> _itemGoldDiff;

    // :(
    std::chrono::time_point<std::chrono::steady_clock> _now = std::chrono::steady_clock::now();
    std::chrono::time_point<std::chrono::steady_clock> _lastPoll = std::chrono::steady_clock::now();

    std::chrono::time_point<std::chrono::steady_clock> _playerInfoNow =
        std::chrono::steady_clock::now();
    std::chrono::time_point<std::chrono::steady_clock> _playerInfoLast =
        std::chrono::steady_clock::now();
};