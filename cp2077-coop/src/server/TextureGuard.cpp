#include "TextureGuard.hpp"
#include "RenderDevice.hpp"
#include "../net/Net.hpp"
#include <iostream>
#include <mutex>

namespace CoopNet
{

static float g_checkTimer = 0.f;
static float g_highTimer = 0.f;
static float g_lowTimer = 0.f;
static uint8_t g_bias = 0;
static std::mutex g_texMutex;


void TextureGuard_Tick(float dt)
{
    std::lock_guard lock(g_texMutex);
    g_checkTimer += dt;
    if (g_checkTimer < 30.f)
        return;
    g_checkTimer = 0.f;

    float usage = RenderDevice_GetVRAMUsage();
    float budget = RenderDevice_GetVRAMBudget();
    if (budget <= 0.f)
        return;
    float ratio = usage / budget;
    if (ratio > 0.9f)
    {
        g_highTimer += 30.f;
        g_lowTimer = 0.f;
        if (g_highTimer > 60.f && g_bias < 3)
        {
            g_bias += 1;
            TextureSystem_SetGlobalMipBias(g_bias);
            Net_BroadcastTextureBiasChange(g_bias);
            std::cerr << "[MemGuard] MipBias +" << int(g_bias) << std::endl;
        }
    }
    else if (ratio < 0.75f)
    {
        g_lowTimer += 30.f;
        g_highTimer = 0.f;
        if (g_lowTimer > 120.f && g_bias > 0)
        {
            g_bias -= 1;
            TextureSystem_SetGlobalMipBias(g_bias);
            Net_BroadcastTextureBiasChange(g_bias);
            std::cerr << "[MemGuard] MipBias -1 -> " << int(g_bias) << std::endl;
        }
    }
    else
    {
        g_highTimer = 0.f;
        g_lowTimer = 0.f;
    }
}

} // namespace CoopNet
