#include "poll.h"

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

std::string poll::getPlayerName(LCUClient& lcu)
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