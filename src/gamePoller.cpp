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

GamePoller::GamePoller()
    : _players(10)
    , _itemGoldDiff(5)
    , _ranksReady(false)
{
    //
}

void GamePoller::handleClosedState(LCUClient& lcuC, std::atomic<leagueState>& leagueState,
                                   std::atomic<bool>& running)
{
    _inLobby = false;

    if (leagueState.load() == leagueState::CLOSED)
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
        getPlayerName(leagueState, lcuC);

        // Keep checking for name every 5 seconds.
        while (running.load() && _playerName.empty())
        {
            _now = std::chrono::steady_clock::now();

            if (std::chrono::duration_cast<std::chrono::seconds>(_now - _lastPoll).count() > 5)
            {
                getPlayerName(leagueState, lcuC);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // Maybe this is poor design, but the while loops will be exitted
        // only when you actually launch the client, so state becomes lobby at this point.
        leagueState.store(leagueState::LOBBY);

        // Make sure the next time _lastPoll is compared, the difference is more than 5 seconds.
        _lastPoll = std::chrono::steady_clock::now() - std::chrono::seconds(6);
    }
}

void GamePoller::handleLobbyState(LCUClient& lcuC, std::atomic<leagueState>& leagueState,
                                  std::atomic<GameMode>& gameMode, std::atomic<float>& csPerMin)
{
    if (!_inLobby)
    {
        resetInGameCache(gameMode, csPerMin);

        QWACK_LOG("In lobby. Waiting for game.");

        _inLobby = true;
    }

    _now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::seconds>(_now - _lastPoll).count() > 5)
    {
        if (_poller.update())
        {
            _poller.getGameMode(lcuC, gameMode);

            if (gameMode.load() != GameMode::SPECTATOR)
            {
                if (!_playersLoaded)
                {
                    getAndSortSessionPlayers(lcuC, gameMode);
                }

                // Default gametime before actually loading in is 0.01810079999268055. Yeah idk either.
                // This makes the INGAME state really mean loaded into the game, not just loading screen.
                if (_poller.getGameTime() > 0.5f)
                {
                    leagueState.store(leagueState::INGAME);
                }
            }
        }
        else
        {
            if (gameMode.load() == GameMode::SPECTATOR)
            {
                gameMode.store(GameMode::NONE);
            }
        }

        _lastPoll = _now;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void GamePoller::handleInGameState(LCUClient& lcuC, std::atomic<leagueState>& leagueState,
                                   std::atomic<GameMode>& gameMode, std::atomic<float>& csPerMin)
{
    if (_inLobby)
    {
        QWACK_LOG("GLHF.");

        _inLobby = false;
    }

    if (_poller.update())
    {
        _currentCS = _poller.getcs(_playerName);
        float gold = _poller.getGold();
        float time = _poller.getGameTime();

        getCSPM(csPerMin, _currentCS, time, gold);

        // Item price polling
        if (gameMode.load() != GameMode::PRACTICETOOL && gameMode.load() != GameMode::SPECTATOR)
        {
            // Retry polling players if failed initially.
            if (_playerInfoFailed)
            {
                _playerInfoNow = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(_playerInfoNow -
                                                                     _playerInfoLast)
                        .count() > 2)
                {
                    QWACK_LOG("Retrying player info fetch in in-game state.");
                    getAndSortSessionPlayers(lcuC, gameMode);

                    _playerInfoLast = _playerInfoNow;
                }
            }
            else // If session players' info loaded successfully.
            {
                _now = std::chrono::steady_clock::now();

                if (std::chrono::duration_cast<std::chrono::seconds>(_now - _lastPoll).count() > 2)
                {
                    pollGoldDiff();
                }
            }
        }
    }
    else
    {
        // If live client update doesn't update anymore.
        leagueState.store(leagueState::LOBBY);
    }
}

void GamePoller::getPlayerName(std::atomic<leagueState>& leagueState, LCUClient& lcuC)
{
    _playerName = _poller.getCurrentSummoner(lcuC);

    if (!_playerName.empty())
    {
        leagueState.store(leagueState::LOBBY);
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

void GamePoller::getAndSortSessionPlayers(LCUClient& lcuC, std::atomic<GameMode>& gameMode)
{
    std::vector<PlayerInfo> newPlayers(10);
    LCU_LOG("Polling Player Info...");

    _poller.getSessionInfo(lcuC, newPlayers, gameMode);

    if (gameMode.load() != GameMode::PRACTICETOOL)
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

// This is less accurate for jungle camps especially early game.
void GamePoller::getCSPM(std::atomic<float>& csPerMin, int currentCS, float time, float gold)
{
    if (time >= 30.0f)
    {
        // CS counter updates every 10 CS, this algorithm will help estimate through gold delta.
        if (_lastCS == currentCS)
        {
            if (gold - _lastGold > 14.0f && gold - _lastGold <= 90)
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

void GamePoller::pollGoldDiff()
{
    _itemDiffReady.store(false);

    for (size_t i = 0; i < _players.size() / 2; i++)
    {
        PlayerInfo& currentPlayer = _players[i]; // Blue team
        PlayerInfo& laneOpponent = _players[i + 5]; // Red team

        // Poll for currentPlayer's items and sort them, then check for a difference
        // with currentPlayer's items from last poll (currentPlayer.itemIDs), and if
        // there is a difference, recalculate total item price and move the new item
        // vector into currentPlayer.

        std::vector<int> bluePlayerItemIDs = _poller.getPlayerItemIDs(currentPlayer);
        std::sort(bluePlayerItemIDs.begin(), bluePlayerItemIDs.end());

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

        // Do the same for the red team player. `laneOpponent` is a bad name here
        // because it makes this process sound relative to our player's team. It isn't.
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

void GamePoller::resetInGameCache(std::atomic<GameMode>& gameMode, std::atomic<float>& csPerMin)
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
    _lastGold = 500.0f;

    gameMode.store(GameMode::NONE);
    csPerMin.store(0.0f);

    QWACK_LOG("In game cache reset.");
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