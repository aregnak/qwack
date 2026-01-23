#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "parser.h"
#include "httplib.h"
#include "json.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>


using json = nlohmann::json;

// struct LCUInfo;

inline std::string base64(const std::string& in) {
    static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val=0, valb=-6;
    for (uint8_t c : in) {
        val = (val<<8) + c;
        valb += 8;
        while (valb>=0) {
            out.push_back(b64_table[(val>>valb)&0x3F]);
            valb-=6;
        }
    }
    if (valb>-6) out.push_back(b64_table[((val<<8)>>(valb+8))&0x3F]);
    while (out.size()%4) out.push_back('=');
    return out;
}

inline float getCSPerMin(const LCUInfo& lcuInfo, const std::string& playerName) {
    std::stringstream ss;
    ss << "riot:" << lcuInfo.password;
    std::string auth = base64(ss.str());

    httplib::SSLClient cli("127.0.0.1", lcuInfo.port);
    cli.enable_server_certificate_verification(false); // LCU uses self-signed cert

    httplib::Headers headers = { {"Authorization", "Basic " + auth} };
    auto res = cli.Get("/liveclientdata/allgamedata", headers);
    if (!res || res->status != 200) return 0.0f;

    auto j = json::parse(res->body);
    float gameTime = j["gameData"]["gameTime"];
    int cs = 0;
    for (auto& p : j["allPlayers"]) {
        if (p["summonerName"].get<std::string>() == playerName)
            cs = p["scores"]["creepScore"];
    }

    return cs / (gameTime / 60.0f);
}