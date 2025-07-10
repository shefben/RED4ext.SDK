#pragma once
#include <string>
#include <cstdint>

namespace CoopNet {

// Returns true if the message was an admin command.
bool AdminCommandHandler_Handle(uint32_t senderId, const std::string& text);

} // namespace CoopNet
