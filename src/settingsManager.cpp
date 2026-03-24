#include "settingsManager.h"

#include <sstream>

nlohmann::json SettingsManager::getSettings()
{
    auto body = loadJsonFile(_settingsPath);
    if (body.empty())
    {
        LCU_LOG("JSON file is empty or missing.");
        return nlohmann::json{};
    }

    auto settingsFile = nlohmann::json::parse(body, nullptr, false);
    if (settingsFile.is_discarded())
    {
        LCU_LOG("Settings JSON parse failed.");
        return nlohmann::json{};
    }

    return settingsFile;
}

SettingsManager::SettingsManager()
{
    handleAppDataFolder();
    //
}

void SettingsManager::handleAppDataFolder()
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
        _settingsPath = qwackPath.string() + "Settings.json";

        if (!std::filesystem::exists(_settingsPath))
        {
            WIN_LOG("Creating Settings.json");

            std::ofstream settingsFile(_settingsPath);
            settingsFile << createSettingsJson().dump(4);
        }
    }
    else
    {
        WIN_LOG("Failed to find AppData folder");
    }
}

// Settings.json template
nlohmann::json SettingsManager::createSettingsJson()
{
    nlohmann::json settings;

    settings["OverlaySettings"] = { { "ShowCSPM", true },
                                    { "ShowRanks", true },
                                    { "ShowGoldDiff", true } };

    return settings;
}

std::string SettingsManager::loadJsonFile(const std::string& path)
{
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}