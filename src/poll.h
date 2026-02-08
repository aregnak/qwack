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

    std::string getCurrentSummoner(LCUClient&);

    void getSessionInfo(LCUClient&, std::vector<PlayerInfo>&);
    std::string getPlayerName(LCUClient&, const std::string);
    std::string getPlayerRank(LCUClient&, const std::string);
    void getPlayerRoleAndTeam(PlayerInfo&);

    std::string getChampionNameById(int);

    float getGameTime();
    int getcs(const std::string&);
    float getGold();

    std::string loadJsonFile(const std::string&);

    // void getPlayerGameInfo(StaticPlayer&);

private:
    void getChampionList();

    httplib::SSLClient cli;
    httplib::Result res;

    nlohmann::json championDataCache;
    nlohmann::json gameDataCache;
};