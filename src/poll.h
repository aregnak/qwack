#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <string>
#include <iostream>
#include <vector>

#include "httplib.h"
#include "json.hpp"

#include "lcuClient.h"
#include "playerInfo.h"

class poll
{
public:
    poll();

    bool update(); // Live Client Data API

    // Using LCU (in client).
    std::string getCurrentSummoner(LCUClient&);
    void getSessionInfo(LCUClient&, std::vector<PlayerInfo>&);
    std::string getPlayerName(LCUClient&, const std::string);
    std::string getPlayerRank(LCUClient&, const std::string);

    // Using Live Client API (in game).
    void getPlayerRoleAndTeam(PlayerInfo&);
    void getPlayerItemSum(PlayerInfo&);
    int getcs(const std::string&);
    float getGameTime();
    float getGold();

    // Helper functions
    std::string getChampionNameById(int);
    std::string loadJsonFile(const std::string&);

    // void getPlayerGameInfo(StaticPlayer&);

private:
    void getChampionList();
    void getItemList();

    httplib::SSLClient cli;
    httplib::Result res;

    nlohmann::json championDataCache;
    nlohmann::json gameDataCache;
    nlohmann::json itemDataCache;
};