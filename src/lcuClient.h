#pragma once

#include "parser.h"
#include "httplib.h"

class LCUClient
{
public:
    LCUClient() = default;

    void connect(const LCUInfo& lcu)
    {
        _client = std::make_unique<httplib::SSLClient>("127.0.0.1", lcu.port);
        _client.enable_server_certificate_verification(false);

        std::stringstream ss;
        ss << "riot:" << lcu.password;
        _auth = base64(ss.str());

        _header = { { "Authorization", "Basic " + _auth } };
    }

    const bool isConnected()
    {
        return _client != nullptr;
        //
    }

    httplib::Result get(const std::string& path)
    {
        return _client->Get(path.c_str(), _header);
        //
    }

private:
    // This is a unique pointer because SSLClient is non copyable.
    std::unique_ptr<httplib::SSLClient> _client;
    httplib::Headers _header;
    std::string _auth;
};