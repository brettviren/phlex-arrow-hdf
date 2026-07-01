#include "phlex_arrow_hdf/ArrowHdfSource.hpp"

#include "phlex_arrow_common/PhlexTypes.hpp"  // phlex_arrow:: aliases for the verboten model types

#include "phlex/concurrency.hpp"
#include "phlex/model/products.hpp"   // phlex::detail::product_for / product_ptr / provider_function_t
#include "phlex/model/type_id.hpp"    // phlex::detail::make_type_id

#include <cstddef>
#include <functional>
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
                               std::string output_layer)
  : m_reader(std::make_shared<ArrowHdfInput>(input_file, writer_creator, product))
  , m_writer_creator(std::move(writer_creator))
  , m_product(std::move(product))
  , m_output_creator(std::move(output_creator))
  , m_output_layer(std::move(output_layer))
  , m_hier(m_reader->scan())  // open + scan once; the reader keeps the handle for reads
{
    auto job = phlex::data_cell_index::job();
    for (auto const& loc : m_hier.root_products) {
        if (loc.creator == m_writer_creator && loc.product == m_product) {
            m_cells.push_back(job);
            break;
        }
    }
    collect_cells(job, m_hier.cells, m_writer_creator, m_product, m_cells);
}

phlex::detail::provider_bundles ArrowHdfSource::create_providers(
    const phlex::product_selector& selector)
{
    phlex::detail::provider_bundles bundles;

    // The one product this source advertises: the uniform TableGroup, stamped
    // with the configured output creator/suffix/layer and the "CURRENT" stage
    // (matching the prior output_product() behaviour).
    phlex_arrow::product_specification spec{phlex_arrow::algorithm_name{m_output_creator},
                                            phlex_arrow::identifier{m_product},
                                            phlex::detail::make_type_id<phlex_arrow::TableGroup>()};
    phlex_arrow::identifier const layer{m_output_layer};
    phlex_arrow::identifier const stage{std::string_view{"CURRENT"}};

    if (!selector.match(spec, layer, stage)) {
        return bundles;  // this source cannot satisfy the request
    }

    // The provider reads the TableGroup for a given cell from the shared open
    // file.  libhdf5 is not thread-safe on one handle, so the provider is serial.
    auto reader = m_reader;
    auto provider = [reader](const phlex::data_cell_index& id) -> phlex::detail::product_ptr {
        return phlex::detail::product_for(reader->read(id));
    };

    bundles.push_back(phlex::detail::provider_bundle{
      .provider_function = std::function<phlex::detail::provider_function_t>(std::move(provider)),
      .max_concurrency = phlex::concurrency::serial,
      .spec = std::move(spec),
      .layer = m_output_layer,
      .stage = "CURRENT"});
    return bundles;
}

phlex::index_generator ArrowHdfSource::indices()
{
    for (auto const& idx : m_cells) {
        co_yield idx;
    }
}

}  // namespace phlex_arrow_hdf
