#pragma once

#include <fstream>
#include <filesystem>
#include <shlobj.h>

#include "json.hpp"
#include "log.h"
#include "util.h"

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
    void setShowCSPM(bool state);
    void setShowRanks(bool state);
    void setShowGoldDiff(bool state);

    void setOpenOnStart(bool state);

private:
    SettingsManager();

    void handleAppDataFolder();
    nlohmann::json createSettingsJson();

    bool saveSettings(const nlohmann::json& settings);
    void setOverlaySetting(const char* key, bool state);
    void setMenuSetting(const char* key, bool state);

    std::string _settingsPath;
};