#ifndef PHLEX_ARROW_HDF_ARROWHDFSOURCE_HPP
#define PHLEX_ARROW_HDF_ARROWHDFSOURCE_HPP

// Phlex source for HDF5 Arrow input (ddm-c3s.18; ported to Phlex 0.4.1).
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
// `phlex::source` and `phlex::provider_bundles` are public (stable `phlex::`
// namespace) as of Phlex 0.4.  Filling a bundle needs Phlex's experimental
// erased-product API; that is confined to phlex-arrow-common's PhlexSource.hpp
// (provide_if_selected), so this package names no phlex::detail or
// phlex::experimental type.
//
// Stage: since Phlex 0.4 an implicit provider must carry a real stage name
// (not the reserved "CURRENT"): the stage of the data being read.  It is the
// `stage` source config key (see SourceModule.cpp).

#include "phlex_arrow_hdf/ArrowHdfInput.hpp"

#include "phlex_arrow_common/CellHierarchy.hpp"

#include "phlex/model/data_cell_index.hpp"
#include "phlex/model/index_generator.hpp"
#include "phlex/source.hpp"  // phlex::source, provider_bundles, product_selector

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
    /// `stage`          : the stage the emitted product carries (the stage
    ///                    that produced the data; must not be "CURRENT").
    ArrowHdfSource(std::string input_file,
                   std::string writer_creator,
                   std::string product,
                   std::string output_creator,
                   std::string output_layer,
                   std::string stage = "input");

    // --- phlex::source interface --------------------------------------------
    phlex::provider_bundles create_providers(
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
    std::string m_stage;

    phlex_arrow::CellHierarchy m_hier;                 // scan result (all products)
    std::vector<phlex::data_cell_index_ptr> m_cells;   // cells holding our product
};

}  // namespace phlex_arrow_hdf

#endif  // PHLEX_ARROW_HDF_ARROWHDFSOURCE_HPP
