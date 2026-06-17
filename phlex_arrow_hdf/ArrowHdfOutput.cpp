#include "phlex_arrow_hdf/ArrowHdfOutput.h"

#include "phlex_arrow_common/ArrowProducts.h"
#include "phlex_arrow_common/StoreAddress.h"
#include "phlex_arrow_common/TableGroup.h"

#include "arrow_hdf/Address.h"

#include "phlex/model/product_specification.hpp"

#include <stdexcept>
#include <string_view>
#include <utility>

namespace phlex_arrow_hdf {

ArrowHdfOutput::ArrowHdfOutput(std::string path, std::vector<std::string> suffixes)
  : m_path(std::move(path)), m_suffixes(std::move(suffixes))
{
}

void ArrowHdfOutput::write(const phlex::experimental::product_store& store)
{
    if (store.empty()) return;

    // (1) select the Arrow-typed products (configured subset, or all).
    auto specs = m_suffixes.empty() ? phlex_arrow::select_arrow_products(store)
                                    : phlex_arrow::select_arrow_products(store, m_suffixes);
    if (specs.empty()) return;

    // Create the file lazily on the first store that has products to write, and
    // reuse the open handle for subsequent stores.
    if (!m_file) {
        auto f = arrow_hdf::Hdf5File::create(m_path);
        if (!f.ok()) {
            throw std::runtime_error("ArrowHdfOutput: create " + m_path + ": " + f.status().ToString());
        }
        m_file.emplace(std::move(*f));
    }

    for (const auto& spec : specs) {
        const std::string product{static_cast<std::string_view>(spec.suffix())};

        // (2) address from the full structured store index (neutral components),
        // then build the technology-specific path here.
        arrow_hdf::Address addr(phlex_arrow::store_address(store, product));

        // (3) translate the TableGroup down to primitive arrow-hdf calls: each
        // member table becomes a child group under the product's parent group.
        const auto& group = store.get_product<phlex_arrow::TableGroup>(spec);
        auto st = m_file->write_tables(addr, group.members, group.type);
        if (!st.ok()) {
            throw std::runtime_error("ArrowHdfOutput: write " + addr.path() + ": " + st.ToString());
        }
    }
}

}  // namespace phlex_arrow_hdf
