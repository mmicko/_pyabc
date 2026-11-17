#include <Python.h>

namespace pyabc
{
    PyObject* PyInit();
}

extern "C"
PyMODINIT_FUNC PyInit__pyabc()
{
    return pyabc::PyInit();
}
