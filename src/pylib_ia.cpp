#include "pylib.h"

// ================
// both python and leptonica define it
#ifdef HAVE_FSTATAT
#undef HAVE_FSTATAT
#endif

#include "segment/segments/lines.h"

namespace py = pybind11;

// ================

// clang-format off
namespace maz {

    void init_ia(py::module& m) 
    {
        // ============

        m.def(
            "ia_lines",
            [](const maz::ia::image& imgb, int letter_h, const std::string& dbg)
            {
                using namespace maz::segment;

                lines::lines_info li;

                if (!imgb.is_binary())
                {
                    ia::ptr_image pimgb = imgb.binary_copy();
                    li = lines::extract(*pimgb, letter_h, dbg);
                }
                else
                {
                    li = lines::extract(imgb, letter_h, dbg);
                }

                return make_tuple(li.hlines, li.vlines);
            },
            "IA extract hlines/vlines");
    }

} // namespace maz
// clang-format on
