#include "ChatFilter.hpp"
#include <algorithm>
#include <cctype>
#include <string_view>

namespace CoopNet {

static const char* g_badWords[] = {"badword1", "badword2", "badword3", nullptr};

bool ChatFilter_IsAllowed(const std::string& text)
{
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    for (const char** w = g_badWords; *w; ++w)
    {
        if (lower.find(*w) != std::string::npos)
            return false;
    }
    return true;
}

} // namespace CoopNet
