#pragma once
#include <cstdint>
#include <string>

namespace CoopNet {
void WebDash_Start();
void WebDash_Stop();
void WebDash_PushEvent(const std::string& json);
}
