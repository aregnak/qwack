#include "gamePoller.h"
#include "game.h"
#include "lcuClient.h"
#include "playerInfo.h"
#include "log.h"

GamePoller::GamePoller()
    : _players(10)
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

        while (running.load() && lcu.port == 0)
        {
            if (!_printedWaitingForClient)
            {
                LCU_LOG("Waiting for League client (open it...)");
                _printedWaitingForClient = true;
            }

            connectToLCU(lcu);
        }

        lcuC.connect(lcu);

        // Get player name
        while (running.load() && _playerName.empty())
        {
            getPlayerName(gameState, lcuC);
        }

        // Maybe this is poor design, but the while loops will be exitted
        // only when you actually launch the client, so state becomes lobby at this point.
        gameState.store(gameState::LOBBY);
    }
}

void GamePoller::handleLobbyState(std::atomic<gameState>& gameState,
                                  std::vector<std::string>& ranks, std::atomic<bool>& practicetool,
                                  std::atomic<float>& csPerMin)
{
    if (!_inLobby)
    {
        resetInGameCache(ranks, practicetool, csPerMin);
        _inLobby = true;
    }

    if (_poller.update())
    {
        // Default gametime before actually loading in is 0.01810079999268055. Yeah idk either.
        // This makes the INGAME state really mean loaded into the game, not just loading screen.
        if (_poller.getGameTime() > 0.5f)
        {
            gameState.store(gameState::INGAME);
        }
    }
}

void GamePoller::handleInGameState(LCUClient& lcuC, std::vector<std::string>& ranks,
                                   std::atomic<float>& csPerMin, std::atomic<gameState>& gameState,
                                   std::atomic<bool>& practicetool, std::mutex& dataMutex)
{
    static int currentCS = 0;

    if (_inLobby)
    {
        _inLobby = false;
        QWACK_LOG("GLHF.");
    }

    if (_poller.update())
    {
        if (!_playersLoaded)
        {
            std::vector<PlayerInfo> newPlayers(10);
            std::vector<std::string> newRanks;
            LCU_LOG("Polling Player Info...");

            _poller.getSessionInfo(lcuC, newPlayers);

            if (newPlayers.empty())
            {
                practicetool.store(true);
                QWACK_LOG("Gamemode is practice tool, skipping player info");
            }

            if (!practicetool.load())
            {
                getSessionPlayers(newRanks, lcuC);
            }
            _playersLoaded = true;

            std::lock_guard<std::mutex> lock(dataMutex);
            _players = std::move(newPlayers);
            ranks = std::move(newRanks);
        }

        currentCS = _poller.getcs(_playerName);
        float gold = _poller.getGold();
        float time = _poller.getGameTime();

        getCSPM(csPerMin, currentCS, time, gold);

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
        // If live client update doesn't update anymore.
        gameState.store(gameState::LOBBY);
    }
}

void GamePoller::connectToLCU(LCUInfo& lcu)
{
    lcu = parseLockfile();

    if (lcu.port == 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void GamePoller::getPlayerName(std::atomic<gameState>& gameState, LCUClient& lcuC)
{
    _playerName = _poller.getCurrentSummoner(lcuC);

    if (_playerName.empty())
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    else
    {
        gameState.store(gameState::LOBBY);
        QWACK_LOG("Summoner found: " << _playerName);
    }
}

void GamePoller::getSessionPlayers(std::vector<std::string>& newRanks, LCUClient& lcuC)
{
    // Unfortunately my understanding of the LCU API led me here,
    // to get players' ranks, we need the puuid, but to get their in game
    // stats, we need the live API (yes, different).
    // this code is very messy for now.
    for (auto& p : _players)
    {
        p.riotID = _poller.getPlayerName(lcuC, p.puuid);
        p.rank = _poller.getPlayerRank(lcuC, p.puuid);

        p.champ = _poller.getChampionNameById(p.champID);
        _poller.getPlayerRoleAndTeam(p);
    }

    sortPlayers(_players);

    for (const auto& p : _players)
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

void GamePoller::getCSPM(std::atomic<float>& csPerMin, int currentCS, float time, float gold)
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

void GamePoller::resetInGameCache(std::vector<std::string>& ranks, std::atomic<bool>& practicetool,
                                  std::atomic<float>& csPerMin)
{
    ranks.clear();
    _players = std::vector<PlayerInfo>(10);

    _playersLoaded = false;
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
