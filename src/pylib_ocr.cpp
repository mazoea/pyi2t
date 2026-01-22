#include "pylib.h"

// ================
// both python and leptonica define it
#ifdef HAVE_FSTATAT
#undef HAVE_FSTATAT
#endif

#include "ocr/engines.h"
#include "ocr/processing.h"
#include "ocr/reocr.h"
#include "ocr/osd.h"

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
            .def("ocr", static_cast<maz::ocr::engine& (maz::ocr::engine_manager::*)()>(&maz::ocr::engine_manager::ocr), py::return_value_policy::reference_internal)
            .def("reocr", static_cast<maz::ocr::engine& (maz::ocr::engine_manager::*)()>(&maz::ocr::engine_manager::reocr), py::return_value_policy::reference_internal);

        // ============

        m.def(
            "ocr_line",
            [](maz::ocr::engine& engine, maz::ia::image& img, bool raw) {
                maz::ocr::run_stats runstats;
                maz::doc::words_type words;
                std::string s = maz::ocr::ocr_line(engine, runstats, words, img, "pyocr:ocr_line");
                return make_tuple(s, words);
            },
            "OCR line image");

        m.def(
            "reocr",
            [](maz::ocr::engine& engine, const maz::ia::image& page_img, doc::bbox_type word_bbox, bool raw) {
                maz::ocr::run_stats runstats;
                maz::doc::words_type words;
                std::string s;

                doc::ptr_word pwtmp(new doc::word_type("", {"", ""}));
                std::list<std::string> tags;
                bool one_simple_line = true;

                std::shared_ptr<segment::word> pwi = ocr::reocr::prepare_image(
                    page_img, word_bbox, pwtmp, tags, one_simple_line);
                if (!pwi) return make_tuple(s, words);

                s = maz::ocr::ocr_line(engine, runstats, words, *pwi, "pyocr:reocr", raw);
                return make_tuple(s, words);
            },
            "reOCR line image");


        m.def(
            "ocr_word",
            [](maz::ocr::engine& engine, maz::ia::image& img) {
                maz::ocr::run_stats runstats;
                maz::doc::words_type words;
                std::string s = maz::ocr::ocr_word(engine, runstats, words, img, "pyocr:ocr_word");
                return make_tuple(s, words);
            },
            "OCR word image");

        m.def(
            "ocr_block",
            [](maz::ocr::engine& engine, maz::ia::image& img) {
                maz::ocr::run_stats runstats;
                maz::doc::words_type words;
                std::string s = maz::ocr::ocr_block(engine, runstats, words, img, "pyocr:ocr_block");
                return make_tuple(s, words);
            },
            "OCR block image");

        m.def(
            "detect_rotation",
            [](const maz::ia::image& img, ocr::engine_manager& eng_mng, std::string dbg_dir = {})
            {
                double angle = 0.;
                bool angle_set = false;
                double deskew = 0.;
                bool deskew_set = false;
                std::string dbg;

                maz::ia::image imgb = img;

                if (!imgb.is_binary())
                {
                  imgb.binarize();
                  if (!dbg_dir.empty()) imgb.save(dbg_dir+"bin.png");
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

                // 3. detect rotation
				// Check if we can access the engine
				ocr::osd rotator(imgb, true, dbg_dir);
				auto rot = rotator.detect(eng_mng);
				if (rot.state == ocr::osd::ret_state::correct)
				{
					angle = 0.;
					angle_set = true;
				} else if (rot.state == ocr::osd::ret_state::up_side_down)
				{
					angle = 180.;
					angle_set = true;
				}
                dbg = rot.to_str();
                if (!angle_set)
                {
                    int guessed = imgb.detect_orientation();
                    if (DONT_KNOW != guessed)
                    {
                        angle = static_cast<double>(guessed);
                        angle_set = true;
                        dbg += ";guessed_by_orientation";
                    }
                }
                if (!angle_set)
                {
                   int guessed = imgb.detect_orientation_bboxes();
                   if (DONT_KNOW != guessed)
                   {
                        angle = static_cast<double>(guessed);
                        angle_set = true;
                        dbg += ";guessed_by_bbs";
                   }
                }
                if (!angle_set)
                {
						ocr::osc rot_checker(imgb, true, dbg);
                        auto roc = rot_checker.detect(eng_mng);
                        dbg +=roc.to_str();
                        if (roc.state == ocr::osc::ret_state::ok)
                        {
                            int pos = 0;
                            double max_conf = roc.confs[0];
                            for (size_t i = 1; i < roc.confs.size(); ++i)
                                                            {
                                if (roc.confs[i] > max_conf)
                                {
                                    max_conf = roc.confs[i];
                                    pos = i;
                                }
                            }
                        }

                }

                return std::make_tuple(angle, deskew, dbg);
            },
            "Return detected rotation and deskew");

    }


} // namespace maz
// clang-format on
