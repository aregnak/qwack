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

    std::string getPlayerName(LCUClient&);

    float getGameTime();
    int getcs(const std::string& playerName);
    float getGold();

private:
    httplib::SSLClient cli;
    httplib::Result res;
};