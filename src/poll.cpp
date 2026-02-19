#include "poll.h"
#include "parser.h"
#include "playerInfo.h"
#include <cstddef>
#include <string>
#include <thread>

using json = nlohmann::json;

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
    res = cli.Get("/liveclientdata/allgamedata");

    if (!res) // if res is a nullptr
    {
        //std::cout << "Not in game.";
        return false;
    }

    //LCU_LOG("HTTP status: " << res->status);

    if (res->status != 200)
    {
        return false;
    }

    gameDataCache = json::parse(res->body);

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
        std::cout << "Failed to get name." << std::endl;
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

void poll::getSessionInfo(LCUClient& lcu, std::vector<PlayerInfo>& players)
{
    auto res = lcu.get("/lol-gameflow/v1/session");
    auto session = json::parse(res->body);
    // auto body = loadJsonFile("./session2.json");
    // auto session = json::parse(body, nullptr, false);

    const std::string gameMode = session["gameData"]["queue"]["gameMode"].get<std::string>();
    LCU_LOG("Game mode: " << gameMode);

    if (gameMode != "PRACTICETOOL")
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
        std::cout << "Failed to get name." << std::endl;
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
    // res = cli.Get("/liveclientdata/allgamedata");

    // auto gameData = json::parse(res->body);
    // auto body = loadJsonFile("./allgamedata2.json");

    // auto gameData = json::parse(body, nullptr, false);

    for (const auto& j : gameDataCache["allPlayers"])
    {
        if (j["championName"] == player.champ)
        {
            player.role = j["position"];
            player.team = j["team"];
        }
    }
}

void poll::getPlayerItemSum(PlayerInfo& player)
{
    // auto body = loadJsonFile("./allgamedata2.json");

    // auto gameData = json::parse(body, nullptr, false);

    int itemsPrice = 0;
    for (const auto& j : gameDataCache["allPlayers"])
    {
        if (j["championName"] == player.champ)
        {
            for (const auto& i : j["items"])
            {
                int price = i["price"].get<int>();
                itemsPrice += price;
            }
            LCU_LOG("Player: " << j["championName"] << " Item Total: " << itemsPrice);
            player.itemsPrice = itemsPrice;
            break;
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