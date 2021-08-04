#include "pylib.h"

// ================
// both python and leptonica define it
#ifdef HAVE_FSTATAT
    #undef HAVE_FSTATAT
#endif

#include "format/format.h"
#include "image-analysis/image.h"
#include "io-document/io-document.h"
#include "io-document/types.h"
#include "io-document/word.h"
#include "layout-analysis/layout/columns.h"
#include "layout-analysis/layout/gridrows.h"
#include "os/version.h"
#include "serialize/serialize.h"

namespace py = pybind11;

// ================

// clang-format off
namespace maz {

    void init_maz(py::module& m) 
    {
        // ============

        py::bind_map<maz_env_type>(m, "env");

        // ============

        py::class_<serial::i_to_json_dict>(m, "i_to_json_dict")
            .def("to_json_str", &serial::i_to_json_dict::to_json_str, py::arg("indent") = -1);



        // ============
    
        py::class_<maz::doc::source_transformation>(m, "doc_source_transformation")
            .def_readwrite("rotation", &maz::doc::source_transformation::rotation)
            .def_readwrite("scale_to_a4", &maz::doc::source_transformation::scale_to_a4)
            .def_readwrite("scale", &maz::doc::source_transformation::scale)
            .def_readwrite("skew", &maz::doc::source_transformation::skew)
            .def_readwrite("bbox_clip", &maz::doc::source_transformation::bbox_clip)
            ;

        // ============

        py::class_<maz::bbox_type>(m, "bbox_type")
            .def(py::init<>())
            .def(py::init<double, double, double, double>())
            .def("width", &maz::bbox_type::width)
            .def("height", &maz::bbox_type::height)
            .def("xlt", &maz::bbox_type::xlt)
            .def("ylt", &maz::bbox_type::ylt)
            .def("xrb", &maz::bbox_type::xrb)
            .def("yrb", &maz::bbox_type::yrb)
            .def("y_mid", &maz::bbox_type::y_mid)
            .def("x_mid", &maz::bbox_type::x_mid)
            .def("__repr__", &maz::bbox_type::to_string)
        ;

        // ============

        py::class_<maz::doc::word_detail_type>(m, "word_detail_type")
            .def(py::init<>())
            .def_readwrite("from_dict", &maz::doc::word_detail_type::from_dict)
            .def_readwrite("baseline", &maz::doc::word_detail_type::baseline)
            .def_readwrite("warnings", &maz::doc::word_detail_type::warnings)
        ;

        py::class_<maz::doc::word_type, std::shared_ptr<maz::doc::word_type>>(m, "word")
            .def_readwrite("bbox", &maz::doc::base_element::bbox)
            .def_readwrite("text", &maz::doc::base_element::text)
            .def("conf", py::overload_cast<>(&maz::doc::base_element::conf, py::const_))
            .def_readwrite("detail", &maz::doc::word_type::detail)
        ;

        py::class_<maz::doc::line_type, std::shared_ptr<maz::doc::line_type>>(m, "line")
            .def_readwrite("bbox", &maz::doc::base_element::bbox)
            .def_readwrite("text", &maz::doc::base_element::text)
            .def("conf", py::overload_cast<>(&maz::doc::base_element::conf, py::const_))
            .def("words", &maz::doc::line_type::words)
            .def("__len__", &maz::doc::line_type::size)
            .def("__getitem__", [](maz::doc::line_type& self, size_t i) -> maz::doc::ptr_word {
                if (i >= self.size()) throw py::index_error();
                return *std::next(self.begin(), i);
            }, py::return_value_policy::reference_internal)
        ;

        py::class_<maz::doc::page_type, maz::doc::source_transformation>(m, "page")
            .def("lines", py::overload_cast<>(&maz::doc::page_type::lines, py::const_), py::return_value_policy::reference_internal)
            .def("str", [](maz::doc::page_type& page) -> std::string {
                return doc::str(page);
            })
            .def("ia_keys", [](maz::doc::page_type& page) -> std::list<std::string> {
                return page.ia_elems().keys();
            })
            .def("ia_bboxes", [](maz::doc::page_type& page, const std::string& key) -> doc::bboxes_type {
                if (!page.ia_elems().has(key)) return {};
                return page.ia_elems().get(key)->bboxes();
                })
            .def("hlines", [](maz::doc::page_type& page) -> doc::bboxes_type {
                // get hlines from IA
                if (!page.ia_elems().has("hlines")) return {};
                return page.ia_elems().get("hlines")->bboxes();
            })
            .def("vlines", [](maz::doc::page_type& page) -> doc::bboxes_type {
                // get vlines only if table 
                auto js = page.get_layout("table_vlines");
                return maz::doc::json2bboxes(js);
            })
            .def("has_image", [](maz::doc::page_type& page, const std::string& key) -> bool {
                return page.images().has(key);
            })
            .def("image_data_png_base64", [](maz::doc::page_type& page, const std::string& key) -> std::string {
                return page.image_data(key).get<std::string>();
            })
            .def_readonly("bbox", &maz::doc::page_type::bbox)
        ;

        // ============

        py::class_<maz::doc::document>(m, "document")
            .def(py::init<maz_env_type&>())
            .def("from_str",
                [](maz::doc::document& doc, const std::string& js_str) {
                    serial::json_dict js = serial::json_impl::parse(js_str);
                    doc.from_json(js);
                })
            .def("info_env", &maz::doc::document::info_env)
            .def("last_page", py::overload_cast<>(&maz::doc::document::last_page, py::const_), py::return_value_policy::reference)
        ;

        // ============    

        py::class_<maz::ia::image>(m, "image")
            .def(py::init<const std::string&, int>(), py::arg("filename"), py::arg("page") = 1)
            .def(py::init([](const std::string& buf, const std::string& type) {
                return ia::image::base64_decode_copy(buf);
            }), py::arg("buf_base64"), py::arg("image_type"))

            .def("hash", &maz::ia::image::hash)
            .def("raw", &maz::ia::image::raw, py::return_value_policy::copy)
            .def("is_binary", &maz::ia::image::is_binary)
            .def("to8bpp", &maz::ia::image::to8bpp)
            .def("deskew", &maz::ia::image::deskew)
            .def("invert", &maz::ia::image::invert)
            .def("binarize_otsu", &maz::ia::image::binarize_otsu)
            .def("binarize_sauvola", &maz::ia::image::binarize_sauvola)
            .def("clip", py::overload_cast<maz::bbox_type>(&maz::ia::image::clip, py::const_), py::return_value_policy::copy)
            .def("downscale2x", &maz::ia::image::downscale2x)
            .def("upscale2x", &maz::ia::image::upscale2x)
            .def("enhance", &maz::ia::image::enhance)
            .def("bbox", &maz::ia::image::bbox)
            .def("base64_encode", &maz::ia::image::base64_encode)
            ;

        // ============
        py::class_<maz::la::cell_type, std::shared_ptr<maz::la::cell_type>>(m, "la_cell")
            .def("bbox", &maz::la::cell_type::bbox)
            .def("empty", &maz::la::cell_type::empty)
            .def("str", &maz::la::cell_type::utf8_text, py::arg("show_original") = false)
            .def("clear", &maz::la::cell_type::clear)
            .def("__len__", &maz::la::cell_type::size)
            ;

        py::class_<maz::la::row_type, std::shared_ptr<maz::la::row_type>>(m, "la_row")
            .def(py::init<size_t, doc::ptr_line>(), py::arg("col_size"), py::arg("pline") = nullptr)
            .def("non_empty", &maz::la::row_type::non_empty)
            .def("__len__", &maz::la::row_type::size)
            .def("__getitem__", [](maz::la::row_type& self, size_t i) ->maz::la::ptr_cell {
                if (i >= self.size()) throw py::index_error();
                return self[i];
            }, py::return_value_policy::reference_internal)
            .def("str", &maz::la::row_type::utf8_text, py::arg("multi_spaces") = true)
            .def("is_wrapped", &maz::la::row_type::is_wrapped)
            .def("ignored", &maz::la::row_type::ignored)
            .def("valid", &maz::la::row_type::valid)
        ;

        py::class_<maz::la::columns, std::shared_ptr<maz::la::columns>>(m, "la_columns")
            .def("__len__", &maz::la::columns::size)
            .def("bbox", &maz::la::columns::bbox)
            .def("bboxes", &maz::la::columns::bboxes)
            .def("all_rows", &maz::la::columns::all_rows)
            .def("text", &maz::la::columns::text)
            ;

        // ============

        py::class_<maz::la::gridline, std::shared_ptr<maz::la::gridline>>(m, "la_gridline")
            .def(py::init<const maz::doc::bboxes_type&, const maz::doc::bboxes_type&>(), py::arg("hlines"), py::arg("vlines"))
            .def("set_target_segment", py::overload_cast<const maz::doc::bbox_type&>(&maz::la::gridline::target_segment))
            .def("target_segment", py::overload_cast<>(&maz::la::gridline::target_segment, py::const_))
            .def("hlines", &maz::la::gridline::hlines)
            .def("vlines", &maz::la::gridline::vlines)
            .def("set_vlines", &maz::la::gridline::set_vlines)
            .def("clear_lines", &maz::la::gridline::clear_lines)
            .def("__repr__", [](maz::la::gridline& self) -> std::string {
                    return fmt::format("ts:{} hlines:{} vlines:{}", 
                        self.target_segment().to_string(), self.hlines().size(), self.vlines().size());
            })
        ;

        /**
         ```
import pyi2t as m
lgr = m.la_gridrows([])
print(lgr)
print(lgr.to_json_str())
         ```
         */
        py::class_<maz::la::grid_rows, serial::i_to_json_dict>(m, "la_gridrows")
            .def(py::init<const std::list<doc::bboxes_type>&>(), py::arg("rows_cells_bboxes"))
            .def("init", py::overload_cast<doc::document&, la::ptr_columns, la::ptr_gridline>(&maz::la::grid_rows::init), py::arg("doc"), py::arg("pcols"), py::arg("gridline"))
        ;

    }
    // clang-format on

} // namespace maz
