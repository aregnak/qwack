#include "poll.h"
#include "log.h"
#include "parser.h"
#include "playerInfo.h"
#include <cstddef>
#include <string>

using json = nlohmann::json;

// This is different from MSVC Debug build, this is for static json tests.
// TODO: Create a better way to conduct static tests.
#define DEBUG_ENABLED false

poll::poll()
    : cli("127.0.0.1", 2999)
{
    cli.enable_server_certificate_verification(false);

    // This is done fast enough not to need its own thread or local data.
    getGameVersion();
    getChampionList();
    getItemList();
}

bool poll::update()
{
#if DEBUG_ENABLED
    auto body = loadJsonFile("./allgamedata2.json");
    if (body.empty())
    {
        LCU_LOG("JSON file is empty or missing.");
        return false;
    }

    gameDataCache = json::parse(body, nullptr, false);

#else
    res = cli.Get("/liveclientdata/allgamedata");

    if (!res) // if res is a nullptr
    {
        return false;
    }

    if (res->status != 200)
    {
        LCU_LOG("Live client data status: " << res->status);
        return false;
    }

    gameDataCache = json::parse(res->body);

#endif // DEBUG_ENABLED

    if (gameDataCache.is_discarded())
    {
        LCU_LOG("Failed to parse /allgamedata.");
        return false;
    }

    return true;
}

std::string poll::getCurrentSummoner(LCUClient& lcu)
{
    auto nres = lcu.get("/lol-summoner/v1/current-summoner");

    if (!nres) // if res is a nullptr
    {
        LCU_LOG("Failed to get Current summoner.");
        return "";
    }

    if (nres->status != 200)
    {
        return "";
    }

    auto name = json::parse(nres->body);
    if (name.is_discarded())
    {
        return "";
    }

    std::stringstream nstream;
    nstream << name["gameName"].get<std::string>() << "#" << name["tagLine"].get<std::string>();

    return nstream.str();
}

void poll::getGameMode(LCUClient& lcu, std::atomic<GameMode>& gameMode)
{
#if DEBUG_ENABLED
    auto body = loadJsonFile("./session2.json");
    auto session = json::parse(body, nullptr, false);

#else
    auto res = lcu.get("/lol-gameflow/v1/session");
    auto session = json::parse(res->body);

#endif // DEBUG_ENABLED

    // If you watch a replay, the game mode will display as that of the actual game, not a different
    // one specifying spectator mode. However, the active player entry will have an error instead.
    if (gameDataCache.contains("activePlayer") && gameDataCache["activePlayer"].contains("error"))
    {
        gameMode.store(GameMode::SPECTATOR);
        return;
    }
    else
    {
        std::string parsedGameMode = session["gameData"]["queue"]["gameMode"].get<std::string>();

        // Prototype code.
        static const std::unordered_map<std::string, GameMode> stringToGameMode = {
            { "CLASSIC", GameMode::CLASSIC },
            { "SWIFTPLAY", GameMode::SWIFTPLAY },
            { "ARAM", GameMode::ARAM },
            { "KIWI", GameMode::KIWI },
            { "PRACTICETOOL", GameMode::PRACTICETOOL },
            { "TUTORIAL_MODULE_1", GameMode::TUTORIAL_MODULE_1 }
        };

        if (session.contains("gameData") && session["gameData"].contains("gameMode") &&
            session["gameData"]["gameMode"].is_string())
        {
            parsedGameMode = session["gameData"]["gameMode"];
        }

        auto it = stringToGameMode.find(parsedGameMode);
        gameMode.store(it != stringToGameMode.end() ? it->second : GameMode::UNKNOWN);
        // Refactor this mess into a function.
    }
}

void poll::getSessionInfo(LCUClient& lcu, std::vector<PlayerInfo>& players,
                          std::atomic<GameMode>& gameMode)
{
#if DEBUG_ENABLED
    auto body = loadJsonFile("./session2.json");
    auto session = json::parse(body, nullptr, false);

#else
    auto res = lcu.get("/lol-gameflow/v1/session");
    auto session = json::parse(res->body);

#endif // DEBUG_ENABLED

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // ! Update this so there is no recursive gameMode parsing.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    std::string parsedGameMode = session["gameData"]["queue"]["gameMode"].get<std::string>();

    LCU_LOG("Game mode: " << parsedGameMode);

    if (parsedGameMode != "PRACTICETOOL")
    {
        size_t i = 0;
        for (const auto& p : session["gameData"]["playerChampionSelections"])
        {
            if (i >= 10)
            {
                break;
            }
            players[i].puuid = p["puuid"].get<std::string>();
            players[i].champID = p["championId"];
            i++;
        }

        LCU_LOG("Finished getting session info.");
    }
    else
    {
        LCU_LOG("Skipped session info.");
        players.clear();
    }
}

std::string poll::getPlayerName(LCUClient& lcu, const std::string puuid)
{
    auto nres = lcu.get("/lol-summoner/v2/summoners/puuid/" + puuid);

    if (!nres) // if res is a nullptr
    {
        LCU_LOG("Failed to get name.");
        return "";
    }

    if (nres->status != 200)
    {
        return "";
    }

    auto name = json::parse(nres->body);
    if (name.is_discarded())
    {
        return "";
    }

    std::stringstream nstream;
    nstream << name["gameName"].get<std::string>() << "#" << name["tagLine"].get<std::string>();

    return nstream.str();
}

std::string poll::getPlayerRank(LCUClient& lcu, const std::string puuid)
{
    auto nres = lcu.get("/lol-ranked/v1/ranked-stats/" + puuid);

    if (!nres) // if res is a nullptr
    {
        return "";
    }

    if (nres->status != 200)
    {
        return "";
    }

    auto rank = json::parse(nres->body);
    if (rank.is_discarded())
    {
        return "";
    }

    if (rank.contains("queueMap") && rank["queueMap"].contains("RANKED_SOLO_5x5"))
    {
        auto& solo = rank["queueMap"]["RANKED_SOLO_5x5"];
        std::stringstream rstream;
        rstream << solo["tier"].get<std::string>() << " " << solo["division"].get<std::string>();
        return rstream.str();
    }
    return "";
}

void poll::getPlayerRoleAndTeam(PlayerInfo& player)
{
    for (const auto& j : gameDataCache["allPlayers"])
    {
        if (j["championName"] == player.champ)
        {
            player.role = j["position"];
            player.team = j["team"];
            // QWACK_LOG("POS: " << player.role);
            // QWACK_LOG("TEAM: " << player.team);
            break;
        }
    }
}

std::vector<int> poll::getPlayerItemIDs(PlayerInfo& player)
{
    std::vector<int> itemIDs;

    for (const auto& j : gameDataCache["allPlayers"])
    {
        if (j["championName"] == player.champ)
        {
            for (const auto& i : j["items"])
            {
                itemIDs.push_back(i["itemID"].get<int>());
            }

            return itemIDs;
        }
    }
}

int poll::getcs(const std::string& playerName)
{
    int cs = 0;
    for (const auto& p : gameDataCache["allPlayers"])
    {
        if (p["summonerName"].get<std::string>() == playerName)
        {
            cs = p["scores"]["creepScore"];
            break;
        }
    }

    return cs;
}

float poll::getGameTime()
{
    return gameDataCache["gameData"]["gameTime"];
    //
}

float poll::getGold()
{
    return gameDataCache["activePlayer"]["currentGold"];
    //
}

// Helper functions
int poll::getItemPrice(std::string itemID)
{
    if (itemDataCache["data"].contains(itemID))
    {
        return itemDataCache["data"][itemID]["gold"]["total"];
    }

    return 0;
}

std::string poll::getChampionNameById(int id)
{
    std::string idstr = std::to_string(id);

    for (const auto& [name, champ] : championDataCache["data"].items())
    {
        if (champ["key"].get<std::string>() == idstr)
        {
            return champ["name"].get<std::string>();
        }
    }

    return "UnknownChampion";
}

// Helper function used for static testing.
std::string poll::loadJsonFile(const std::string& path)
{
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Private
void poll::getGameVersion()
{
    httplib::Client cli("https://ddragon.leagueoflegends.com");

    // Get current game version.
    auto res = cli.Get("/api/versions.json");
    if (!res || res->status != 200)
    {
        LCU_LOG("Failed to load ddragon versions.");
    }

    auto versions = json::parse(res->body, nullptr, false);
    if (versions.is_discarded())
    {
        LCU_LOG("Failed to parse ddragon versions.");
    }

    gameVersion = versions[0];
}

void poll::getChampionList()
{
    httplib::Client cli("https://ddragon.leagueoflegends.com");

    // Get champion list based on current version.
    auto cres = cli.Get("/cdn/" + gameVersion + "/data/en_US/champion.json");

    if (!cres || cres->status != 200)
    {
        LCU_LOG("Failed to load ddragon champions.");
    }

    championDataCache = json::parse(cres->body, nullptr, false);
    if (championDataCache.is_discarded())
    {
        LCU_LOG("Failed to parse ddragon champions.");
    }
    else
    {
        LCU_LOG("Successfully loaded champions, version: " + gameVersion);
    }
}

void poll::getItemList()
{
    httplib::Client cli("https://ddragon.leagueoflegends.com");

    // Get champion list based on current version.
    auto cres = cli.Get("/cdn/" + gameVersion + "/data/en_US/item.json");

    if (!cres || cres->status != 200)
    {
        LCU_LOG("Failed to load ddragon items.");
    }

    itemDataCache = json::parse(cres->body, nullptr, false);
    if (itemDataCache.is_discarded())
    {
        LCU_LOG("Failed to parse ddragon items.");
    }
    else
    {
        LCU_LOG("Successfully loaded items, version: " + gameVersion);
    }
}