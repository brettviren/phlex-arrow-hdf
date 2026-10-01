// Unit test for ArrowHdfSource: the Phlex read source (ddm-c3s.18; Phlex 0.4.1).
//
// Writes two data cells' TableGroups (as the write path would), then exercises
// the source directly (no framework/driver needed):
//   * indices() enumerates exactly the two cells present;
//   * create_providers() returns one provider for a matching product_selector,
//     and none for a non-matching one; and
//   * the returned provider reads back the correct TableGroup per cell.

#include "phlex_arrow_hdf/ArrowHdfSource.hpp"

#include "phlex_arrow_common/StoreAddress.hpp"  // index_address
#include "phlex_arrow_common/TableGroup.hpp"

#include "arrow_hdf/Address.hpp"
#include "arrow_hdf/Hdf5File.hpp"

#include "phlex/model/data_cell_index.hpp"
#include "phlex/source.hpp"  // phlex::product_selector, populate_types

#include <arrow/api.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace ah = arrow_hdf;
static int fails = 0;
static void check(bool ok, const std::string& w)
{
    std::cout << (ok ? "ok   " : "FAIL ") << w << "\n";
    if (!ok) ++fails;
}

static std::shared_ptr<arrow::Table> toy(int v)
{
    arrow::Int32Builder b;
    (void)b.Append(v);
    (void)b.Append(v + 1);
    std::shared_ptr<arrow::Array> a;
    (void)b.Finish(&a);
    return arrow::Table::Make(arrow::schema({arrow::field("x", arrow::int32(), false)}), {a});
}

// Extract the TableGroup a provider produced for a cell.
template <typename ProductPtr>
static const phlex_arrow::TableGroup& group_of(const ProductPtr& p)
{
    return *static_cast<const phlex_arrow::TableGroup*>(p->address());
}

// A selector as the framework hands it to create_providers(): with the
// consumer's argument type (TableGroup) stamped on by phlex::populate_types.
static phlex::product_selector typed(phlex::product_selector sel)
{
    std::vector<phlex::product_selector> sels{std::move(sel)};
    phlex::populate_types<std::tuple<phlex_arrow::TableGroup>>(sels);
    return sels.front();
}

int main()
{
    const std::string path = "/tmp/arrow_hdf_source_test.h5";
    const std::string writer = "writer", product = "frame";

    auto job = phlex::data_cell_index::job();
    auto ev1 = job->make_child("event", 1);
    auto ev2 = job->make_child("event", 2);

    {
        auto f = ah::Hdf5File::create(path).ValueOrDie();
        for (auto const& ev : {ev1, ev2}) {
            ah::Address addr(phlex_arrow::index_address(*ev, writer, product));
            auto n = static_cast<int>(ev->number());
            auto st = f.write_tables(addr, {{"traces", toy(10 * n)}, {"frame_tags", toy(100 * n)}},
                                     "wc.frame");
            check(st.ok(), "write cell " + std::to_string(n));
        }
    }

    // The source: writer_creator="writer" (addressing), advertised as
    // output_creator="input" in layer "event".
    phlex_arrow_hdf::ArrowHdfSource src(path, writer, product, "input", "event", "sim");

    // indices() must enumerate exactly the two written cells.
    std::vector<std::size_t> nums;
    for (auto const& idx : src.indices()) {
        nums.push_back(idx->number());
    }
    std::ranges::sort(nums);
    check(nums == std::vector<std::size_t>({1, 2}), "indices() enumerates the two file cells");
    check(src.cells().size() == 2, "cells() view matches");

    // A matching selector -> exactly one provider that reads the right data.
    auto const match_sel = typed({.creator = "input", .layer = "event", .suffix = "frame"});
    auto bundles = src.create_providers(match_sel);
    check(bundles.size() == 1, "create_providers: one bundle for a matching selector");
    if (bundles.size() == 1) {
        check(bundles[0].layer == "event", "bundle layer");
        check(bundles[0].stage == "sim", "bundle stage (the configured data stage)");
        // Keep each product_ptr alive while inspecting the TableGroup it owns.
        auto p1 = bundles[0].provider_function(*ev1);
        auto const& g1 = group_of(p1);
        check(g1.type == "wc.frame", "provider cell 1 type label");
        check(g1.members.count("traces") && toy(10)->Equals(*g1.members.at("traces")),
              "provider cell 1 'traces' content");
        auto p2 = bundles[0].provider_function(*ev2);
        auto const& g2 = group_of(p2);
        check(g2.members.count("traces") && toy(20)->Equals(*g2.members.at("traces")),
              "provider cell 2 content (distinct cell -> distinct data)");
    }

    // A non-matching selector (wrong suffix) -> no providers.
    auto const miss_sel = typed({.creator = "input", .layer = "event", .suffix = "not_frame"});
    check(src.create_providers(miss_sel).empty(),
          "create_providers: no bundle for a non-matching selector");
    check(src.create_providers(typed({.layer = "event", .suffix = "frame", .stage = "reco"})).empty(),
          "create_providers: no bundle for another stage");

    bool threw = false;
    try {
        phlex_arrow_hdf::ArrowHdfSource bad(path, writer, product, "input", "event", "CURRENT");
    } catch (std::invalid_argument const&) {
        threw = true;
    }
    check(threw, "stage \"CURRENT\" is rejected");

    if (fails) { std::cerr << fails << " failures\n"; return 1; }
    std::cout << "source OK\n";
    return 0;
}
