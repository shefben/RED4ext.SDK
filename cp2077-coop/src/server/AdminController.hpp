#pragma once
#include "../net/Connection.hpp"
#include <cstdint>

namespace CoopNet {

enum class AdminCmdType : uint8_t { Kick = 0, Ban = 1, Mute = 2 };

void AdminController_Start();
void AdminController_Stop();
void AdminController_PollCommands();
bool AdminController_IsBanned(uint32_t peerId);
void AdminController_Ban(uint32_t peerId);
void AdminController_AddKickVote(uint32_t voterId, uint32_t targetId);

} // namespace CoopNet
