#pragma once
#include "../net/Connection.hpp"
#include <cstdint>

namespace CoopNet {

enum class AdminCmdType : uint8_t { Kick = 0, Ban = 1, Mute = 2 };

void AdminController_Start();
void AdminController_Stop();
void AdminController_PollCommands();
bool AdminController_IsBanned(uint32_t peerId);
void AdminController_Kick(uint32_t peerId);
void AdminController_Ban(uint32_t peerId);
void AdminController_Mute(uint32_t peerId, uint32_t mins);
void AdminController_Unmute(uint32_t peerId);

} // namespace CoopNet
