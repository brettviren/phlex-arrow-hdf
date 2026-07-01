// Phlex plugin: register the HDF5 Arrow read source (ddm-c3s.18).
//
// Loading this plugin registers an ArrowHdfSource (a phlex::source) under the
// job's "sources" config section.  The framework calls the source's
// create_providers() to implicitly wire a provider that emits the TableGroup
// stored for each data cell (read from HDF5 by ArrowHdfInput); a downstream
// convert node (wire-cell-phlex-arrow) turns the TableGroup back into a WCT
// product.  The source also enumerates the file's cells (indices()/cells()),
// which a file-driven driver (ddm-c3s.16) can use to drive exactly those cells.
//
// This replaces the previous PHLEX_REGISTER_PROVIDERS + m.provide(...)
// .output_product(...) path (the informal 0.2.0 provider builder): the read
// path now uses only the official 0.3.0 source extension point.
//
// Config keys (under "sources"):
//   input_file     (string, required): the HDF5 file to read.
//   creator        (string, required): the creator that WROTE the data (used to
//                  locate it in the file — the on-disk address).
//   product        (string, required): the product suffix to read; also the
//                  suffix of the product this source emits.
//   output_layer   (string, required): the Phlex layer to emit into.
//   output_creator (string, optional, default "input"): the creator label on
//                  the emitted product (what downstream nodes route on).

#include "phlex_arrow_hdf/ArrowHdfSource.hpp"

#include "phlex/configuration.hpp"
#include "phlex/source.hpp"

#include <string>

PHLEX_REGISTER_SOURCE(s, config)
{
    auto const input_file = config.get<std::string>("input_file");
    auto const creator = config.get<std::string>("creator");
    auto const product = config.get<std::string>("product");
    auto const output_layer = config.get<std::string>("output_layer");
    auto const output_creator = config.get<std::string>("output_creator", std::string{"input"});

    // Register under the source's configuration label (the "sources" section key,
    // supplied by the framework as module_label) so a driver's `uses_sources`
    // can refer to this source by that name.
    auto const label = config.get<std::string>("module_label", std::string{"arrow_hdf_read"});

    s.add_source<phlex_arrow_hdf::ArrowHdfSource>(
      label, input_file, creator, product, output_creator, output_layer);
}
