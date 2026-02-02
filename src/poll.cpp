#include "poll.h"
#include "lcuClient.h"
#include "playerInfo.h"

using json = nlohmann::json;

poll::poll()
    : cli("127.0.0.1", 2999)
{
    cli.enable_server_certificate_verification(false);
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

std::string loadJsonFile(const std::string& path)
{
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::vector<std::string> poll::getPUUIDs(LCUClient& lcu)
{
    // auto res = lcu.get("/lol-gameflow/v1/session");
    auto body = loadJsonFile("./session2.json");

    // auto session = json::parse(res->body);
    auto session = json::parse(body, nullptr, false);
    if (session.is_discarded())
    {
        return std::vector<std::string>();
    }

    std::vector<std::string> puuids;
    for (auto& p : session["gameData"]["playerChampionSelections"])
    {
        puuids.push_back(p["puuid"]);
        // std::cout << p["puuid"] << std::endl;
    }

    return puuids;
}

std::string poll::getPlayerName(LCUClient& lcu, std::string puuid)
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

std::string poll::getPlayerRank(LCUClient& lcu, std::string puuid)
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
        rstream << solo["tier"] << " " << solo["division"];
        return rstream.str();
    }
    return "";
}

void poll::getPlayerRoleAndTeam(PlayerInfo& player, std::string riotID)
{
    auto body = loadJsonFile("./allgamedata2.json");

    auto j = json::parse(body, nullptr, false);

    for (auto& j : j["allPlayers"])
    {
        if (j["riotId"].get<std::string>() == riotID)
        {
            player.role = j["position"];
            player.team = j["team"];
        }
    }
}

// std::string poll::getPlayerTeam()
// {
//     auto body = loadJsonFile("./allgamedata.json");

//     auto j = json::parse(body, nullptr, false);
//     for (auto& p : j["allPlayers"])
//     {
//         if (p["summonerName"].get<std::string>() == playerName)
//         {
//             cs = p["scores"]["creepScore"];
//         }
//     }
// }

float poll::getGameTime()
{
    auto j = json::parse(res->body);
    if (j.is_discarded())
    {
        LCU_LOG("JSON parse failed");
        return -1.0f;
    }

    float gameTime = j["gameData"]["gameTime"];
    return gameTime;
}

int poll::getcs(const std::string& playerName)
{
    auto j = json::parse(res->body);
    if (j.is_discarded())
    {
        LCU_LOG("JSON parse failed");
        return -1.0f;
    }

    int cs = 0;
    for (auto& p : j["allPlayers"])
    {
        if (p["summonerName"].get<std::string>() == playerName)
        {
            cs = p["scores"]["creepScore"];
        }
    }

    return cs;
}

float poll::getGold()
{
    auto j = json::parse(res->body);
    if (j.is_discarded())
    {
        LCU_LOG("JSON parse failed");
        return -1.0f;
    }

    float gold = j["activePlayer"]["currentGold"];
    // LCU_LOG("Player Gold: " << gold);

    return gold;
}

// void getPlayerGameInfo(StaticPlayer& player)
// {
//     // auto res = lcu.get("/lol-gameflow/v1/session");

//     // auto session = json::parse(res->body);
//     auto body = loadJsonFile("./allgamedata.json");

//     auto j = json::parse(body, nullptr, false);

//     player.riotID =

//     //
// }