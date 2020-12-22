#include "pylib.h"

// ================

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
            .def(py::init<const std::string&, const std::string&>())
            .def(py::init<maz::doc::page_type&, const std::string&>())
            .def("type", &maz::ml::classify::ib_form::type)
            .def("get_ib_section", &maz::ml::classify::ib_form::get_ib_section);

        py::class_<maz::forms::ib::page_segments_detector>(m, "ib_page_segments_detector")
            .def(py::init<const maz::doc::lines_type&>())
            .def("size", &maz::forms::ib::page_segments_detector::size)
            .def("segments", &maz::forms::ib::page_segments_detector::segments);

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
            .def("__len__", &maz::forms::ib::columns::size)
            .def("__getitem__", [](const maz::forms::ib::columns& cols, size_t i) ->maz::forms::ib::base_column {
                if (i >= cols.size()) throw py::index_error();
                return cols[i];
            }, py::return_value_policy::reference_internal)
        ;

        // ============ 
    
        py::class_<maz::forms::ib::line>(m, "ib_line")
            .def("valid", &maz::forms::ib::line::valid)
            .def("bbox", py::overload_cast<>(&maz::forms::ib::line::bbox, py::const_))
            .def("str", &maz::forms::ib::line::str)
            .def("json_str", [](maz::forms::ib::line& l) ->std::string {
                return l.to_json(serial::full).dump(2);
            })
        ;

        py::class_<maz::forms::ib::report>(m, "ib_report")
            .def(py::init([](maz::doc::document& doc,
                             maz::la::ptr_columns pcols,
                             maz::la::ptr_gridline pgrid,
                             const std::string& template_path) {
                auto ptpl = maz::forms::ib::form_template::create(template_path, doc);
                return maz::forms::ib::report(doc, pcols, pgrid, ptpl, {});
            }))
            .def("find_columns", &maz::forms::ib::report::find_columns)
            .def("handle_corner_case", &maz::forms::ib::report::segment_handle_corner_case)
            .def("extend_grid", &maz::forms::ib::report::segment_extend_grid)
            .def("words_to_columns", &maz::forms::ib::report::words_to_columns)
            .def("best_columns", &maz::forms::ib::report::best_columns)
            .def("parse", &maz::forms::ib::report::parse)
            .def("types", &maz::forms::ib::report::types)
            .def("bboxes", &maz::forms::ib::report::bboxes)
            .def("size", &maz::forms::ib::report::size)
            .def("items", &maz::forms::ib::report::items)
            .def("set_text_debug", &maz::forms::ib::report::set_text_debug)
            ;

        // ============ 

        m.def(
            "detect_columns",
            [](maz::doc::page_type& page, maz::ia::image& imgb) -> std::shared_ptr<maz::la::columns> {
                auto& stats = page.statistics(true);
                int h_line = maz::to_int(stats.means.h_line);
                int h_word = maz::to_int(stats.means.h_word);
                maz::la::ptr_columns pcols_ =
                    std::shared_ptr<maz::la::columns>(new maz::la::columns(
                        maz::forms::ib::columns_from_text::min_cols, page.lines()));
                pcols_->detect(imgb, pcols_->get_h_param(h_line, h_word));
                return pcols_;
            },
            "Create columns representation",
            py::return_value_policy::copy);

        m.def(
            "init_columns",
            [](maz::doc::page_type& page) -> std::shared_ptr<maz::la::columns> {
                auto& stats = page.statistics(true);
                int h_line = maz::to_int(stats.means.h_line);
                int h_word = maz::to_int(stats.means.h_word);
                maz::la::ptr_columns pcols_ =
                    std::shared_ptr<maz::la::columns>(new maz::la::columns(
                        maz::forms::ib::columns_from_text::min_cols, page.lines()));
                return pcols_;
            },
            "Initialize columns for further processing, should be filled subsequently",
            py::return_value_policy::copy);

    }

} // namespace maz
// clang-format on
