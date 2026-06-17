// Phlex plugin: register the HDF5 Arrow output module.
//
// Phlex plugin loading IS the technology dispatch — loading this module
// registers an HDF5 output node bound to ArrowHdfOutput.  No internal backend
// registry (per the narrow-waist design).  Config keys:
//   output_file (string, required): the HDF5 file to write.
//   products    (array<string>, optional): product suffixes to write; default
//               (empty) writes all Arrow-typed products.

#include "phlex_arrow_hdf/ArrowHdfOutput.hpp"

#include "phlex/module.hpp"

#include <string>
#include <vector>

PHLEX_REGISTER_ALGORITHMS(m, config)
{
    auto const output_file = config.get<std::string>("output_file");
    auto const products = config.get<std::vector<std::string>>("products", {});

    auto node = m.make<phlex_arrow_hdf::ArrowHdfOutput>(output_file, products);
    // Output nodes default to concurrency::serial — required for libhdf5.
    node.output("arrow_hdf_write", &phlex_arrow_hdf::ArrowHdfOutput::write);
}
