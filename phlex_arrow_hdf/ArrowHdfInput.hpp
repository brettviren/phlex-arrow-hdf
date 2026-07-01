#ifndef PHLEX_ARROW_HDF_ARROWHDFINPUT_HPP
#define PHLEX_ARROW_HDF_ARROWHDFINPUT_HPP

// The HDF5 read side of phlex-arrow-hdf: a single-provider reader that produces
// the uniform Arrow product (phlex_arrow::TableGroup) stored for a given data
// cell.  It is the inverse of ArrowHdfOutput.
//
// Per data cell, the reader: (1) builds the neutral address from the cell index
// + the WRITER's creator + product suffix (phlex-arrow-common index_address);
// (2) turns it into an arrow_hdf::Address here (the technology step); (3) reads
// the member tables + type label (arrow_hdf::read_tables) and assembles a
// TableGroup.  Domain reconstruction (TableGroup -> WCT object) is the convert
// node's job in wire-cell-phlex-arrow, not here.
//
// The creator used for ADDRESSING is the original writer's (supplied by
// configuration); it is not this reader's own Phlex source name.
//
// Concurrency: libhdf5 is not thread-safe; a provider that shares one open file
// must be driven serially (as on the write side).

#include "arrow_hdf/Hdf5File.hpp"

#include "phlex_arrow_common/CellHierarchy.hpp"
#include "phlex_arrow_common/TableGroup.hpp"

#include "phlex/model/data_cell_index.hpp"

#include <optional>
#include <string>

namespace phlex_arrow_hdf {

class ArrowHdfInput {
  public:
    /// `path` is the HDF5 file to read.  `creator`/`product` identify the data
    /// to read: the creator that WROTE it (used to build the stored address)
    /// and the product suffix.
    ArrowHdfInput(std::string path, std::string creator, std::string product);

    /// Produce the TableGroup stored for `index`.  Opens the file read-only on
    /// first use and reuses the handle.  Throws on an HDF5 error.
    phlex_arrow::TableGroup read(const phlex::data_cell_index& index);

    /// Scan the file and interpret its stored addresses as a Phlex cell
    /// hierarchy (the cells + the (creator, product) locations present).  Uses
    /// the SAME open handle as read(), so a source that scans then reads shares
    /// one open file.  Throws on an HDF5 error.
    phlex_arrow::CellHierarchy scan();

  private:
    void ensure_open();  // open m_file read-only on first use; no-op thereafter

    std::string m_path;
    std::string m_creator;
    std::string m_product;
    std::optional<arrow_hdf::Hdf5File> m_file;  // opened lazily, reused across cells
};

}  // namespace phlex_arrow_hdf

#endif  // PHLEX_ARROW_HDF_ARROWHDFINPUT_HPP
