#include "phlex_arrow_hdf/ArrowHdfInput.hpp"

#include "phlex_arrow_common/StoreAddress.hpp"  // index_address

#include "arrow_hdf/Address.hpp"    // arrow_hdf::Address, path_unescape
#include "arrow_hdf/Hierarchy.hpp"  // arrow_hdf::Hierarchy (scan result)

#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace phlex_arrow_hdf {

ArrowHdfInput::ArrowHdfInput(std::string path, std::string creator, std::string product)
  : m_path(std::move(path)), m_creator(std::move(creator)), m_product(std::move(product))
{
}

void ArrowHdfInput::ensure_open()
{
    if (m_file) return;
    auto f = arrow_hdf::Hdf5File::open_readonly(m_path);
    if (!f.ok()) {
        throw std::runtime_error("ArrowHdfInput: open " + m_path + ": " + f.status().ToString());
    }
    m_file.emplace(std::move(*f));
}

phlex_arrow::TableGroup ArrowHdfInput::read(const phlex::data_cell_index& index)
{
    ensure_open();

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

phlex_arrow::CellHierarchy ArrowHdfInput::scan()
{
    ensure_open();

    auto h = m_file->scan();
    if (!h.ok()) {
        throw std::runtime_error("ArrowHdfInput: scan " + m_path + ": " + h.status().ToString());
    }

    // scan() reports one path per stored MEMBER table, i.e.
    // "/layer0/number0/.../creator/product/<member>" (components escaped): a
    // product group holds one child object per member (the type label is an
    // attribute on the group, not a child, so it is not reported).  Recover the
    // product ADDRESS by dropping the trailing member component, then dedupe the
    // members of the same product to one address.  Split on '/', drop empties,
    // path_unescape each.  This is the technology step (the layering rule keeps
    // it out of arrow-hdf and phlex-arrow-common).
    std::set<std::vector<std::string>> bases;
    for (auto const& path : h->product_paths()) {
        std::vector<std::string> comps;
        for (std::size_t i = 0; i < path.size();) {
            if (path[i] == '/') { ++i; continue; }
            auto j = path.find('/', i);
            if (j == std::string::npos) j = path.size();
            comps.push_back(arrow_hdf::path_unescape(path.substr(i, j - i)));
            i = j;
        }
        if (comps.empty()) continue;  // ignore anything with no member component
        comps.pop_back();             // drop the member name -> the product address
        bases.insert(std::move(comps));
    }
    std::vector<std::vector<std::string>> raw(bases.begin(), bases.end());
    return phlex_arrow::CellHierarchy::from_addresses(raw);
}

}  // namespace phlex_arrow_hdf
