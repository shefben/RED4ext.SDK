#pragma once
#if defined(__has_include)
#  if __has_include(<Python.h>)
#    include <Python.h>
#  else
struct _object;
using PyObject = _object;
#  endif
#else
#  include <Python.h>
#endif
namespace CoopNet {
bool PyVM_Init();
bool PyVM_Shutdown();
bool PyVM_RunString(const char* code);
void PyVM_Dispatch(const std::string& name, PyObject* dict);
}
