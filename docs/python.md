# Python Plugin Best Practices

- Validate objects returned from the Python C API before using them.
- Release temporary Python objects with `Py_XDECREF` to avoid crashes.
- Clear errors with `PyErr_Clear()` when a failing call is not fatal.
- Keep event handlers short to minimise blocking of the game thread.
