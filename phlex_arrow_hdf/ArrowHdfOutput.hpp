#ifndef PHLEX_ARROW_HDF_ARROWHDFOUTPUT_H
#define PHLEX_ARROW_HDF_ARROWHDFOUTPUT_H

// The HDF5 write side of phlex-arrow-hdf: a thin Phlex output module that
// persists Arrow products to HDF5 by delegating to arrow-hdf.
//
// The logic lives in this class (so it is unit-testable); a tiny plugin
// translation unit registers it as a Phlex output node (see OutputModule.cpp).
//
// Per store, the module: (1) SELECTS the Arrow-typed products (configured
// subset) via phlex-arrow-common; (2) ADDRESSES each from the full structured
// store.index() + source + suffix (phlex-arrow-common, neutral components),
// turning the components into an arrow_hdf::Address here (the technology step);
// (3) DELEGATES the write to arrow_hdf::Hdf5File.
//
// Concurrency: register at concurrency::serial (libhdf5 is not thread-safe).
// This class does no internal locking; the serial output node provides it.

#include "arrow_hdf/Hdf5File.hpp"

#include "phlex_arrow_common/CoveragePolicy.hpp"
#include "phlex_arrow_common/PhlexTypes.hpp"

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace phlex_arrow_hdf {

class ArrowHdfOutput {
  public:
    /// `path` is the output HDF5 file. `suffixes` optionally restricts which
    /// Arrow products to write (by product suffix); empty = all Arrow products.
    explicit ArrowHdfOutput(std::string path, std::vector<std::string> suffixes = {});

    /// Phlex output entry point: persist this store's selected Arrow products.
    /// (Signature is void(product_store const&).)  Throws on an HDF5 error.
    void write(const phlex_arrow::product_store& store);

    /// This module's coverage claim (its configured suffixes; empty = wildcard,
    /// i.e. claims all Arrow products).  A job-level validator composes the
    /// claims of all output modules and runs phlex_arrow::enforce_coverage to
    /// catch unclaimed or double-claimed products before any data flows.
    phlex_arrow::ModuleClaim claim() const { return {m_path, m_suffixes}; }

  private:
    std::string m_path;
    std::vector<std::string> m_suffixes;
    std::optional<arrow_hdf::Hdf5File> m_file;   // created lazily, reused across stores
    std::set<std::string> m_logged_drops;        // suffixes already reported as dropped (log once)
};

}  // namespace phlex_arrow_hdf

#endif  // PHLEX_ARROW_HDF_ARROWHDFOUTPUT_H
