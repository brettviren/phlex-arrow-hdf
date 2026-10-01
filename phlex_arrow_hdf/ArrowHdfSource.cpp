#include "phlex_arrow_hdf/ArrowHdfSource.hpp"

#include "phlex_arrow_common/PhlexSource.hpp"  // provide_if_selected (experimental-API choke point)

#include "phlex/concurrency.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace phlex_arrow_hdf {

namespace {
    // Walk the CellHierarchy, building a data_cell_index per node and collecting
    // those that hold (creator, product), outermost-first (pre-order).
    void collect_cells(const phlex::data_cell_index_ptr& parent,
                       const std::vector<phlex_arrow::CellNode>& nodes,
                       const std::string& creator,
                       const std::string& product,
                       std::vector<phlex::data_cell_index_ptr>& out)
    {
        for (auto const& node : nodes) {
            auto idx = parent->make_child(node.layer, static_cast<std::size_t>(node.number));
            for (auto const& loc : node.products) {
                if (loc.creator == creator && loc.product == product) {
                    out.push_back(idx);
                    break;
                }
            }
            collect_cells(idx, node.children, creator, product, out);
        }
    }
}  // namespace

ArrowHdfSource::ArrowHdfSource(std::string input_file,
                               std::string writer_creator,
                               std::string product,
                               std::string output_creator,
                               std::string output_layer,
                               std::string stage)
  : m_reader(std::make_shared<ArrowHdfInput>(input_file, writer_creator, product))
  , m_writer_creator(std::move(writer_creator))
  , m_product(std::move(product))
  , m_output_creator(std::move(output_creator))
  , m_output_layer(std::move(output_layer))
  , m_stage(std::move(stage))
  , m_hier(m_reader->scan())  // open + scan once; the reader keeps the handle for reads
{
    if (m_stage.empty() || m_stage == "CURRENT") {
        throw std::invalid_argument(
          "phlex_arrow_hdf source: 'stage' must name the stage that produced the data "
          "(not empty, not the reserved \"CURRENT\")");
    }
    auto job = phlex::data_cell_index::job();
    // ProductLoc carries the canonical "plugin:algorithm" creator form;
    // normalize the configured writer creator the same way.
    const auto writer = phlex_arrow::canonical_creator(m_writer_creator);
    for (auto const& loc : m_hier.root_products) {
        if (loc.creator == writer && loc.product == m_product) {
            m_cells.push_back(job);
            break;
        }
    }
    collect_cells(job, m_hier.cells, writer, m_product, m_cells);
}

phlex::provider_bundles ArrowHdfSource::create_providers(
    const phlex::product_selector& selector)
{
    // The one product this source advertises: the uniform TableGroup, stamped
    // with the configured output creator/suffix/layer and the data's stage.
    // The provider reads the TableGroup for a given cell from the shared open
    // file.  libhdf5 is not thread-safe on one handle, so the provider is serial.
    auto reader = m_reader;
    return phlex_arrow::provide_if_selected<phlex_arrow::TableGroup>(
      selector, m_output_creator, m_product, m_output_layer, m_stage,
      phlex::concurrency::serial,
      [reader](const phlex::data_cell_index& id) { return reader->read(id); });
}

phlex::index_generator ArrowHdfSource::indices()
{
    for (auto const& idx : m_cells) {
        co_yield idx;
    }
}

}  // namespace phlex_arrow_hdf
