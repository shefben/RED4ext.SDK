#include "PluginManager.hpp"
#include "PythonVM.hpp"
#if defined(__has_include)
#  if __has_include(<Python.h>)
#    include <Python.h>
#  endif
#else
#  include <Python.h>
#endif
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "../net/Net.hpp"
#include "../core/Hash.hpp"
#include "../third_party/zstd/zstd.h"
#include <openssl/sha.h>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace CoopNet {
struct PluginInfo {
    PluginMetadata meta;
    std::string hash;
    PyObject* module{nullptr};
    std::vector<uint32_t> whitelist;
    unsigned errors{0};
    bool enabled{true};
};

struct CommandInfo
{
    std::string help;
    PyObject* func{nullptr};
    std::string plugin;
};

static std::unordered_map<std::string, PluginInfo> g_plugins;
static std::unordered_map<std::string, CommandInfo> g_commands;
static float g_timer = 0.f;

void PluginManager_RegisterCommand(const std::string& name, const std::string& help,
                                   PyObject* func, const std::string& plugin)
{
    Py_INCREF(func);
    g_commands[name] = {help, func, plugin};
}
static uint16_t g_nextPluginId = 1;

bool PluginManager_IsEnabled(const std::string& plugin)
{
    auto it = g_plugins.find(plugin);
    return it != g_plugins.end() && it->second.enabled;
}

void PluginManager_LogException(const std::string& plugin)
{
    PyObject *type, *value, *trace;
    PyErr_Fetch(&type, &value, &trace);
    PyErr_NormalizeException(&type, &value, &trace);
    PyObject* tb = PyImport_ImportModule("traceback");
    PyObject* fmt = PyObject_GetAttrString(tb, "format_exception");
    PyObject* list = PyObject_CallFunctionObjArgs(fmt, type, value, trace, NULL);
    PyObject* empty = PyUnicode_FromString("");
    PyObject* text = PyUnicode_Join(empty, list);
    const char* ctext = PyUnicode_AsUTF8(text);
    fs::create_directories("logs/plugins");
    std::ofstream f("logs/plugins/" + plugin + ".log", std::ios::app);
    f << ctext << std::endl;
    Py_DECREF(text);
    Py_DECREF(empty);
    Py_DECREF(list);
    Py_DECREF(fmt);
    Py_DECREF(tb);
    Py_DECREF(type);
    Py_DECREF(value);
    Py_XDECREF(trace);
    auto it = g_plugins.find(plugin);
    if (it != g_plugins.end())
    {
        if (++it->second.errors >= 5 && it->second.enabled)
        {
            it->second.enabled = false;
            std::string msg = "[Plugin " + it->second.meta.name + " disabled – error]";
            Net_BroadcastChat(msg);
        }
    }
}

const PluginMetadata* PluginManager_GetInfo(const std::string& name)
{
    auto it = g_plugins.find(name);
    if (it != g_plugins.end())
        return &it->second.meta;
    return nullptr;
}

bool PluginManager_GetData(const std::string& name, uint16_t& id,
                           const std::vector<uint32_t>*& whitelist)
{
    auto it = g_plugins.find(name);
    if (it == g_plugins.end())
        return false;
    id = it->second.meta.id;
    whitelist = &it->second.whitelist;
    return true;
}

static void PushAssets(const std::string& name, uint16_t pluginId)
{
    fs::path dir = fs::path("plugins") / name / "assets";
    if (!fs::exists(dir))
        return;
    std::vector<char> buf;
    for (const auto& f : fs::recursive_directory_iterator(dir))
    {
        if (!f.is_regular_file())
            continue;
        std::ifstream in(f.path(), std::ios::binary);
        std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        uint16_t pathLen = static_cast<uint16_t>(f.path().lexically_relative(dir).string().size());
        std::string rel = f.path().lexically_relative(dir).string();
        buf.insert(buf.end(), reinterpret_cast<char*>(&pathLen), reinterpret_cast<char*>(&pathLen) + 2);
        buf.insert(buf.end(), rel.begin(), rel.end());
        uint32_t len = static_cast<uint32_t>(data.size());
        buf.insert(buf.end(), reinterpret_cast<char*>(&len), reinterpret_cast<char*>(&len) + 4);
        buf.insert(buf.end(), data.begin(), data.end());
    }
    if (buf.size() > 5u * 1024u * 1024u)
        return;
    std::vector<uint8_t> comp(ZSTD_compressBound(buf.size()));
    size_t z = ZSTD_compress(comp.data(), comp.size(), buf.data(), buf.size(), 3);
    if (ZSTD_isError(z))
        return;
    comp.resize(z);
    unsigned char sha[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(comp.data()), comp.size(), sha);
    Net_BroadcastAssetBundle(pluginId, comp);
}
static PyObject* BuildDict(const std::vector<std::pair<std::string, std::string>>& kv)
{
    PyObject* d = PyDict_New();
    for (const auto& [k, v] : kv)
        PyDict_SetItemString(d, k.c_str(), PyUnicode_FromString(v.c_str()));
    return d;
}

static std::string HashFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    std::hash<std::string> h;
    return std::to_string(h(ss.str()));
}

static void LoadPlugin(const std::string& path)
{
    std::string hash = HashFile(path);
    auto it = g_plugins.find(path);
    if (it != g_plugins.end() && it->second.hash == hash)
        return;

    if (it != g_plugins.end())
    {
        PyObject* importlib = PyImport_ImportModule("importlib");
        if (!importlib)
        {
            PyErr_Print();
            return;
        }
        PyObject* reloaded = PyObject_CallMethod(importlib, "reload", "O", it->second.module);
        Py_DECREF(importlib);
        if (!reloaded)
        {
            PyErr_Print();
            return;
        }
        it->second.module = reloaded;
        it->second.hash = hash;
        PushAssets(path, it->second.meta.id);
    }
    else
    {
        PyObject* name = PyUnicode_DecodeFSDefault(path.c_str());
        PyObject* module = PyImport_Import(name);
        Py_DECREF(name);
        if (!module)
        {
            PyErr_Print();
            return;
        }
        PluginInfo info{};
        info.module = module;
        info.hash = hash;
        info.meta.id = g_nextPluginId++;
        PyObject* meta = PyObject_GetAttrString(module, "__plugin__");
        if (meta && PyDict_Check(meta))
        {
            PyObject* nameObj = PyDict_GetItemString(meta, "name");
            PyObject* verObj = PyDict_GetItemString(meta, "version");
            if (nameObj)
                info.meta.name = PyUnicode_AsUTF8(nameObj);
            if (verObj)
                info.meta.version = PyUnicode_AsUTF8(verObj);
            info.meta.hash = hash;
            PyObject* funcs = PyDict_GetItemString(meta, "client_funcs");
            if (funcs && PyList_Check(funcs))
            {
                Py_ssize_t n = PyList_Size(funcs);
                for (Py_ssize_t i = 0; i < n; ++i)
                {
                    PyObject* f = PyList_GetItem(funcs, i);
                    if (PyUnicode_Check(f))
                        info.whitelist.push_back(Fnv1a32(PyUnicode_AsUTF8(f)));
                }
            }
        }
        Py_XDECREF(meta);
        g_plugins[path] = info;
        PyModule_AddIntConstant(module, "__plugin_id__", info.meta.id);
        PushAssets(path, info.meta.id);
    }
}

static void Scan()
{
    for (const auto& e : fs::directory_iterator("plugins"))
    {
        if (e.path().extension() == ".py")
            LoadPlugin(e.path().stem().string());
    }
}

bool PluginManager_Init()
{
    g_timer = 0.f;
    if (!PyVM_Init())
        return false;
    if (fs::exists("plugins"))
        Scan();
    return true;
}

void PluginManager_Shutdown()
{
    for (auto& kv : g_plugins)
        Py_XDECREF(kv.second.module);
    g_plugins.clear();
    PyVM_Shutdown();
}

void PluginManager_Tick(float dt)
{
    g_timer += dt;
    PyObject* d = Py_BuildValue("{s:f}", "dt", dt);
    PyVM_Dispatch("OnTick", d);
    Py_DECREF(d);
    if (g_timer >= 60.f)
    {
        g_timer = 0.f;
        Scan();
    }
}

void PluginManager_DispatchEvent(const std::string& name, PyObject* dict)
{
    PyVM_Dispatch(name, dict);
}

bool PluginManager_HandleChat(uint32_t peerId, const std::string& msg, bool)
{
    if (msg.rfind('/', 0) != 0)
        return false;
    std::stringstream ss(msg.substr(1));
    std::string cmd;
    ss >> cmd;
    std::vector<std::string> args;
    std::string a;
    while (ss >> a)
        args.push_back(a);
    auto it = g_commands.find(cmd);
    if (it == g_commands.end())
        return false;
    PyObject* argList = PyList_New(args.size());
    for (size_t i = 0; i < args.size(); ++i)
        PyList_SetItem(argList, i, PyUnicode_FromString(args[i].c_str()));
    PyObject* dict = Py_BuildValue("{s:I,s:O}", "peerId", peerId, "args", argList);
    PyVM_Dispatch("OnChatMsg", dict);
    if (PluginManager_IsEnabled(it->second.plugin))
    {
        PyObject* res = PyObject_CallFunction(it->second.func, "IO", peerId, argList);
        if (!res && PyErr_Occurred())
        {
            PluginManager_LogException(it->second.plugin);
            PyErr_Clear();
        }
        Py_XDECREF(res);
    }
    Py_DECREF(argList);
    Py_DECREF(dict);
    return true;
}

} // namespace CoopNet
