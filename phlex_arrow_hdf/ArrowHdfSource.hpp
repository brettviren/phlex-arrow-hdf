#ifndef PHLEX_ARROW_HDF_ARROWHDFSOURCE_HPP
#define PHLEX_ARROW_HDF_ARROWHDFSOURCE_HPP

// Phlex 0.3.0 source for HDF5 Arrow input (ddm-c3s.18).
//
// This is the official-extension-point replacement for the old
// PHLEX_REGISTER_PROVIDERS + m.provide(...).output_product(...) read module.  A
// phlex::source is the single object that:
//   * OWNS the open file + its scan (the shared read state that both the
//     implicit providers and the file-driven driver operate on — this is the
//     natural owner that resolves ddm-c3s.10), and
//   * create_providers(selector): the implicit-provider FACTORY.  The framework
//     calls this with each downstream input product_selector and wires in the
//     returned bundle(s); ours reads the uniform phlex_arrow::TableGroup for a
//     data cell (via ArrowHdfInput), and
//   * indices(): enumerates the data cells present in the file, so a file-driven
//     driver (ddm-c3s.16) can drive exactly those cells.  The driver receives
//     THIS object and reads hierarchy()/cells() (indices() itself is non-const
//     and framework-facing).
//
// `phlex::source` is a sanctioned public alias.  The provider ABI it requires
// (phlex::detail::provider_bundle / product_specification / product_ptr) is the
// one remaining "verboten"-named surface; it is unavoidable (Phlex's own FORM
// source and unit tests spell it too) and is confined to this file + the
// phlex_arrow aliases in phlex-arrow-common's PhlexTypes.hpp.

#include "phlex_arrow_hdf/ArrowHdfInput.hpp"

#include "phlex_arrow_common/CellHierarchy.hpp"

#include "phlex/core/product_selector.hpp"
#include "phlex/model/data_cell_index.hpp"
#include "phlex/model/index_generator.hpp"
#include "phlex/source.hpp"

#include <memory>
#include <string>
#include <vector>

namespace phlex_arrow_hdf {

class ArrowHdfSource : public phlex::source {
  public:
    /// `input_file`     : the HDF5 file to read.
    /// `writer_creator` : the creator that WROTE the data (used to address it
    ///                    in the file and to select the cells to enumerate).
    /// `product`        : the product suffix to read/emit.
    /// `output_creator` : the creator label the emitted product carries (what
    ///                    downstream nodes route on).
    /// `output_layer`   : the Phlex layer the product lives in.
    ArrowHdfSource(std::string input_file,
                   std::string writer_creator,
                   std::string product,
                   std::string output_creator,
                   std::string output_layer);

    // --- phlex::source interface --------------------------------------------
    phlex::detail::provider_bundles create_providers(
        const phlex::product_selector& selector) override;
    phlex::index_generator indices() override;

    // --- driver-facing view (ddm-c3s.16) ------------------------------------
    /// The data cells in the file that hold this source's (writer_creator,
    /// product), outermost-first order.  A file-driven driver yields these.
    const std::vector<phlex::data_cell_index_ptr>& cells() const { return m_cells; }
    /// The full interpreted cell hierarchy of the file (all creators/products).
    const phlex_arrow::CellHierarchy& hierarchy() const { return m_hier; }

  private:
    std::shared_ptr<ArrowHdfInput> m_reader;  // shared open-file reader (scan + per-cell read)
    std::string m_writer_creator;
    std::string m_product;
    std::string m_output_creator;
    std::string m_output_layer;

    phlex_arrow::CellHierarchy m_hier;                 // scan result (all products)
    std::vector<phlex::data_cell_index_ptr> m_cells;   // cells holding our product
};

}  // namespace phlex_arrow_hdf

#endif  // PHLEX_ARROW_HDF_ARROWHDFSOURCE_HPP
