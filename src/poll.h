#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <string>
#include <iostream>

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

    std::vector<std::string> getPUUIDs(LCUClient&);
    std::string getPlayerName(LCUClient&, std::string);
    std::string getPlayerRank(LCUClient&, std::string);
    void getPlayerRoleAndTeam(PlayerInfo&);

    float getGameTime();
    int getcs(const std::string&);
    float getGold();

    // void getPlayerGameInfo(StaticPlayer&);

private:
    httplib::SSLClient cli;
    httplib::Result res;
};