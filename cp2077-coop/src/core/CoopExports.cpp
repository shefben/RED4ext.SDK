#include "HttpClient.hpp"
#include <memory>
#include "GameProcess.hpp"
#include "../net/Net.hpp" // FIX: expose networking helpers
#include "../voice/VoiceEncoder.hpp"
#include "../voice/VoiceDecoder.hpp"
#include "Version.hpp"
#include "../runtime/InventoryController.hpp"
#include "Logger.hpp" // Add logger support
#include "SessionState.hpp" // Add SessionState functions
#include <RED4ext/RED4ext.hpp>

using CoopNet::HttpResponse;
using CoopNet::HttpAsyncResult;

static void HttpGetFn(RED4ext::IScriptable* aCtx, RED4ext::CStackFrame* aFrame, HttpResponse* aOut, int64_t a4)
{
    RED4ext::CString url;
    RED4ext::GetParameter(aFrame, &url);
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::Http_Get(url.c_str());
}

static void HttpPostFn(RED4ext::IScriptable* aCtx, RED4ext::CStackFrame* aFrame, HttpResponse* aOut, int64_t a4)
{
    RED4ext::CString url, body, mime;
    RED4ext::GetParameter(aFrame, &url);
    RED4ext::GetParameter(aFrame, &body);
    RED4ext::GetParameter(aFrame, &mime);
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::Http_Post(url.c_str(), body.c_str(), mime.c_str());
}

static void HttpGetAsyncFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, uint32_t* aOut, int64_t)
{
    RED4ext::CString url;
    RED4ext::GetParameter(aFrame, &url);
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::Http_GetAsync(url.c_str(), 5000, 1);
}

static void HttpPollAsyncFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, CoopNet::HttpAsyncResult* aOut, int64_t)
{
    aFrame->code++;
    CoopNet::HttpAsyncResult res{};
    if (CoopNet::Http_PollAsync(res)) {
        if (aOut) *aOut = res;
    } else if (aOut) {
        aOut->token = 0;
        aOut->resp = {0, {}};
    }
}

static void LaunchFn(RED4ext::IScriptable* aCtx, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4ext::CString exe, args;
    RED4ext::GetParameter(aFrame, &exe);
    RED4ext::GetParameter(aFrame, &args);
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::GameProcess_Launch(exe.c_str(), args.c_str());
}

static void NetIsConnectedFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t)
{
    aFrame->code++;
    if (aOut)
        *aOut = Net_IsConnected();
}

static void NetSendJoinRequestFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t)
{
    uint32_t serverId = 0;
    RED4ext::GetParameter(aFrame, &serverId);
    aFrame->code++;
    Net_SendJoinRequest(serverId);
}

static void NetPollFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t)
{
    uint32_t maxMs = 0;
    RED4ext::GetParameter(aFrame, &maxMs);
    aFrame->code++;
    Net_Poll(maxMs);
}

static void SessionActiveCountFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, uint32_t* aOut, int64_t)
{
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::SessionState_GetActivePlayerCount();
}

// === NEW NETWORKING BRIDGE FUNCTIONS ===

static void NetInitializeFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t)
{
    aFrame->code++;
    Net_Init();
    if (aOut)
        *aOut = true;
}

static void NetConnectToServerFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t)
{
    RED4ext::CString host;
    uint32_t port = 7777;
    RED4ext::GetParameter(aFrame, &host);
    RED4ext::GetParameter(aFrame, &port);
    aFrame->code++;
    
    bool result = Net_ConnectToServer(host.c_str(), port);
    if (aOut)
        *aOut = result;
}

static void VoiceStartFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t)
{
    RED4ext::CString dev;
    uint32_t sr = 48000;
    uint32_t br = 24000;
    bool opus = true;
    RED4ext::GetParameter(aFrame, &dev);
    RED4ext::GetParameter(aFrame, &sr);
    RED4ext::GetParameter(aFrame, &br);
    RED4ext::GetParameter(aFrame, &opus);
    aFrame->code++;
    if (aOut)
        *aOut = CoopVoice::StartCapture(dev.c_str(), sr, br, opus ? CoopVoice::Codec::Opus : CoopVoice::Codec::PCM);
}

static void VoiceEncodeFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, int32_t* aOut, int64_t)
{
    int16_t* pcm = nullptr;
    uint8_t* buf = nullptr;
    RED4ext::GetParameter(aFrame, &pcm);
    RED4ext::GetParameter(aFrame, &buf);
    aFrame->code++;
    if (aOut)
        *aOut = CoopVoice::EncodeFrame(pcm, buf, CoopVoice::GetFrameBytes());
}

static void VoiceStopFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t)
{
    aFrame->code++;
    CoopVoice::StopCapture();
}

static void VoiceSetVolumeFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t)
{
    float vol = 1.f;
    RED4ext::GetParameter(aFrame, &vol);
    aFrame->code++;
    CoopVoice::SetCaptureVolume(vol);
    CoopVoice::SetPlaybackVolume(vol);
}

static void VoiceSetCodecFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t)
{
    bool opus = true;
    RED4ext::GetParameter(aFrame, &opus);
    aFrame->code++;
    CoopVoice::SetEncoderCodec(opus ? CoopVoice::Codec::Opus : CoopVoice::Codec::PCM);
    CoopVoice::SetDecoderCodec(opus ? CoopVoice::Codec::Opus : CoopVoice::Codec::PCM);
}

// Version check native functions
static void VersionGetBuildCRCFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, uint32_t* aOut, int64_t)
{
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::GetBuildCRC();
}

static void VersionValidateRemoteFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t)
{
    uint32_t remoteCRC = 0;
    RED4ext::GetParameter(aFrame, &remoteCRC);
    aFrame->code++;
    if (aOut)
        *aOut = CoopNet::ValidateRemoteVersion(remoteCRC);
}

static void VersionGetStringFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, RED4ext::CString* aOut, int64_t)
{
    aFrame->code++;
    if (aOut)
    {
        std::string versionStr = CoopNet::Version::Current().ToString();
        *aOut = versionStr.c_str();
    }
}

// Inventory native functions
static void InventorySyncPlayerInventoryFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t)
{
    uint32_t peerId = 0;
    uint32_t version = 0;
    uint64_t money = 0;
    RED4ext::GetParameter(aFrame, &peerId);
    RED4ext::GetParameter(aFrame, &version);
    RED4ext::GetParameter(aFrame, &money);
    // Note: Items array would need custom serialization in a real implementation
    aFrame->code++;
    
    if (aOut) {
        CoopNet::PlayerInventorySnap snap;
        snap.peerId = peerId;
        snap.version = version;
        snap.money = money;
        snap.items.clear(); // Simplified for demo
        *aOut = CoopNet::InventoryController::Instance().UpdatePlayerInventory(snap);
    }
}

static void InventoryRequestTransferFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, uint32_t* aOut, int64_t)
{
    uint32_t fromPeer = 0, toPeer = 0, quantity = 0;
    uint64_t itemId = 0;
    RED4ext::GetParameter(aFrame, &fromPeer);
    RED4ext::GetParameter(aFrame, &toPeer);
    RED4ext::GetParameter(aFrame, &itemId);
    RED4ext::GetParameter(aFrame, &quantity);
    aFrame->code++;
    
    if (aOut) {
        *aOut = CoopNet::InventoryController::Instance().RequestItemTransfer(fromPeer, toPeer, itemId, quantity);
    }
}

static void InventoryRegisterPickupFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t)
{
    uint64_t itemId = 0;
    uint32_t playerId = 0;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    RED4ext::GetParameter(aFrame, &itemId);
    RED4ext::GetParameter(aFrame, &posX);
    RED4ext::GetParameter(aFrame, &posY);
    RED4ext::GetParameter(aFrame, &posZ);
    RED4ext::GetParameter(aFrame, &playerId);
    aFrame->code++;
    
    if (aOut) {
        float worldPos[3] = {posX, posY, posZ};
        *aOut = CoopNet::InventoryController::Instance().RegisterWorldItemPickup(itemId, worldPos, playerId);
    }
}

static void InventoryIsItemTakenFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t)
{
    uint64_t itemId = 0;
    RED4ext::GetParameter(aFrame, &itemId);
    aFrame->code++;
    
    if (aOut) {
        *aOut = CoopNet::InventoryController::Instance().IsWorldItemTaken(itemId);
    }
}

static void InventoryProcessTransferFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t)
{
    uint32_t requestId = 0;
    bool approve = false;
    RED4ext::CString reason;
    RED4ext::GetParameter(aFrame, &requestId);
    RED4ext::GetParameter(aFrame, &approve);
    RED4ext::GetParameter(aFrame, &reason);
    aFrame->code++;
    
    if (aOut) {
        *aOut = CoopNet::InventoryController::Instance().ProcessTransferRequest(requestId, approve, reason.c_str());
    }
}

static void InventoryGetPlayerCountFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, uint32_t* aOut, int64_t)
{
    aFrame->code++;
    if (aOut) {
        *aOut = static_cast<uint32_t>(CoopNet::InventoryController::Instance().GetPlayerCount());
    }
}

static void InventoryCleanupFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t)
{
    aFrame->code++;
    CoopNet::InventoryController::Instance().CleanupExpiredRequests();
    CoopNet::InventoryController::Instance().ClearExpiredPickups();
}

static RED4ext::TTypedClass<CoopNet::HttpResponse> g_httpRespCls("HttpResponse");
static RED4ext::TTypedClass<CoopNet::HttpAsyncResult> g_httpAsyncCls("HttpAsyncResult");

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

    g_httpAsyncCls.flags = {.isNative = true};
    auto u32 = rtti->GetType("Uint32");
    auto respType = &g_httpRespCls;
    auto tokProp = RED4ext::CProperty::Create(u32, "token", &g_httpAsyncCls,
                                             offsetof(HttpAsyncResult, token), nullptr, {.isPublic = true});
    auto respProp = RED4ext::CProperty::Create(respType, "resp", &g_httpAsyncCls,
                                              offsetof(HttpAsyncResult, resp), nullptr, {.isPublic = true});
    g_httpAsyncCls.props.EmplaceBack(tokProp);
    g_httpAsyncCls.props.EmplaceBack(respProp);
    g_httpAsyncCls.size = sizeof(HttpAsyncResult);
    rtti->RegisterType(&g_httpAsyncCls);
}

// Forward declarations for functions defined later
static void NetSendInventorySnapshotFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t);
static void NetSendItemTransferRequestFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t);
static void NetSendItemPickupFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t);
static void InventorySyncUpdatePlayerInventoryFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t);
static void InventorySyncRequestTransferFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, uint32_t* aOut, int64_t);
static void InventorySyncRegisterPickupFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t);
static void InventorySyncIsItemTakenFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t);
static void InventorySyncProcessTransferFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t);
static void InventorySyncGetPlayerCountFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, uint32_t* aOut, int64_t);
static void InventorySyncCleanupFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t);

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

    auto ga = RED4ext::CGlobalFunction::Create("HttpRequest_HttpGetAsync", "HttpRequest_HttpGetAsync", &HttpGetAsyncFn);
    ga->flags = flags;
    ga->AddParam("String", "url");
    ga->SetReturnType("Uint32");
    rtti->RegisterFunction(ga);

    auto pa = RED4ext::CGlobalFunction::Create("HttpRequest_PollAsync", "HttpRequest_PollAsync", &HttpPollAsyncFn);
    pa->flags = flags;
    pa->SetReturnType("HttpAsyncResult");
    rtti->RegisterFunction(pa);

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

    auto ap = RED4ext::CGlobalFunction::Create("SessionState_GetActivePlayerCount", "SessionState_GetActivePlayerCount", &SessionActiveCountFn);
    ap->flags = flags;
    ap->SetReturnType("Uint32");
    rtti->RegisterFunction(ap);

    auto vs = RED4ext::CGlobalFunction::Create("CoopVoice_StartCapture", "CoopVoice_StartCapture", &VoiceStartFn);
    vs->flags = flags;
    vs->AddParam("String", "device");
    vs->AddParam("Uint32", "sampleRate");
    vs->AddParam("Uint32", "bitrate");
    vs->AddParam("Bool", "opus");
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

    auto vvol = RED4ext::CGlobalFunction::Create("CoopVoice_SetVolume", "CoopVoice_SetVolume", &VoiceSetVolumeFn);
    vvol->flags = flags;
    vvol->AddParam("Float", "volume");
    rtti->RegisterFunction(vvol);

    auto vc = RED4ext::CGlobalFunction::Create("CoopVoice_SetCodec", "CoopVoice_SetCodec", &VoiceSetCodecFn);
    vc->flags = flags;
    vc->AddParam("Bool", "opus");
    rtti->RegisterFunction(vc);

    // Register version check functions
    auto vbcrc = RED4ext::CGlobalFunction::Create("VersionCheck_GetBuildCRC", "VersionCheck_GetBuildCRC", &VersionGetBuildCRCFn);
    vbcrc->flags = flags;
    vbcrc->SetReturnType("Uint32");
    rtti->RegisterFunction(vbcrc);

    auto vvalid = RED4ext::CGlobalFunction::Create("VersionCheck_ValidateRemoteVersion", "VersionCheck_ValidateRemoteVersion", &VersionValidateRemoteFn);
    vvalid->flags = flags;
    vvalid->AddParam("Uint32", "remoteCRC");
    vvalid->SetReturnType("Bool");
    rtti->RegisterFunction(vvalid);

    auto vstr = RED4ext::CGlobalFunction::Create("VersionCheck_GetVersionString", "VersionCheck_GetVersionString", &VersionGetStringFn);
    vstr->flags = flags;
    vstr->SetReturnType("String");
    rtti->RegisterFunction(vstr);

    // Register inventory system functions
    auto invSync = RED4ext::CGlobalFunction::Create("InventorySync_UpdatePlayerInventory", "InventorySync_UpdatePlayerInventory", &InventorySyncPlayerInventoryFn);
    invSync->flags = flags;
    invSync->AddParam("Uint32", "peerId");
    invSync->AddParam("Uint32", "version");
    invSync->AddParam("Uint64", "money");
    invSync->SetReturnType("Bool");
    rtti->RegisterFunction(invSync);

    auto invTransfer = RED4ext::CGlobalFunction::Create("InventorySync_RequestTransfer", "InventorySync_RequestTransfer", &InventoryRequestTransferFn);
    invTransfer->flags = flags;
    invTransfer->AddParam("Uint32", "fromPeer");
    invTransfer->AddParam("Uint32", "toPeer");
    invTransfer->AddParam("Uint64", "itemId");
    invTransfer->AddParam("Uint32", "quantity");
    invTransfer->SetReturnType("Uint32");
    rtti->RegisterFunction(invTransfer);

    auto invPickup = RED4ext::CGlobalFunction::Create("InventorySync_RegisterPickup", "InventorySync_RegisterPickup", &InventoryRegisterPickupFn);
    invPickup->flags = flags;
    invPickup->AddParam("Uint64", "itemId");
    invPickup->AddParam("Float", "posX");
    invPickup->AddParam("Float", "posY");
    invPickup->AddParam("Float", "posZ");
    invPickup->AddParam("Uint32", "playerId");
    invPickup->SetReturnType("Bool");
    rtti->RegisterFunction(invPickup);

    auto invTaken = RED4ext::CGlobalFunction::Create("InventorySync_IsItemTaken", "InventorySync_IsItemTaken", &InventoryIsItemTakenFn);
    invTaken->flags = flags;
    invTaken->AddParam("Uint64", "itemId");
    invTaken->SetReturnType("Bool");
    rtti->RegisterFunction(invTaken);

    auto invProcess = RED4ext::CGlobalFunction::Create("InventorySync_ProcessTransfer", "InventorySync_ProcessTransfer", &InventoryProcessTransferFn);
    invProcess->flags = flags;
    invProcess->AddParam("Uint32", "requestId");
    invProcess->AddParam("Bool", "approve");
    invProcess->AddParam("String", "reason");
    invProcess->SetReturnType("Bool");
    rtti->RegisterFunction(invProcess);

    auto invCount = RED4ext::CGlobalFunction::Create("InventorySync_GetPlayerCount", "InventorySync_GetPlayerCount", &InventoryGetPlayerCountFn);
    invCount->flags = flags;
    invCount->SetReturnType("Uint32");
    rtti->RegisterFunction(invCount);

    auto invCleanup = RED4ext::CGlobalFunction::Create("InventorySync_Cleanup", "InventorySync_Cleanup", &InventoryCleanupFn);
    invCleanup->flags = flags;
    rtti->RegisterFunction(invCleanup);
    
    // === NEW NETWORKING FUNCTION REGISTRATIONS ===
    
    auto netInit = RED4ext::CGlobalFunction::Create("Net_Initialize", "Net_Initialize", &NetInitializeFn);
    netInit->flags = flags;
    netInit->SetReturnType("Bool");
    rtti->RegisterFunction(netInit);
    
    auto netConnect = RED4ext::CGlobalFunction::Create("Net_ConnectToServer", "Net_ConnectToServer", &NetConnectToServerFn);
    netConnect->flags = flags;
    netConnect->AddParam("String", "host");
    netConnect->AddParam("Uint32", "port");
    netConnect->SetReturnType("Bool");
    rtti->RegisterFunction(netConnect);

    // === Register Inventory Sync Functions ===
    
    auto netSendInv = RED4ext::CGlobalFunction::Create("Net_SendInventorySnapshot", "Net_SendInventorySnapshot", &NetSendInventorySnapshotFn);
    netSendInv->flags = flags;
    rtti->RegisterFunction(netSendInv);

    auto netSendTransfer = RED4ext::CGlobalFunction::Create("Net_SendItemTransferRequest", "Net_SendItemTransferRequest", &NetSendItemTransferRequestFn);
    netSendTransfer->flags = flags;
    rtti->RegisterFunction(netSendTransfer);

    auto netSendPickup = RED4ext::CGlobalFunction::Create("Net_SendItemPickup", "Net_SendItemPickup", &NetSendItemPickupFn);
    netSendPickup->flags = flags;
    rtti->RegisterFunction(netSendPickup);

    auto invSyncUpdate = RED4ext::CGlobalFunction::Create("InventorySync_UpdatePlayerInventory", "InventorySync_UpdatePlayerInventory", &InventorySyncUpdatePlayerInventoryFn);
    invSyncUpdate->flags = flags;
    invSyncUpdate->AddParam("Uint32", "peerId");
    invSyncUpdate->AddParam("Uint32", "version");
    invSyncUpdate->AddParam("Uint64", "money");
    invSyncUpdate->SetReturnType("Bool");
    rtti->RegisterFunction(invSyncUpdate);

    auto invSyncTransfer = RED4ext::CGlobalFunction::Create("InventorySync_RequestTransfer", "InventorySync_RequestTransfer", &InventorySyncRequestTransferFn);
    invSyncTransfer->flags = flags;
    invSyncTransfer->AddParam("Uint32", "fromPeer");
    invSyncTransfer->AddParam("Uint32", "toPeer");
    invSyncTransfer->AddParam("Uint64", "itemId");
    invSyncTransfer->AddParam("Uint32", "quantity");
    invSyncTransfer->SetReturnType("Uint32");
    rtti->RegisterFunction(invSyncTransfer);

    auto invSyncPickup = RED4ext::CGlobalFunction::Create("InventorySync_RegisterPickup", "InventorySync_RegisterPickup", &InventorySyncRegisterPickupFn);
    invSyncPickup->flags = flags;
    invSyncPickup->AddParam("Uint64", "itemId");
    invSyncPickup->AddParam("Float", "posX");
    invSyncPickup->AddParam("Float", "posY");
    invSyncPickup->AddParam("Float", "posZ");
    invSyncPickup->AddParam("Uint32", "playerId");
    invSyncPickup->SetReturnType("Bool");
    rtti->RegisterFunction(invSyncPickup);

    auto invSyncTaken = RED4ext::CGlobalFunction::Create("InventorySync_IsItemTaken", "InventorySync_IsItemTaken", &InventorySyncIsItemTakenFn);
    invSyncTaken->flags = flags;
    invSyncTaken->AddParam("Uint64", "itemId");
    invSyncTaken->SetReturnType("Bool");
    rtti->RegisterFunction(invSyncTaken);

    auto invSyncProcess = RED4ext::CGlobalFunction::Create("InventorySync_ProcessTransfer", "InventorySync_ProcessTransfer", &InventorySyncProcessTransferFn);
    invSyncProcess->flags = flags;
    invSyncProcess->AddParam("Uint32", "requestId");
    invSyncProcess->AddParam("Bool", "approve");
    invSyncProcess->AddParam("String", "reason");
    invSyncProcess->SetReturnType("Bool");
    rtti->RegisterFunction(invSyncProcess);

    auto invSyncCount = RED4ext::CGlobalFunction::Create("InventorySync_GetPlayerCount", "InventorySync_GetPlayerCount", &InventorySyncGetPlayerCountFn);
    invSyncCount->flags = flags;
    invSyncCount->SetReturnType("Uint32");
    rtti->RegisterFunction(invSyncCount);

    auto invSyncCleanup = RED4ext::CGlobalFunction::Create("InventorySync_Cleanup", "InventorySync_Cleanup", &InventorySyncCleanupFn);
    invSyncCleanup->flags = flags;
    rtti->RegisterFunction(invSyncCleanup);
}

// === Inventory Sync Native Functions ===

static void NetSendInventorySnapshotFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t) {
    // PlayerInventorySnap parameter - would need proper struct handling
    aFrame->code++; // Skip parameters for now
    CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "Net_SendInventorySnapshot called (placeholder)");
}

static void NetSendItemTransferRequestFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t) {
    // ItemTransferRequest parameter - would need proper struct handling  
    aFrame->code++; // Skip parameters for now
    CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "Net_SendItemTransferRequest called (placeholder)");
}

static void NetSendItemPickupFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t) {
    // ItemPickupEvent parameter - would need proper struct handling
    aFrame->code++; // Skip parameters for now
    CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "Net_SendItemPickup called (placeholder)");
}

static void InventorySyncUpdatePlayerInventoryFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t) {
    uint32_t peerId;
    uint32_t version;
    uint64_t money;
    
    RED4ext::GetParameter(aFrame, &peerId);
    RED4ext::GetParameter(aFrame, &version);
    RED4ext::GetParameter(aFrame, &money);
    aFrame->code++;
    
    CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "InventorySync_UpdatePlayerInventory: peer=" + std::to_string(peerId));
    
    if (aOut) *aOut = true; // Always succeed for testing
}

static void InventorySyncRequestTransferFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, uint32_t* aOut, int64_t) {
    uint32_t fromPeer;
    uint32_t toPeer;
    uint64_t itemId;
    uint32_t quantity;
    
    RED4ext::GetParameter(aFrame, &fromPeer);
    RED4ext::GetParameter(aFrame, &toPeer);
    RED4ext::GetParameter(aFrame, &itemId);
    RED4ext::GetParameter(aFrame, &quantity);
    aFrame->code++;
    
    static uint32_t nextRequestId = 1;
    uint32_t requestId = nextRequestId++;
    
    CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "InventorySync_RequestTransfer: from=" + std::to_string(fromPeer) + " to=" + std::to_string(toPeer));
    
    if (aOut) *aOut = requestId;
}

static void InventorySyncRegisterPickupFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t) {
    uint64_t itemId;
    float posX, posY, posZ;
    uint32_t playerId;
    
    RED4ext::GetParameter(aFrame, &itemId);
    RED4ext::GetParameter(aFrame, &posX);
    RED4ext::GetParameter(aFrame, &posY);
    RED4ext::GetParameter(aFrame, &posZ);
    RED4ext::GetParameter(aFrame, &playerId);
    aFrame->code++;
    
    CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "InventorySync_RegisterPickup: item=" + std::to_string(itemId) + " player=" + std::to_string(playerId));
    
    if (aOut) *aOut = true;
}

static void InventorySyncIsItemTakenFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t) {
    uint64_t itemId;
    RED4ext::GetParameter(aFrame, &itemId);
    aFrame->code++;
    
    // For testing, always return false (item not taken)
    if (aOut) *aOut = false;
}

static void InventorySyncProcessTransferFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t) {
    uint32_t requestId;
    bool approve;
    RED4ext::CString reason;
    
    RED4ext::GetParameter(aFrame, &requestId);
    RED4ext::GetParameter(aFrame, &approve);
    RED4ext::GetParameter(aFrame, &reason);
    aFrame->code++;
    
    CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "InventorySync_ProcessTransfer: req=" + std::to_string(requestId) + " approve=" + (approve ? "true" : "false"));
    
    if (aOut) *aOut = true;
}

static void InventorySyncGetPlayerCountFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, uint32_t* aOut, int64_t) {
    aFrame->code++;
    if (aOut) *aOut = 1; // Always return 1 for testing (just local player)
}

static void InventorySyncCleanupFn(RED4ext::IScriptable*, RED4ext::CStackFrame* aFrame, void*, int64_t) {
    aFrame->code++;
    CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "InventorySync_Cleanup called");
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason, const RED4ext::Sdk* aSdk)
{
    RED4EXT_UNUSED_PARAMETER(aHandle);
    RED4EXT_UNUSED_PARAMETER(aSdk);
    switch (aReason)
    {
    case RED4ext::EMainReason::Load:
    {
        // Initialize logger first
        CoopNet::Logger::Initialize();
        CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "CP2077 Coop mod loading...");
        
        auto rtti = RED4ext::CRTTISystem::Get();
        rtti->AddRegisterCallback(RegisterTypes);
        rtti->AddPostRegisterCallback(PostRegisterTypes);
        
        // Initialize networking for client
        Net_Init();
        CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "Client networking initialized");
        
        break;
    }
    case RED4ext::EMainReason::Unload:
    {
        CoopNet::Logger::Log(CoopNet::LogLevel::INFO, "CP2077 Coop mod unloading...");
        Net_Shutdown();
        CoopNet::Logger::Shutdown();
        break;
    }
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
