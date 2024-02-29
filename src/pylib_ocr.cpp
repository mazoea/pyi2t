#include "pylib.h"

// ================
// both python and leptonica define it
#ifdef HAVE_FSTATAT
#undef HAVE_FSTATAT
#endif

#include "ocr/engines.h"
#include "ocr/processing.h"
#include "ocr/reocr.h"

namespace py = pybind11;

// ================

// clang-format off
namespace maz {

    void init_ocr(py::module& m) 
    {
        py::class_<maz::ocr::engine>(m, "ocr_engine")
            .def("name", &maz::ocr::engine::name)
            .def("image", &maz::ocr::engine::image)
            .def("recognize", &maz::ocr::engine::recognise)
            .def("init", &maz::ocr::engine::init)
            .def("data_version", &maz::ocr::engine::data_version)
            .def("version", &maz::ocr::engine::version)
            .def("known_word", &maz::ocr::engine::known_word)
            .def("known_userdict_word", &maz::ocr::engine::known_userdict_word)
            .def("adjust_for_text_word", &maz::ocr::engine::adjust_for_text_word)
            .def("adjust_for_text_line", &maz::ocr::engine::adjust_for_text_line)
            .def("adjust_for_page", &maz::ocr::engine::adjust_for_page);

        // ============

        py::class_<maz::ocr::engine_manager>(m, "ocr_engine_manager")
            .def(py::init<const std::string&, const std::string&>(), py::arg("default"), py::arg("reocr"))
            .def("ocr", &maz::ocr::engine_manager::ocr, py::return_value_policy::reference_internal)
            .def("reocr", &maz::ocr::engine_manager::reocr, py::return_value_policy::reference_internal);

        // ============

        m.def(
            "ocr_line",
            [](maz::ocr::engine& engine, maz::ia::image& img, bool raw = false) {
                maz::ocr::run_stats runstats;
                maz::doc::words_type words;
                std::string s = maz::ocr::ocr_line(engine, runstats, words, img, raw);
                return make_tuple(s, words);
            },
            "OCR line image");

        m.def(
            "reocr",
            [](maz::ocr::engine& engine, const maz::ia::image& page_img, doc::bbox_type word_bbox, bool raw = false) {
                maz::ocr::run_stats runstats;
                maz::doc::words_type words;
                std::string s;

                doc::ptr_word pwtmp(new doc::word_type("", {"", ""}));
                std::list<std::string> tags;
                bool one_simple_line = true;

                std::shared_ptr<segment::word> pwi = ocr::reocr::prepare_image(
                    page_img, word_bbox, pwtmp, tags, one_simple_line);
                if (!pwi) return make_tuple(s, words);

                s = maz::ocr::ocr_line(engine, runstats, words, *pwi, raw);
                return make_tuple(s, words);
            },
            "reOCR line image");


        m.def(
            "ocr_word",
            [](maz::ocr::engine& engine, maz::ia::image& img) {
                maz::ocr::run_stats runstats;
                maz::doc::words_type words;
                std::string s = maz::ocr::ocr_word(engine, runstats, words, img);
                return make_tuple(s, words);
            },
            "OCR word image");

    }

} // namespace maz
// clang-format on
