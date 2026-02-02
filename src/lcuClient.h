#pragma once

#include "parser.h"
#include "httplib.h"

class LCUClient
{
public:
    explicit LCUClient(const LCUInfo& lcu)
        : _client("127.0.0.1", lcu.port)
    {
        _client.enable_server_certificate_verification(false);

        std::stringstream ss;
        ss << "riot:" << lcu.password;
        _auth = base64(ss.str());

        _header = { { "Authorization", "Basic " + _auth } };
    }

    httplib::Result get(const std::string& path)
    {
        return _client.Get(path.c_str(), _header);
        //
    }

private:
    httplib::SSLClient _client;
    httplib::Headers _header;
    std::string _auth;
};