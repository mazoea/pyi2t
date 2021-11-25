#include "pylib.h"

// ================
// both python and leptonica define it
#ifdef HAVE_FSTATAT
#undef HAVE_FSTATAT
#endif

#include "forms/ib/columns_text.h"
#include "forms/ib/page_segments_detector.h"
#include "forms/ib/report.h"
#include "ml/forms.h"

namespace py = pybind11;

// ================

// clang-format off
namespace maz {

    void init_forms(py::module& m)
    {
        // ============

        py::class_<maz::ml::features, serial::i_to_json_dict>(m, "ml_features").def(py::init<>());

        // ============

        py::class_<maz::ml::classify::ib_form::form_type, serial::i_to_json_dict>(
            m, "ml_ib_form_type")
            .def(py::init<>())
            .def("is_ib", &maz::ml::classify::ib_form::form_type::is_ib)
            .def("str", &maz::ml::classify::ib_form::form_type::str);

        py::class_<maz::ml::classify::ib_form, maz::ml::features>(m, "ml_ib_form")
            .def(py::init<const std::string&>())
            .def("process", &maz::ml::classify::ib_form::process)
            .def("type", &maz::ml::classify::ib_form::type)
            .def("get_ib_section", &maz::ml::classify::ib_form::get_ib_section);

        py::class_<maz::forms::ib::page_segments_detector>(m, "ib_page_segments_detector")
            .def(py::init<const maz::doc::lines_type&>())
            .def("size", &maz::forms::ib::page_segments_detector::size)
            .def("segments", &maz::forms::ib::page_segments_detector::segments);

        m.def(
            "ml_ib_form_prepare",
            [](maz::doc::document& doc) {
                maz::ml::classify::ib_form::reformat(doc);
            },
            "Prepare document for feature extraction");

        // ============

        py::class_<maz::forms::ib::col_feats>(m, "ib_col_feats")
            .def_readwrite("values", &maz::forms::ib::col_feats::values)
            .def("header", py::overload_cast<>(&maz::forms::ib::col_feats::header, py::const_))
        ;

        py::class_<maz::forms::ib::base_column> ib_base_column (m, "ib_base_column");
        ib_base_column.def("features", &maz::forms::ib::base_column::features)
            .def("type", &maz::forms::ib::base_column::tp)
            .def("clear", &maz::forms::ib::base_column::clear)
        ;

        py::enum_<maz::forms::ib::base_column::type>(ib_base_column, "ib_base_column_type")
            .value("k_unknown", maz::forms::ib::base_column::type::k_unknown)
            .value("k_dos", maz::forms::ib::base_column::type::k_dos)
            .value("k_revCode", maz::forms::ib::base_column::type::k_revCode)
            .value("k_serviceCode", maz::forms::ib::base_column::type::k_serviceCode)
            .value("k_description", maz::forms::ib::base_column::type::k_description)
            .value("k_units", maz::forms::ib::base_column::type::k_units)
            .value("k_unitCost", maz::forms::ib::base_column::type::k_unitCost)
            .value("k_charge", maz::forms::ib::base_column::type::k_charge)
            .export_values();

        py::class_<maz::forms::ib::columns>(m, "ib_columns")
            .def("known", py::overload_cast<>(&maz::forms::ib::columns::known, py::const_))
            .def("unknown", &maz::forms::ib::columns::unknown)
            .def("disable", &maz::forms::ib::columns::disable)
            .def("clear", [](maz::forms::ib::columns& self, size_t i) {
                self.clear(i);
            })
            .def("moves", &maz::forms::ib::columns::moves)
            .def("__len__", &maz::forms::ib::columns::size)
            .def("__getitem__", [](const maz::forms::ib::columns& cols, size_t i) ->maz::forms::ib::base_column {
                if (i >= cols.size()) throw py::index_error();
                return cols[i];
            }, py::return_value_policy::reference_internal)
            .def("__repr__", [](maz::forms::ib::columns& self) -> std::string {
                return self.str({});
            })
        ;

        // ============ 
    
        py::class_<maz::forms::ib::line, serial::i_to_json_dict>(m, "ib_line")
            .def("valid", &maz::forms::ib::line::valid)
            .def("bbox", py::overload_cast<>(&maz::forms::ib::line::bbox, py::const_))
            .def("str", &maz::forms::ib::line::str)
        ;

        py::class_<maz::forms::ib::lines>(m, "lines")
            .def("__len__", &maz::forms::ib::lines::size)
            .def("__getitem__", [](const maz::forms::ib::lines& ib_lines, size_t i) ->maz::forms::ib::line {
            if (i >= ib_lines.size()) throw py::index_error();
                return *std::next(ib_lines.begin(), i);
            }, py::return_value_policy::reference_internal)
            .def("all_size", &maz::forms::ib::lines::all_size)
            .def("ignored_size", &maz::forms::ib::lines::ignored_size)
        ;

        py::class_<maz::forms::ib::report, std::shared_ptr<maz::forms::ib::report>>(m, "ib_report")
            .def("find_columns", &maz::forms::ib::report::find_columns)
            .def("handle_corner_case", &maz::forms::ib::report::handle_corner_case)
            .def("words_to_columns", &maz::forms::ib::report::words_to_columns)
            .def("best_columns", &maz::forms::ib::report::best_columns)
            .def("parse", &maz::forms::ib::report::parse)
        
            .def("types", &maz::forms::ib::report::types)
            .def("formats", &maz::forms::ib::report::formats)

            .def("bboxes", &maz::forms::ib::report::bboxes)
            .def("size", &maz::forms::ib::report::size)
            .def("items", &maz::forms::ib::report::items)
            .def("set_text_debug", &maz::forms::ib::report::set_text_debug)
            //
            .def("pgrid", &maz::forms::ib::report::pgrid)
            .def("pcols", &maz::forms::ib::report::pcols)
            //
            .def("save_checkpoint",
                py::overload_cast<const std::string&, const std::string&>(&maz::forms::ib::report::save_checkpoint),
                py::arg("key"), py::arg("tags") = "")
            .def("save_ib_info", &maz::forms::ib::report::save_ib_info)
        ;

        // ============ 

        m.def(
            "create_report",
            [](maz::doc::document& doc,
                maz::la::ptr_columns pcols,
                maz::la::ptr_gridline pgrid,
                const std::string& template_path,
                const maz::ia::image& imgb,
                const std::string& dbg) -> std::shared_ptr<maz::forms::ib::report> 
            {
                    auto ptpl = maz::forms::ib::form_template::create(template_path, doc);

                    // we need shared_ptr, internally pixClone ensures we will not
                    // double free the memory
                    maz::ia::ptr_image pimg = std::make_shared<maz::ia::image>(imgb);

                    // create the report
                    std::shared_ptr<maz::forms::ib::report> preport = maz::forms::ib::report::create(
                        doc, pcols, pgrid, ptpl, pimg, dbg
                    );
                    return preport;
            },
            py::arg("doc"), 
            py::arg("pcols"),
            py::arg("pgrid"),
            py::arg("template_path"),
            py::arg("imgb"),
            py::arg("dbg") = "",
            "Initialize report",
            py::return_value_policy::copy
        );

        m.def(
            "create_columns",
            [](maz::doc::page_type& page) -> std::shared_ptr<maz::la::columns> {
                return maz::la::columns::create(
                    maz::forms::ib::columns_from_text::min_cols, page
                );
            },
            "Initialize columns for further processing, should be filled subsequently",
            py::return_value_policy::copy
        );

        m.def(
            "init_columns",
            [](const doc::page_type& page, 
                maz::la::columns& cols, 
                maz::la::gridline& gridline) -> void 
            {
                maz::forms::ib::report::init_columns(page, cols, gridline);
            },
            "Initialize columns from gridline"
        );

        m.def(
            "detect_columns",
            [](maz::doc::page_type& page, 
                maz::ia::image& imgb, 
                maz::la::ptr_gridline pgrid, 
                int line_h,
                const std::string& dbg) -> std::shared_ptr<maz::la::columns> 
            {
                return maz::forms::ib::report::detect_columns(imgb, page, pgrid, line_h, dbg);
            },
            py::arg("page"),
            py::arg("imgb"),
            py::arg("pgrid"),
            py::arg("line_h") = -1,
            py::arg("dbg") = "",
            "Detect columns representation on an image",
            py::return_value_policy::copy
        );

        m.def(
            "columns_from_table",
            [](maz::doc::page_type& page, 
                const ia::image& imgb, 
                const doc::bbox_type& ib_bbox, 
                const std::string& dbg) -> std::shared_ptr<maz::la::columns> 
            {
                if (!page.ia_elems().has("table_bbox")) return {};
                if (!page.ia_elems().has("table_bboxes")) return {};

                // stored as array
                doc::bboxes_type table_bbox_arr = page.ia_elems().get("table_bbox")->bboxes();
                if (1 != table_bbox_arr.size()) return {};
                doc::bbox_type table_bbox = table_bbox_arr.front();

                static constexpr size_t min_cols = maz::forms::ib::columns_from_text::min_cols;
                doc::bboxes_type table_bboxes = page.ia_elems().get("table_bboxes")->bboxes();
                if (table_bboxes.size() < min_cols) return {};

                la::ptr_columns pcols = maz::forms::ib::report::use_table_columns(
                    imgb, page.lines(), ib_bbox, table_bbox, table_bboxes, dbg
                );
                return pcols;
            },
            py::arg("page"),
            py::arg("imgb"),
            py::arg("ib_bbox"),
            py::arg("dbg") = "",
            "Decide if we can use the columns from a generic table layout",
            py::return_value_policy::copy
        );

        m.def("rough_ib_lines",
            [](const maz::doc::lines_type& lines) -> doc::lines_type {
                return forms::ib::rough_ib_lines(lines);
            },
            "Returns lines that are IB like.",
            py::return_value_policy::copy
        );

    }

} // namespace maz
// clang-format on
