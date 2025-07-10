#include "HttpClient.hpp"
#include <memory>
#include "GameProcess.hpp"
#include "../net/Net.hpp" // FIX: expose networking helpers
#include "../voice/VoiceEncoder.hpp"
#include <RED4ext/RED4ext.hpp>

using CoopNet::HttpResponse;

static void HttpGetFn(RED4ext::IScriptable* aCtx, RED4ext::CStackFrame* aFrame, HttpResponse* aOut, void* a4)
{
    RED4ext::CString url;
    RED4ext::GetParameter(aFrame, &url);
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::Http_Get(url.c_str());
}

static void HttpPostFn(RED4ext::IScriptable* aCtx, RED4ext::CStackFrame* aFrame, HttpResponse* aOut, void* a4)
{
    RED4ext::CString url, body, mime;
    RED4ext::GetParameter(aFrame, &url);
    RED4ext::GetParameter(aFrame, &body);
    RED4ext::GetParameter(aFrame, &mime);
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::Http_Post(url.c_str(), body.c_str(), mime.c_str());
}

static void LaunchFn(RED4ext::IScriptable* aCtx, RED4ext::CStackFrame* aFrame, bool* aOut, void* a4)
{
    RED4ext::CString exe, args;
    RED4ext::GetParameter(aFrame, &exe);
    RED4ext::GetParameter(aFrame, &args);
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::GameProcess_Launch(exe.c_str(), args.c_str());
}

static void NetIsConnectedFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, void*)
{
    aFrame->code++;
    if (aOut)
        *aOut = Net_IsConnected();
}

static void NetSendJoinRequestFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, void*)
{
    uint32_t serverId = 0;
    RED4ext::GetParameter(aFrame, &serverId);
    aFrame->code++;
    Net_SendJoinRequest(serverId);
}

static void NetPollFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, void*)
{
    uint32_t maxMs = 0;
    RED4ext::GetParameter(aFrame, &maxMs);
    aFrame->code++;
    Net_Poll(maxMs);
}

static void VoiceStartFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, void*)
{
    RED4ext::CString dev;
    uint32_t sr = 48000;
    uint32_t br = 24000;
    RED4ext::GetParameter(aFrame, &dev);
    RED4ext::GetParameter(aFrame, &sr);
    RED4ext::GetParameter(aFrame, &br);
    aFrame->code++;
    if (aOut)
        *aOut = CoopVoice::StartCapture(dev.c_str(), sr, br);
}

static void VoiceEncodeFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, int32_t* aOut, void*)
{
    int16_t* pcm = nullptr;
    uint8_t* buf = nullptr;
    RED4ext::GetParameter(aFrame, &pcm);
    RED4ext::GetParameter(aFrame, &buf);
    aFrame->code++;
    if (aOut)
        *aOut = CoopVoice::EncodeFrame(pcm, buf);
}

static void VoiceStopFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, void*)
{
    aFrame->code++;
    CoopVoice::StopCapture();
}

static RED4ext::TTypedClass<CoopNet::HttpResponse> g_httpRespCls("HttpResponse");

RED4EXT_C_EXPORT void RED4EXT_CALL RegisterTypes()
{
    g_httpRespCls.flags = {.isNative = true};
    auto rtti = RED4ext::CRTTISystem::Get();
    auto u16 = rtti->GetType("Uint16");
    auto str = rtti->GetType("String");
    using Flags = RED4ext::CProperty::Flags;
    auto statusProp = RED4ext::CProperty::Create(u16, "status", &g_httpRespCls,
                                                 offsetof(HttpResponse, status), nullptr, {.isPublic = true});
    auto bodyProp = RED4ext::CProperty::Create(str, "body", &g_httpRespCls,
                                               offsetof(HttpResponse, body), nullptr, {.isPublic = true});
    g_httpRespCls.props.EmplaceBack(statusProp);
    g_httpRespCls.props.EmplaceBack(bodyProp);
    g_httpRespCls.size = sizeof(HttpResponse);
    rtti->RegisterType(&g_httpRespCls);
}

RED4EXT_C_EXPORT void RED4EXT_CALL PostRegisterTypes()
{
    auto rtti = RED4ext::CRTTISystem::Get();
    RED4ext::CBaseFunction::Flags flags{.isNative = true, .isStatic = true};

    auto g = RED4ext::CGlobalFunction::Create("HttpRequest_HttpGet", "HttpRequest_HttpGet", &HttpGetFn);
    g->flags = flags;
    g->AddParam("String", "url");
    g->SetReturnType("HttpResponse");
    rtti->RegisterFunction(g);

    auto p = RED4ext::CGlobalFunction::Create("HttpRequest_HttpPost", "HttpRequest_HttpPost", &HttpPostFn);
    p->flags = flags;
    p->AddParam("String", "url");
    p->AddParam("String", "payload");
    p->AddParam("String", "mime");
    p->SetReturnType("HttpResponse");
    rtti->RegisterFunction(p);

    auto l = RED4ext::CGlobalFunction::Create("GameProcess_Launch", "GameProcess_Launch", &LaunchFn);
    l->flags = flags;
    l->AddParam("String", "exe");
    l->AddParam("String", "args");
    l->SetReturnType("Bool");
    rtti->RegisterFunction(l);

    auto isConn = RED4ext::CGlobalFunction::Create("Net_IsConnected", "Net_IsConnected", &NetIsConnectedFn);
    isConn->flags = flags;
    isConn->SetReturnType("Bool");
    rtti->RegisterFunction(isConn);

    auto join = RED4ext::CGlobalFunction::Create("Net_SendJoinRequest", "Net_SendJoinRequest", &NetSendJoinRequestFn);
    join->flags = flags;
    join->AddParam("Uint32", "serverId");
    rtti->RegisterFunction(join);

    auto poll = RED4ext::CGlobalFunction::Create("Net_Poll", "Net_Poll", &NetPollFn);
    poll->flags = flags;
    poll->AddParam("Uint32", "maxMs");
    rtti->RegisterFunction(poll);

    auto vs = RED4ext::CGlobalFunction::Create("CoopVoice_StartCapture", "CoopVoice_StartCapture", &VoiceStartFn);
    vs->flags = flags;
    vs->AddParam("String", "device");
    vs->AddParam("Uint32", "sampleRate");
    vs->AddParam("Uint32", "bitrate");
    vs->SetReturnType("Bool");
    rtti->RegisterFunction(vs);

    auto ve = RED4ext::CGlobalFunction::Create("CoopVoice_EncodeFrame", "CoopVoice_EncodeFrame", &VoiceEncodeFn);
    ve->flags = flags;
    ve->AddParam("script_ref<Int16>", "pcm");
    ve->AddParam("script_ref<Uint8>", "buf");
    ve->SetReturnType("Int32");
    rtti->RegisterFunction(ve);

    auto vstop = RED4ext::CGlobalFunction::Create("CoopVoice_StopCapture", "CoopVoice_StopCapture", &VoiceStopFn);
    vstop->flags = flags;
    rtti->RegisterFunction(vstop);
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason, const RED4ext::Sdk* aSdk)
{
    RED4EXT_UNUSED_PARAMETER(aHandle);
    RED4EXT_UNUSED_PARAMETER(aSdk);
    switch (aReason)
    {
    case RED4ext::EMainReason::Load:
    {
        auto rtti = RED4ext::CRTTISystem::Get();
        rtti->AddRegisterCallback(RegisterTypes);
        rtti->AddPostRegisterCallback(PostRegisterTypes);
        break;
    }
    case RED4ext::EMainReason::Unload:
        break;
    }
    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo* aInfo)
{
    aInfo->name = L"CoopExports";
    aInfo->author = L"Codex";
    aInfo->version = RED4EXT_SEMVER(1, 0, 0);
    aInfo->runtime = RED4EXT_RUNTIME_LATEST;
    aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_LATEST;
}
