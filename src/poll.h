#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <string>
#include <iostream>

#include "httplib.h"
#include "json.hpp"

#include "lcuClient.h"

class poll
{
public:
    poll();

    bool update(); // Live Client Data API

    std::string getCurrentSummoner(LCUClient&);
    std::string getPlayerName(LCUClient&, std::string);
    std::vector<std::string> getPUUIDs(LCUClient&);

    float getGameTime();
    int getcs(const std::string&);
    float getGold();

private:
    httplib::SSLClient cli;
    httplib::Result res;
};