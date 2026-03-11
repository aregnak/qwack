#include "gamePoller.h"
#include "game.h"
#include "lcuClient.h"
#include "parser.h"
#include "playerInfo.h"
#include "log.h"
#include "poll.h"
#include <chrono>
#include <string>
#include <thread>
#include <wingdi.h>

GamePoller::GamePoller()
    : _players(10)
    , _itemGoldDiff(5)
    , _ranksReady(false)
{
    //
}

void GamePoller::handleClosedState(LCUClient& lcuC, std::atomic<gameState>& gameState,
                                   std::atomic<bool>& running)
{
    _inLobby = false;

    if (gameState.load() == gameState::CLOSED)
    {
        LCUInfo lcu;

        lcu = parseLockfile();

        while (running.load() && lcu.port == 0)
        {
            if (!_printedWaitingForClient)
            {
                LCU_LOG("Waiting for League client (open it...)");
                _printedWaitingForClient = true;
            }

            _now = std::chrono::steady_clock::now();

            if (std::chrono::duration_cast<std::chrono::seconds>(_now - _lastPoll).count() > 10)
            {
                lcu = parseLockfile();
                _lastPoll = _now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        lcuC.connect(lcu);

        // Get player name
        getPlayerName(gameState, lcuC);

        // Keep checking for name every 5 seconds.
        while (running.load() && _playerName.empty())
        {
            _now = std::chrono::steady_clock::now();

            if (std::chrono::duration_cast<std::chrono::seconds>(_now - _lastPoll).count() > 5)
            {
                getPlayerName(gameState, lcuC);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // Maybe this is poor design, but the while loops will be exitted
        // only when you actually launch the client, so state becomes lobby at this point.
        gameState.store(gameState::LOBBY);
    }
}

void GamePoller::handleLobbyState(LCUClient& lcuC, std::atomic<gameState>& gameState,
                                  std::atomic<bool>& practicetool, std::atomic<float>& csPerMin)
{
    if (!_inLobby)
    {
        resetInGameCache(practicetool, csPerMin);
        _inLobby = true;
    }

    // _now = std::chrono::steady_clock::now();

    // if (std::chrono::duration_cast<std::chrono::seconds>(_now - _lastPoll).count() > 5)
    // {
    if (_poller.update())
    {
        if (!_playersLoaded)
        {
            getAndSortSessionPlayers(lcuC, practicetool);
        }

        // Default gametime before actually loading in is 0.01810079999268055. Yeah idk either.
        // This makes the INGAME state really mean loaded into the game, not just loading screen.
        if (_poller.getGameTime() > 0.5f)
        {
            gameState.store(gameState::INGAME);
        }
    }
    // }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void GamePoller::handleInGameState(LCUClient& lcuC, std::atomic<float>& csPerMin,
                                   std::atomic<gameState>& gameState,
                                   std::atomic<bool>& practicetool)
{
    if (_inLobby)
    {
        _inLobby = false;
        QWACK_LOG("GLHF.");
    }

    if (_poller.update())
    {
        _currentCS = _poller.getcs(_playerName);
        float gold = _poller.getGold();
        float time = _poller.getGameTime();

        getCSPM(csPerMin, _currentCS, time, gold);

        // Item price polling
        if (!practicetool)
        {
            if (_playerInfoFailed)
            {
                _playerInfoNow = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(_playerInfoNow -
                                                                     _playerInfoLast)
                        .count() > 2)
                {
                    QWACK_LOG("Retrying player info fetch in game state.");
                    getAndSortSessionPlayers(lcuC, practicetool);

                    _playerInfoLast = _playerInfoNow;
                }
            }

            _now = std::chrono::steady_clock::now();

            if (std::chrono::duration_cast<std::chrono::seconds>(_now - _lastPoll).count() > 2)
            {
                _itemDiffReady.store(false);

                for (size_t i = 0; i < _players.size() / 2; i++)
                {
                    PlayerInfo& currentPlayer = _players[i]; // Blue team
                    PlayerInfo& laneOpponent = _players[i + 5]; // Red team

                    std::vector<int> bluePlayerItemIDs = _poller.getPlayerItemIDs(currentPlayer);
                    std::sort(bluePlayerItemIDs.begin(), bluePlayerItemIDs.end());

                    // You might be wondering why I don't sort currentPlayer and laneOpponent itemIDs,
                    // this is because those vectors are filled with the value of the already sorted
                    // bluePlayerItemIDs or redPlayerItemIDs respecively, so sorting the already sorted
                    // vector is not ideal in this case. This only works because this is the only place
                    // that writes into the PlayerInfo itemIDs.

                    if (!std::equal(bluePlayerItemIDs.begin(), bluePlayerItemIDs.end(),
                                    currentPlayer.itemIDs.begin(), currentPlayer.itemIDs.end()))
                    {
                        currentPlayer.totalItemPrice = 0;
                        for (int& itemID : bluePlayerItemIDs)
                        {
                            int price = _poller.getItemPrice(std::to_string(itemID));
                            currentPlayer.totalItemPrice += price;
                        }

                        currentPlayer.itemIDs = std::move(bluePlayerItemIDs);
                    }

                    std::vector<int> redPlayerItemIDs = _poller.getPlayerItemIDs(laneOpponent);
                    std::sort(redPlayerItemIDs.begin(), redPlayerItemIDs.end());

                    if (!std::equal(redPlayerItemIDs.begin(), redPlayerItemIDs.end(),
                                    laneOpponent.itemIDs.begin(), laneOpponent.itemIDs.end()))
                    {
                        laneOpponent.totalItemPrice = 0;
                        for (int& itemID : redPlayerItemIDs)
                        {
                            int price = _poller.getItemPrice(std::to_string(itemID));
                            laneOpponent.totalItemPrice += price;
                        }

                        laneOpponent.itemIDs = std::move(redPlayerItemIDs);
                    }

                    // Compute delta
                    _itemGoldDiff[i] = (currentPlayer.totalItemPrice - laneOpponent.totalItemPrice);
                }

                _lastPoll = _now;

                _itemDiffReady.store(true);
            }
        }
    }
    else
    {
        // If live client update doesn't update anymore.
        gameState.store(gameState::LOBBY);
    }
}

void GamePoller::getPlayerName(std::atomic<gameState>& gameState, LCUClient& lcuC)
{
    _playerName = _poller.getCurrentSummoner(lcuC);

    if (!_playerName.empty())
    {
        gameState.store(gameState::LOBBY);
        QWACK_LOG("Summoner found: " << _playerName);
    }
}

void GamePoller::getAllPlayersInfo(std::vector<PlayerInfo>& newPlayers, LCUClient& lcuC)
{
    // Unfortunately my understanding of the LCU API led me here,
    // to get players' ranks, we need the puuid, but to get their in game
    // stats, we need the live API (yes, different).
    // this code is very messy for now.
    _playerInfoFailed = false;

    for (auto& p : newPlayers)
    {
        p.riotID = _poller.getPlayerName(lcuC, p.puuid);
        p.rank = _poller.getPlayerRank(lcuC, p.puuid);

        p.champ = _poller.getChampionNameById(p.champID);
        _poller.getPlayerRoleAndTeam(p);

        auto fields = { p.riotID, p.rank, p.champ, p.role, p.team };
        if (std::any_of(fields.begin(), fields.end(), [](const auto& s) { return s.empty(); }))
        {
            _playerInfoFailed = true;
            break;
        }
    }

    if (!_playerInfoFailed)
    {
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

                _ranks.push_back(oss.str());
            }
            else
            {
                // If invalid tier returned, empty string.
                _ranks.push_back("");
            }

            LCU_LOG("puuid: " << p.puuid << " riotID: " << p.riotID << " rank: " << p.rank
                              << " role: " << p.role << " team: " << p.team);
        }

        _ranksReady.store(true);
        QWACK_LOG("Successfully loaded players.");
    }
}

void GamePoller::getAndSortSessionPlayers(LCUClient& lcuC, std::atomic<bool>& practicetool)
{
    std::vector<PlayerInfo> newPlayers(10);
    LCU_LOG("Polling Player Info...");

    _poller.getSessionInfo(lcuC, newPlayers, _gameMode);

    if (newPlayers.empty())
    {
        practicetool.store(true);
        QWACK_LOG("Gamemode is practice tool, skipping player info");
    }

    if (!practicetool.load())
    {
        getAllPlayersInfo(newPlayers, lcuC);
    }

    if (!_playerInfoFailed)
    {
        _playersLoaded = true;
        _players = std::move(newPlayers);
        QWACK_LOG("Player info fetching succeeded.");
    }
    else
    {
        QWACK_LOG("Player info fetching failed.");
    }
}

void GamePoller::getCSPM(std::atomic<float>& csPerMin, int currentCS, float time, float gold)
{
    if (time >= 30.0f)
    {
        // CS counter updates every 10 CS, this algorithm will help estimate through gold delta.
        if (_lastCS == currentCS)
        {
            if (gold - _lastGold > 14.0f)
            {
                _estimatedCS++;
            }
            // Get gold difference twice per second, we only want the delta IF there is a change of 14 or higher during poll.
            _lastGold = gold;
        }
        else
        {
            _lastCS = currentCS;
            _estimatedCS = 0;
        }

        _totalCS = _estimatedCS + currentCS;

        // This is really just to make it a slight bit more accurate in case
        // something triggers a lot of additional "cs" but in reality it is something else.
        if (_totalCS - currentCS > 10)
        {
            _estimatedCS--;
        }
        csPerMin.store(_totalCS / (time / 60.0f), std::memory_order_relaxed);
    }
}

void GamePoller::resetInGameCache(std::atomic<bool>& practicetool, std::atomic<float>& csPerMin)
{
    _ranks.clear();
    _ranksReady.store(false);

    // Clear and resize to 10.
    _players = std::vector<PlayerInfo>(10);
    _playersLoaded = false;

    _currentCS = 0;
    _lastCS = 0;
    _estimatedCS = 0;
    _totalCS = 0;
    _lastGold = 0.0f;

    _gameMode.clear();

    practicetool.store(false);
    csPerMin.store(0.0f);

    QWACK_LOG("In lobby. Waiting for game.");
}

void GamePoller::resetPlayerName()
{
    _playerName = std::string();
    //
}

void GamePoller::setPrintedWaitingForClient(bool state)
{
    _printedWaitingForClient = state;
    //
}

const bool GamePoller::isRanksReady()
{
    return _ranksReady.load();
    //
}

std::vector<std::string> GamePoller::getRanks()
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _ranks;
}

const bool GamePoller::isItemDiffReady()
{
    return _itemDiffReady.load();
    //
}

std::vector<int> GamePoller::getItemGoldDiff()
{
    std::lock_guard<std::mutex> lock(_dataMutex);
    return _itemGoldDiff;
    //
}

const std::string GamePoller::getGameMode()
{
    return _gameMode;
    //
}