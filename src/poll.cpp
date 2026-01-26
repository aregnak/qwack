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
        std::cout << "Not in game.";
        return false;
    }

    LCU_LOG("HTTP status: " << res->status);

    if (res->status != 200)
    {
        return false;
    }

    return true;
}

std::string poll::getPlayerName()
{
    res = cli.Get("/liveclientdata/activeplayername");

    std::string name = json::parse(res->body);

    if (!res) // if res is a nullptr
    {
        std::cout << "Not in game.";
        return "";
    }

    return name;
}

float poll::getcs(const std::string& playerName)
{
    // LCU_LOG("Polling LCU...");

    // httplib::SSLClient cli("127.0.0.1", 2999);
    // cli.enable_server_certificate_verification(false);

    // auto res = cli.Get("/liveclientdata/allgamedata");

    // LCU_LOG("HTTP status: " << res->status);

    auto j = json::parse(res->body);
    if (j.is_discarded())
    {
        LCU_LOG("JSON parse failed");
        return -1.0f;
    }

    float gameTime = j["gameData"]["gameTime"];
    int cs = 0;
    float gold = 0.0f;
    for (auto& p : j["allPlayers"])
    {
        // std::string name = p["summonerName"];
        // std::string team = p["team"];

        // LCU_LOG(" - " << name << " | Team: " << team);

        if (p["summonerName"].get<std::string>() == playerName)
        {
            cs = p["scores"]["creepScore"];
            LCU_LOG("Found player " << playerName << " CS=" << cs);
        }
    }

    float tempTime = 0;
    float goldDelta = 0;
    float prevGold = 0;

    gold = j["activePlayer"]["currentGold"];
    std::cout << "Player Gold: " << gold << std::endl;

    if (tempTime <= gameTime - 0.8f)
    {
        tempTime = gameTime;
        prevGold = gold;
    }

    goldDelta = gold - prevGold;
    std::cout << "Gold delta: " << goldDelta << std::endl;

    float cspm = cs / (gameTime / 60.0f);

    LCU_LOG("CS/min = " << cspm);
    return cspm;
}