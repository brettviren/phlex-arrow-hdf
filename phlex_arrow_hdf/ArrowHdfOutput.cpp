#include "phlex_arrow_hdf/ArrowHdfOutput.hpp"

#include "phlex_arrow_common/ArrowProducts.hpp"
#include "phlex_arrow_common/StoreAddress.hpp"
#include "phlex_arrow_common/TableGroup.hpp"

#include "arrow_hdf/Address.hpp"

#include "phlex/model/product_specification.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace phlex_arrow_hdf {

namespace {

// The package's logger.  A phlex plugin must NOT log through spdlog's global
// DEFAULT logger: in a phlex+WCT process, WireCell hijacks the default logger
// (set_default_logger to its "wct" logger, which has no sinks until WCT logging
// is configured), silencing any default-logger output that runs after a
// WCT-pulling plugin loads.  Since spdlog's registry IS shared correctly across
// the binary and all plugins (one libspdlog.so, SPDLOG_COMPILED_LIB), the safe
// pattern is to OWN a named logger with an explicit sink, fetched-or-created
// once.  See beads ddm-jj1.
spdlog::logger& logger()
{
    static std::shared_ptr<spdlog::logger> lg = [] {
        if (auto existing = spdlog::get("phlex-arrow-hdf")) return existing;
        return spdlog::stderr_color_mt("phlex-arrow-hdf");
    }();
    return *lg;
}

}  // namespace

ArrowHdfOutput::ArrowHdfOutput(std::string path, std::vector<std::string> suffixes)
  : m_path(std::move(path)), m_suffixes(std::move(suffixes))
{
}

void ArrowHdfOutput::write(const phlex::experimental::product_store& store)
{
    if (store.empty()) return;

    // (1) select the Arrow-typed products (configured subset, or all).  When a
    // claim list is configured, report any Arrow product we are NOT persisting
    // so an intentional drop is never silent (logged once per distinct suffix).
    auto specs = phlex_arrow::select_arrow_products(store);
    if (!m_suffixes.empty()) {
        std::vector<phlex::experimental::product_specification> claimed;
        for (const auto& spec : specs) {
            const std::string suffix{static_cast<std::string_view>(spec.suffix())};
            const bool mine =
              std::find(m_suffixes.begin(), m_suffixes.end(), suffix) != m_suffixes.end();
            if (mine) {
                claimed.push_back(spec);
            }
            else if (m_logged_drops.insert(suffix).second) {
                logger().info("phlex-arrow-hdf[{}]: not persisting Arrow product '{}' "
                              "(not in this module's claim list)",
                              m_path, suffix);
            }
        }
        specs = std::move(claimed);
    }
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
