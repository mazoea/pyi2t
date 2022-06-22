// author: jm (Mazoea s.r.o.)
// date: 2020
#pragma once

#include "os/compiler.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

// make env accessible via reference
PYBIND11_MAKE_OPAQUE(std::map<std::string, std::string>);
using maz_env_type = std::map<std::string, std::string>;

namespace maz {

    /** Export maz base. */
    void init_maz(pybind11::module&);

    /** Export ia related. */
    void init_ia(pybind11::module&);

    /** Export OCR related functions. */
    void init_ocr(pybind11::module&);

    /** Export processing extensions - IB forms. */
    void init_forms(pybind11::module&);

} // namespace maz
