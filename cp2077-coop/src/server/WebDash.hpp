#pragma once
#include <cstdint>

namespace CoopNet {
void WebDash_Start();
void WebDash_Stop();
void WebDash_PushEvent(const std::string& json);
}
