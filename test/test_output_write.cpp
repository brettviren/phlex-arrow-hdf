// Integration test for the HDF5 Arrow output module (ddm-c3s.5).
//
// Build a product_store holding an Arrow table at a data cell, run
// ArrowHdfOutput::write, then read the file back with arrow-hdf and check the
// table round-trips and the group path reflects the cell hierarchy.

#include "phlex_arrow_hdf/ArrowHdfOutput.h"

#include "phlex_arrow_common/StoreAddress.h"
#include "phlex_arrow_common/ArrowProducts.h"

#include "arrow_hdf/Hdf5File.h"
#include "arrow_hdf/Address.h"

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

static phlex_arrow::arrow_table_ptr make_toy_table()
{
    arrow::Int32Builder b;
    (void)b.Append(1); (void)b.Append(2); (void)b.Append(3);
    std::shared_ptr<arrow::Array> arr; (void)b.Finish(&arr);
    auto schema = arrow::schema({arrow::field("x", arrow::int32(), false)},
                                arrow::key_value_metadata({{"arrow.schema", "wc.toy"}}));
    return arrow::Table::Make(schema, {arr});
}

int main()
{
    const std::string path = "/tmp/phlex_arrow_hdf_output.h5";

    auto job = phlex::data_cell_index::job();
    auto run = job->make_child("run", 1);
    auto ev = run->make_child("event", 5);
    pe::product_store store(ev, pe::algorithm_name("sim"));

    auto table = make_toy_table();
    store.add_product<phlex_arrow::arrow_table_ptr>(pe::product_specification("toy"), phlex_arrow::arrow_table_ptr(table));

    {
        phlex_arrow_hdf::ArrowHdfOutput out(path);
        out.write(store);
    }  // out destroyed -> HDF5 file closed

    // Read back via arrow-hdf at the same address the module would have used.
    auto f = arrow_hdf::Hdf5File::open_readonly(path).ValueOrDie();
    arrow_hdf::Address addr(phlex_arrow::store_address(store, "toy"));
    auto back = f.read(addr);
    check(back.ok(), "read back the written product");
    if (back.ok()) {
        auto back_table = back.ValueOrDie();
        check(table->Equals(*back_table, /*check_metadata=*/true), "table round-trips");
    }

    // scan() finds exactly that product path, reflecting the cell hierarchy.
    auto h = f.scan().ValueOrDie();
    check(h.product_paths() == std::vector<std::string>{addr.path()}, "scan path = " + addr.path());

    if (fails) { std::cerr << fails << " failures\n"; return 1; }
    std::cout << "output_write OK\n";
    return 0;
}
