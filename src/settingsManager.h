#pragma once

#include <fstream>
#include <filesystem>
#include <shlobj.h>

#include "json.hpp"
#include "log.h"

// Settings.json template
nlohmann::json createSettingsJson()
{
    nlohmann::json settings;

    settings["OverlaySettings"] = { { "ShowCSPM", true },
                                    { "ShowRanks", true },
                                    { "ShowGoldDiff", true } };

    return settings;
}

void handleAppdataFolder()
{
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path)))
    {
        std::wstring wpath(path);
        CoTaskMemFree(path);

        std::string folder(wpath.begin(), wpath.end());
        folder += "\\Qwack\\";

        std::filesystem::path qwackPath(folder);

        if (!std::filesystem::exists(qwackPath))
        {
            WIN_LOG("Qwack settings folder does not exist.");

            if (!std::filesystem::create_directories(qwackPath))
            {
                WIN_LOG("Created Qwack settings folder.");
            }
            else
            {
                WIN_LOG("Failed to create Qwack settings folder.");
            }
        }

        // Check and create Settings.json
        std::string settingsPath = qwackPath.string() + "Settings.json";

        if (!std::filesystem::exists(settingsPath))
        {
            WIN_LOG("Creating Settings.json");

            std::ofstream settingsFile(settingsPath);
            settingsFile << createSettingsJson().dump(4);
        }
    }
    else
    {
        WIN_LOG("Failed to find AppData folder");
    }
}
