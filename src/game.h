#pragma once

#include <atomic>
#include <string>
#include <unordered_map>

enum class leagueState
{
    CLOSED,
    LOBBY,
    INGAME
};

// Update this list.
// Also maybe Game Mode isn't the best name? this is more like the state of the current
// game session you're in, spectator isn't a game mode, its a state...
enum class GameMode
{
    NONE,
    UNKNOWN,
    CLASSIC,
    SWIFTPLAY,
    ARAM,
    KIWI,
    PRACTICETOOL,
    SPECTATOR,
    TUTORIAL
};

inline const std::unordered_map<GameMode, std::string> toString = {

    { GameMode::NONE, "Not in game." },
    { GameMode::CLASSIC, "Classic (draft/ranked)" },
    { GameMode::SWIFTPLAY, "Swiftplay" },
    { GameMode::ARAM, "ARAM" },
    { GameMode::KIWI, "ARAM: Mayhem" },
    { GameMode::PRACTICETOOL, "Practice tool" },
    { GameMode::TUTORIAL, "Tutorial" },
    { GameMode::UNKNOWN, "Unknown from map" },
    { GameMode::SPECTATOR, "Viewing replay (Spectator mode)" }
};

inline std::string gameModeToString(std::atomic<GameMode>& gameMode)
{
    auto it = toString.find(gameMode.load());
    if (it != toString.end())
    {
        return it->second;
    }

    return "UNKNOWN";
}
