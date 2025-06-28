#include "PythonVM.hpp"
#include <Python.h>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>
#include "PluginManager.hpp"
#include "../core/Hash.hpp"
#include "../net/Net.hpp"
#include "../net/Packets.hpp"
#include "../net/Snapshot.hpp"
#include "../server/VehicleController.hpp"

namespace CoopNet {
static bool g_init = false;
static uint32_t g_nextNpcId = 1000u;
struct Listener {
    PyObject* func;
    std::string plugin;
};
static std::unordered_map<std::string, std::vector<Listener>> g_listeners;

static PyObject* Py_RegisterEvent(PyObject*, PyObject* args)
{
    const char* name;
    PyObject* cb;
    if (!PyArg_ParseTuple(args, "sO", &name, &cb))
        return nullptr;
    if (!PyCallable_Check(cb))
    {
        PyErr_SetString(PyExc_TypeError, "callback not callable");
        return nullptr;
    }
    PyObject* globals = PyEval_GetGlobals();
    const char* modName = PyUnicode_AsUTF8(PyDict_GetItemString(globals, "__name__"));
    Py_INCREF(cb);
    g_listeners[name].push_back({cb, modName});
    Py_RETURN_NONE;
}

static PyObject* Py_RegisterCommand(PyObject*, PyObject* args)
{
    const char* name;
    const char* help;
    PyObject* cb;
    if (!PyArg_ParseTuple(args, "ssO", &name, &help, &cb))
        return nullptr;
    if (!PyCallable_Check(cb))
    {
        PyErr_SetString(PyExc_TypeError, "callback not callable");
        return nullptr;
    }
    PyObject* globals = PyEval_GetGlobals();
    const char* modName = PyUnicode_AsUTF8(PyDict_GetItemString(globals, "__name__"));
    PluginManager_RegisterCommand(name, help, cb, modName);
    Py_RETURN_NONE;
}

static PyObject* Py_SpawnNpc(PyObject*, PyObject* args)
{
    const char* tpl;
    PyObject* pos;
    int phase;
    if (!PyArg_ParseTuple(args, "sOi", &tpl, &pos, &phase))
        return nullptr;
    if (!PyTuple_Check(pos) || PyTuple_Size(pos) != 3)
    {
        PyErr_SetString(PyExc_TypeError, "pos must be tuple(x,y,z)");
        return nullptr;
    }
    double x = PyFloat_AsDouble(PyTuple_GetItem(pos, 0));
    double y = PyFloat_AsDouble(PyTuple_GetItem(pos, 1));
    double z = PyFloat_AsDouble(PyTuple_GetItem(pos, 2));
    NpcSnap snap{};
    snap.npcId = g_nextNpcId++;
    snap.templateId = Fnv1a32(tpl);
    snap.sectorHash = Fnv1a64Pos(static_cast<float>(x), static_cast<float>(y));
    snap.pos = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    snap.rot = {0.f, 0.f, 0.f, 1.f};
    snap.state = NpcState::Idle;
    snap.health = 100u;
    snap.aiState = 0u;
    snap.appearanceSeed = 0u;
    snap.phaseId = static_cast<uint32_t>(phase);
    NpcSpawnPacket pkt{snap};
    Net_Broadcast(EMsg::NpcSpawn, &pkt, sizeof(pkt));
    std::cout << "spawn_npc " << tpl << " id=" << snap.npcId << std::endl;
    return PyLong_FromUnsignedLong(snap.npcId);
}

static PyObject* Py_TeleportPeer(PyObject*, PyObject* args)
{
    unsigned int peer;
    PyObject* pos;
    PyObject* rot;
    if (!PyArg_ParseTuple(args, "IOO", &peer, &pos, &rot))
        return nullptr;
    std::cout << "teleport_peer id=" << peer << std::endl;
    Py_RETURN_NONE;
}

static PyObject* Py_SetWeather(PyObject*, PyObject* args)
{
    int id;
    if (!PyArg_ParseTuple(args, "i", &id))
        return nullptr;
    Net_BroadcastWorldState(0, static_cast<uint8_t>(id), static_cast<uint16_t>(std::rand()));
    std::cout << "set_weather " << id << std::endl;
    Py_RETURN_NONE;
}

static PyObject* Py_ShowPopup(PyObject*, PyObject* args)
{
    unsigned int peer;
    const char* text;
    double dur;
    if (!PyArg_ParseTuple(args, "Isd", &peer, &text, &dur))
        return nullptr;
    RED4ext::CString msg(text);
    RED4ext::ExecuteFunction("CoopNotice", "Show", nullptr, &msg);
    std::cout << "show_popup peer=" << peer << " text=" << text << std::endl;
    Py_RETURN_NONE;
}

static PyObject* Py_GetPeerPositions(PyObject*, PyObject*)
{
    auto conns = Net_GetConnections();
    PyObject* list = PyList_New(conns.size());
    for (size_t i = 0; i < conns.size(); ++i)
    {
        auto* c = conns[i];
        PyObject* tup = PyTuple_New(2);
        PyTuple_SetItem(tup, 0, PyLong_FromUnsignedLong(c->peerId));
        PyObject* pos = PyTuple_Pack(3, c->avatarPos.X, c->avatarPos.Y, c->avatarPos.Z);
        PyTuple_SetItem(tup, 1, pos);
        PyList_SetItem(list, i, tup);
    }
    return list;
}

static PyObject* Py_Dist(PyObject*, PyObject* args)
{
    PyObject* a;
    PyObject* b;
    if (!PyArg_ParseTuple(args, "OO", &a, &b))
        return nullptr;
    if (!PyTuple_Check(a) || !PyTuple_Check(b) || PyTuple_Size(a) != 3 || PyTuple_Size(b) != 3)
    {
        PyErr_SetString(PyExc_TypeError, "expected (x,y,z) tuples");
        return nullptr;
    }
    double ax = PyFloat_AsDouble(PyTuple_GetItem(a, 0));
    double ay = PyFloat_AsDouble(PyTuple_GetItem(a, 1));
    double az = PyFloat_AsDouble(PyTuple_GetItem(a, 2));
    double bx = PyFloat_AsDouble(PyTuple_GetItem(b, 0));
    double by = PyFloat_AsDouble(PyTuple_GetItem(b, 1));
    double bz = PyFloat_AsDouble(PyTuple_GetItem(b, 2));
    double dx = ax - bx;
    double dy = ay - by;
    double dz = az - bz;
    double d = std::sqrt(dx * dx + dy * dy + dz * dz);
    return PyFloat_FromDouble(d);
}

static PyObject* Py_SpawnVehicle(PyObject*, PyObject* args)
{
    const char* tpl;
    PyObject* pos;
    PyObject* rot;
    int phase = 0;
    if (!PyArg_ParseTuple(args, "sOO|i", &tpl, &pos, &rot, &phase))
        return nullptr;
    if (!PyTuple_Check(pos) || PyTuple_Size(pos) != 3 || !PyTuple_Check(rot) || PyTuple_Size(rot) != 4)
    {
        PyErr_SetString(PyExc_TypeError, "pos/rot tuple size");
        return nullptr;
    }
    TransformSnap t{};
    t.pos = {static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(pos, 0))),
             static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(pos, 1))),
             static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(pos, 2)))};
    t.rot = {static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(rot, 0))),
             static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(rot, 1))),
             static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(rot, 2))),
             static_cast<float>(PyFloat_AsDouble(PyTuple_GetItem(rot, 3)))};
    VehicleController_SpawnPhaseVehicle(Fnv1a32(tpl), 0u, t, static_cast<uint32_t>(phase));
    Py_RETURN_NONE;
}

static PyObject* Py_SendRPC(PyObject*, PyObject* args)
{
    unsigned int peer;
    const char* fn;
    PyObject* payload;
    if (!PyArg_ParseTuple(args, "IsO", &peer, &fn, &payload))
        return nullptr;
    PyObject* jsonMod = PyImport_ImportModule("json");
    if (!jsonMod)
        return nullptr;
    PyObject* dumps = PyObject_GetAttrString(jsonMod, "dumps");
    PyObject* jstr = PyObject_CallFunctionObjArgs(dumps, payload, nullptr);
    Py_DECREF(dumps);
    Py_DECREF(jsonMod);
    if (!jstr)
        return nullptr;
    const char* js = PyUnicode_AsUTF8(jstr);
    uint16_t len = static_cast<uint16_t>(strlen(js));
    Py_DECREF(jstr);
    PyObject* globals = PyEval_GetGlobals();
    const char* modName = PyUnicode_AsUTF8(PyDict_GetItemString(globals, "__name__"));
    uint16_t pluginId = 0;
    const std::vector<uint32_t>* wl = nullptr;
    PluginManager_GetData(modName, pluginId, wl);
    uint32_t hash = Fnv1a32(fn);
    if (wl && std::find(wl->begin(), wl->end(), hash) == wl->end())
        Py_RETURN_NONE;
    CoopNet::Connection* c = Net_FindConnection(peer);
    if (c)
        Net_SendPluginRPC(c, pluginId, hash, js, len);
    Py_RETURN_NONE;
}

bool PyVM_Init()
{
    if (g_init)
        return true;
    PyConfig cfg;
    PyConfig_InitIsolatedConfig(&cfg);
    cfg.isolated = 1;
    cfg.site_import = 0;
    if (PyStatus_Exception(Py_InitializeFromConfig(&cfg)))
        return false;
    PyEval_InitThreads();
    PyObject* builtins = PyEval_GetBuiltins();
    PyDict_DelItemString(builtins, "open");
    PyDict_DelItemString(builtins, "socket");
    PyDict_DelItemString(builtins, "subprocess");
    static PyMethodDef gameMethods[] = {
        {"_register_event", Py_RegisterEvent, METH_VARARGS, nullptr},
        {"_register_command", Py_RegisterCommand, METH_VARARGS, nullptr},
        {"spawn_npc", Py_SpawnNpc, METH_VARARGS, nullptr},
        {"teleport_peer", Py_TeleportPeer, METH_VARARGS, nullptr},
        {"set_weather", Py_SetWeather, METH_VARARGS, nullptr},
        {"show_popup", Py_ShowPopup, METH_VARARGS, nullptr},
        {"get_peer_positions", Py_GetPeerPositions, METH_NOARGS, nullptr},
        {"dist", Py_Dist, METH_VARARGS, nullptr},
        {"spawn_vehicle", Py_SpawnVehicle, METH_VARARGS, nullptr},
        {"send_rpc", Py_SendRPC, METH_VARARGS, nullptr},
        {nullptr, nullptr, 0, nullptr}};
    static PyModuleDef gameModule = {PyModuleDef_HEAD_INIT, "game", nullptr, -1,
                                     gameMethods};
    PyObject* game = PyModule_Create(&gameModule);
    if (!game)
        return false;
    PyObject* main = PyImport_AddModule("__main__");
    Py_INCREF(game);
    PyModule_AddObject(main, "game", game);
    PyRun_SimpleString(
        "def on(e):\n"
        "    def wrap(f):\n"
        "        game._register_event(e, f)\n"
        "        return f\n"
        "    return wrap\n"
        "def register_command(n,h):\n"
        "    def wrap(f):\n"
        "        game._register_command(n,h,f)\n"
        "        return f\n"
        "    return wrap\n");
    g_init = true;
    return true;
}

bool PyVM_Shutdown()
{
    if (!g_init)
        return true;
    Py_Finalize();
    g_init = false;
    return true;
}

bool PyVM_RunString(const char* code)
{
    if (!g_init)
        return false;
    return PyRun_SimpleString(code) == 0;
}

void PyVM_Dispatch(const std::string& name, PyObject* dict)
{
    auto it = g_listeners.find(name);
    if (it == g_listeners.end())
        return;
    for (const Listener& l : it->second)
    {
        if (!PluginManager_IsEnabled(l.plugin))
            continue;
        PyObject* res = PyObject_CallFunctionObjArgs(l.func, dict, nullptr);
        if (!res && PyErr_Occurred())
        {
            PluginManager_LogException(l.plugin);
            PyErr_Clear();
        }
        Py_XDECREF(res);
    }
}
} // namespace CoopNet
