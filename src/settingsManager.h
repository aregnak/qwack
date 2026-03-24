#pragma once

#include <fstream>
#include <filesystem>
#include <shlobj.h>

#include "json.hpp"
#include "log.h"

class SettingsManager
{
public:
    // Delete copy constructor & assignment operator
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    static SettingsManager& Get()
    {
        static SettingsManager instance;
        return instance;
    }

    nlohmann::json getSettings();

private:
    SettingsManager();

    void handleAppDataFolder();
    nlohmann::json createSettingsJson();
    // TODO: make 1 loadJsonFile helper function to be used anywhere.
    std::string loadJsonFile(const std::string& path);

    std::string _settingsPath;
};