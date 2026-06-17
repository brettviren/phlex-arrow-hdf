// Integration test for the HDF5 Arrow output module (ddm-c3s.5).
//
// Build a product_store holding a TableGroup (two members) at a data cell, run
// ArrowHdfOutput::write, then read the members back with arrow-hdf and check
// they round-trip and the group paths reflect the cell hierarchy.

#include "phlex_arrow_hdf/ArrowHdfOutput.hpp"

#include "phlex_arrow_common/StoreAddress.hpp"
#include "phlex_arrow_common/TableGroup.hpp"

#include "arrow_hdf/Hdf5File.hpp"
#include "arrow_hdf/Address.hpp"

#include "phlex/model/data_cell_index.hpp"
#include "phlex/model/product_store.hpp"
#include "phlex/model/algorithm_name.hpp"
#include "phlex/model/product_specification.hpp"

#include <arrow/api.h>

#include <iostream>
#include <string>
#include <vector>

namespace pe = phlex::experimental;
static int fails = 0;
static void check(bool ok, const std::string& what)
{
    std::cout << (ok ? "ok   " : "FAIL ") << what << "\n";
    if (!ok) ++fails;
}

static phlex_arrow::arrow_table_ptr toy(int v, const std::string& schema_name)
{
    arrow::Int32Builder b; (void)b.Append(v); (void)b.Append(v + 1);
    std::shared_ptr<arrow::Array> arr; (void)b.Finish(&arr);
    auto schema = arrow::schema({arrow::field("x", arrow::int32(), false)},
                                arrow::key_value_metadata({{"arrow.schema", schema_name}}));
    return arrow::Table::Make(schema, {arr});
}

int main()
{
    const std::string path = "/tmp/phlex_arrow_hdf_output.h5";

    auto job = phlex::data_cell_index::job();
    auto run = job->make_child("run", 1);
    auto ev = run->make_child("event", 5);
    pe::product_store store(ev, pe::algorithm_name("sim"));

    // A frame-like aggregate: two members.
    auto traces = toy(10, "wc.frame");
    auto frame_tags = toy(20, "wc.frame.frame_tags");
    phlex_arrow::TableGroup group{"wc.frame", {{"traces", traces}, {"frame_tags", frame_tags}}};
    store.add_product<phlex_arrow::TableGroup>(pe::product_specification("frame"), std::move(group));

    {
        phlex_arrow_hdf::ArrowHdfOutput out(path);
        out.write(store);
    }  // out destroyed -> HDF5 file closed

    auto f = arrow_hdf::Hdf5File::open_readonly(path).ValueOrDie();
    arrow_hdf::Address base(phlex_arrow::store_address(store, "frame"));  // /run/1/event/5/<creator>/frame

    auto rt = f.read(arrow_hdf::Address(base.path() + "/traces"));
    auto rf = f.read(arrow_hdf::Address(base.path() + "/frame_tags"));
    check(rt.ok() && traces->Equals(**&rt.ValueOrDie(), true), "member 'traces' round-trips");
    check(rf.ok() && frame_tags->Equals(**&rf.ValueOrDie(), true), "member 'frame_tags' round-trips");

    auto h = f.scan().ValueOrDie();
    check(h.product_paths() == std::vector<std::string>{base.path() + "/frame_tags", base.path() + "/traces"},
          "scan finds both members under " + base.path());

    if (fails) { std::cerr << fails << " failures\n"; return 1; }
    std::cout << "output_write OK\n";
    return 0;
}
