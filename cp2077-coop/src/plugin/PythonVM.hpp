#pragma once
namespace CoopNet {
bool PyVM_Init();
bool PyVM_Shutdown();
bool PyVM_RunString(const char* code);
void PyVM_Dispatch(const std::string& name, PyObject* dict);
}
