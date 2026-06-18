// Phlex plugin: register the HDF5 Arrow read source (single provider).
//
// Loading this plugin registers a source whose single provider emits the
// TableGroup stored for each data cell, read from an HDF5 file by
// ArrowHdfInput.  A downstream convert node (wire-cell-phlex-arrow) turns the
// TableGroup back into a WCT product.
//
// Config keys:
//   input_file     (string, required): the HDF5 file to read.
//   creator        (string, required): the creator that WROTE the data (used to
//                  locate it in the file — the on-disk address).
//   product        (string, required): the product suffix to read; also the
//                  suffix of the product this source emits.
//   output_layer   (string, required): the Phlex layer to emit into.
//   output_creator (string, optional, default "input"): the creator label on
//                  the emitted product (what downstream nodes route on).
//
// NOTE: this provides products only for the data cells the driver yields; a
// driver that auto-discovers the file's cells is the deferred ddm-c3s.16 work.

#include "phlex_arrow_hdf/ArrowHdfInput.hpp"

#include "phlex_arrow_common/PhlexTypes.hpp"  // phlex_arrow::identifier

#include "phlex/configuration.hpp"
#include "phlex/model/data_cell_index.hpp"
#include "phlex/source.hpp"

#include <memory>
#include <string>

using namespace phlex;

PHLEX_REGISTER_PROVIDERS(m, config)
{
    auto const input_file = config.get<std::string>("input_file");
    auto const creator = config.get<std::string>("creator");
    auto const product = config.get<std::string>("product");
    auto const output_layer = config.get<std::string>("output_layer");
    auto const output_creator = config.get<std::string>("output_creator", std::string{"input"});

    auto reader = std::make_shared<phlex_arrow_hdf::ArrowHdfInput>(input_file, creator, product);

    m.provide("arrow_hdf_read",
              [reader](data_cell_index const& id) -> phlex_arrow::TableGroup {
                  return reader->read(id);
              })
      .output_product({.creator = output_creator,
                       .layer = output_layer,
                       .suffix = phlex_arrow::identifier{product}});
}
