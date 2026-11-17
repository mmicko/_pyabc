#include "pyabc.h"

#include <misc/util/abc_global.h>

ABC_NAMESPACE_HEADER_START

int Abc_RealMain(int argc, char *argv[]);

ABC_NAMESPACE_HEADER_END

namespace pyzz
{
PyObject* zz_init();
}

int main(int argc, char *argv[])
{
    PyImport_AppendInittab("_pyabc", pyabc::PyInit);
    PyImport_AppendInittab("_pyzz", pyzz::zz_init);

    // do not import site, as the Python 2 version did with Py_NoSiteFlag
    py::initialize interpreter(argv[0], 1, false);

    try
    {
        py::Import_ImportModule("pyabc");
    }
    catch(py::exception&)
    {
        fprintf( stderr, "error: could not load module pyabc:\n");
        PyErr_Print();
    }

    return ABC_NAMESPACE_PREFIX Abc_RealMain(argc, argv);
}
