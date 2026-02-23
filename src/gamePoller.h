#pragma once

#include <filesystem>
#include <string>
#include <thread>

#include "httplib.h"

#include "game.h"
#include "lcuClient.h"
#include "parser.h"
#include "poll.h"
#include "debugPrints.h"

namespace lcuPoller
{

void handleClosedState(LCUClient& lcuC, poll& poller, std::atomic<gameState>& gameState,
                       std::atomic<bool>& running, std::string&, bool& printedWaitingForClient);
void handleLobbyState();
void handleInGameState();

void connectToLCU(LCUInfo&);
void getPlayerName(std::atomic<gameState>&, LCUClient&, poll&, std::string&);
void getSessionPlayers(std::vector<PlayerInfo>&, std::vector<std::string>&, poll&, LCUClient&);

} // namespace lcuPoller