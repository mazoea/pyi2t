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

        m.def(
            "detect_deskew",
            [](const maz::ia::image& img)
            {
                double deskew = 0.;

                maz::ia::image imgb = img;

                if (!imgb.is_binary())
                {
                  imgb.binarize();
				}

                // 1. scale it to a4 size
                float really_scaled = 0.f;
                float scale_to_a4 = 0.f;
                imgb.scale_to_A4_like(scale_to_a4, really_scaled);

                // 2. deskew
                float fconfidence = 0.f;
                float skew = 0.f;
				imgb.deskew(skew, fconfidence);
				// rotate also the original image
				if (0. != skew)
				{
					deskew = skew;
				}

                return deskew;
            },
            "Return detected deskew");

    }

} // namespace maz
// clang-format on
