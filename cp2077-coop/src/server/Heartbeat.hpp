#pragma once
#include <string>
namespace CoopNet {
void Heartbeat_Send(const std::string& sessionJson);
void Heartbeat_Announce(const std::string& json);
}
