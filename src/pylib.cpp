#include "pylib.h"

// ================

// clang-format off
#ifndef PYMODULENAME
    #pragma message("[PYMODULENAME] empty - could cause problems when importing from python!")
    #define PYMODULENAME pyi2t
#else

    #ifndef TOSTRING
        #define DO_EXPAND(VAL) VAL##1
        #define EXPAND(VAL) DO_EXPAND(VAL)
        #define STRINGIFY(x) #x
        #define TOSTRING(x) STRINGIFY(x)
    #endif
    #define _PYMSG TOSTRING(PYMODULENAME)
    #pragma message("[PYMODULENAME] set to " _PYMSG)

    #define _PYVER TOSTRING(PY_VERSION)
    #pragma message("[PY_VERSION_HEX] set to " _PYVER)

#endif

// ================

#include "os/version.h"

namespace py = pybind11;

// ================

PYBIND11_MODULE(PYMODULENAME, m)
{
    m.doc() = R"pbdoc(
           i2t python wrapper
    )pbdoc";

    // ============

    m.attr("__version__") = maz::version();

    // ============

    maz::init_maz(m);
    maz::init_ia(m);
    maz::init_ocr(m);
    maz::init_forms(m);
}

// clang-format on
