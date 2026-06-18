#include "phlex_arrow_hdf/ArrowHdfInput.hpp"

#include "phlex_arrow_common/StoreAddress.hpp"  // index_address

#include "arrow_hdf/Address.hpp"

#include <stdexcept>
#include <utility>

namespace phlex_arrow_hdf {

ArrowHdfInput::ArrowHdfInput(std::string path, std::string creator, std::string product)
  : m_path(std::move(path)), m_creator(std::move(creator)), m_product(std::move(product))
{
}

phlex_arrow::TableGroup ArrowHdfInput::read(const phlex::data_cell_index& index)
{
    if (!m_file) {
        auto f = arrow_hdf::Hdf5File::open_readonly(m_path);
        if (!f.ok()) {
            throw std::runtime_error("ArrowHdfInput: open " + m_path + ": " + f.status().ToString());
        }
        m_file.emplace(std::move(*f));
    }

    // (1)+(2) neutral address from the cell + writer's creator + product, then
    // build the technology path here.
    arrow_hdf::Address addr(phlex_arrow::index_address(index, m_creator, m_product));

    // (3) read the member tables + type label and assemble the TableGroup.
    auto nt = m_file->read_tables(addr);
    if (!nt.ok()) {
        throw std::runtime_error("ArrowHdfInput: read_tables " + addr.path() + ": " +
                                 nt.status().ToString());
    }
    auto& g = *nt;
    return phlex_arrow::TableGroup{std::move(g.type_label), std::move(g.tables)};
}

}  // namespace phlex_arrow_hdf
