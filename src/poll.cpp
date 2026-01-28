#include "poll.h"
#include "parser.h"

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

std::string poll::getPlayerName(const LCUInfo& lcu)
{
    std::stringstream ss;
    ss << "riot:" << lcu.password;
    std::string auth = base64(ss.str());

    httplib::SSLClient ncli("127.0.0.1", lcu.port);
    ncli.enable_server_certificate_verification(false);

    httplib::Headers headers = { { "Authorization", "Basic " + auth } };

    auto nres = ncli.Get("/lol-summoner/v1/current-summoner", headers);

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

float poll::getcs(const std::string& playerName)
{
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
        if (p["summonerName"].get<std::string>() == playerName)
        {
            cs = p["scores"]["creepScore"];
            LCU_LOG("Found player " << playerName << " CS=" << cs);
        }
    }

    // float tempTime = 0;
    // float goldDelta = 0;
    // float prevGold = 0;

    // gold = j["activePlayer"]["currentGold"];
    // std::cout << "Player Gold: " << gold << std::endl;

    // if (tempTime <= gameTime - 0.8f)
    // {
    //     tempTime = gameTime;
    //     prevGold = gold;
    // }

    // goldDelta = gold - prevGold;
    // std::cout << "Gold delta: " << goldDelta << std::endl;

    float cspm = cs / (gameTime / 60.0f);

    LCU_LOG("CS/min = " << cspm);
    return cspm;
}