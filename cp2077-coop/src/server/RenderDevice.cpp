#include "RenderDevice.hpp"
#include <RED4ext/Relocation.hpp>
#include <RED4ext/Detail/AddressHashes.hpp>

namespace CoopNet {

float RenderDevice_GetVRAMUsage()
{
    using func_t = float (*)();
    static RED4ext::RelocFunc<func_t> func(RED4ext::Detail::AddressHashes::RenderDevice_GetVRAMUsage);
    return func();
}

float RenderDevice_GetVRAMBudget()
{
    using func_t = float (*)();
    static RED4ext::RelocFunc<func_t> func(RED4ext::Detail::AddressHashes::RenderDevice_GetVRAMBudget);
    return func();
}

void TextureSystem_SetGlobalMipBias(int bias)
{
    using func_t = void (*)(int);
    static RED4ext::RelocFunc<func_t> func(RED4ext::Detail::AddressHashes::TextureSystem_SetGlobalMipBias);
    func(bias);
}

} // namespace CoopNet
