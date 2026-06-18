// Unit test for ArrowHdfInput: the single-provider HDF5 read path (ddm-c3s.6).
//
// Writes two data cells' TableGroups at the addresses the read side computes
// (via index_address), then reads each back through ArrowHdfInput and checks
// the type label, member names, the per-cell table contents, and that distinct
// cells yield distinct data.

#include "phlex_arrow_hdf/ArrowHdfInput.hpp"

#include "phlex_arrow_common/StoreAddress.hpp"  // index_address

#include "arrow_hdf/Address.hpp"
#include "arrow_hdf/Hdf5File.hpp"

#include "phlex/model/data_cell_index.hpp"

#include <arrow/api.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace ah = arrow_hdf;
static int fails = 0;
static void check(bool ok, const std::string& w)
{
    std::cout << (ok ? "ok   " : "FAIL ") << w << "\n";
    if (!ok) ++fails;
}

// A one-column toy table holding {v, v+1}.
static std::shared_ptr<arrow::Table> toy(int v)
{
    arrow::Int32Builder b;
    (void)b.Append(v);
    (void)b.Append(v + 1);
    std::shared_ptr<arrow::Array> a;
    (void)b.Finish(&a);
    return arrow::Table::Make(arrow::schema({arrow::field("x", arrow::int32(), false)}), {a});
}

int main()
{
    const std::string path = "/tmp/arrow_hdf_input_read.h5";
    const std::string creator = "writer", product = "frame";

    auto job = phlex::data_cell_index::job();
    auto ev1 = job->make_child("event", 1);
    auto ev2 = job->make_child("event", 2);

    // Write each cell's TableGroup at the address the reader will compute.
    {
        auto f = ah::Hdf5File::create(path).ValueOrDie();
        for (auto const& ev : {ev1, ev2}) {
            ah::Address addr(phlex_arrow::index_address(*ev, creator, product));
            auto n = static_cast<int>(ev->number());
            auto st = f.write_tables(addr, {{"traces", toy(10 * n)}, {"frame_tags", toy(100 * n)}},
                                     "wc.frame");
            check(st.ok(), "write cell " + std::to_string(n));
        }
    }

    // Read back through the provider.
    phlex_arrow_hdf::ArrowHdfInput in(path, creator, product);

    auto g1 = in.read(*ev1);
    check(g1.type == "wc.frame", "cell 1 type label");
    check(g1.members.size() == 2 && g1.members.count("traces") && g1.members.count("frame_tags"),
          "cell 1 member names");
    check(g1.members.count("traces") && toy(10)->Equals(*g1.members.at("traces")),
          "cell 1 'traces' content");
    check(g1.members.count("frame_tags") && toy(100)->Equals(*g1.members.at("frame_tags")),
          "cell 1 'frame_tags' content");

    auto g2 = in.read(*ev2);
    check(g2.members.count("traces") && toy(20)->Equals(*g2.members.at("traces")),
          "cell 2 'traces' content (distinct cell -> distinct data)");

    if (fails) { std::cerr << fails << " failures\n"; return 1; }
    std::cout << "input_read OK\n";
    return 0;
}
