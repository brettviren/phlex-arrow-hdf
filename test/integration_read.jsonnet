// phlex-arrow-hdf read-path integration workflow (ddm-c3s.17).
//
// Reads back the file produced by integration_frame_to_hdf.jsonnet
// (integration_out.h5, written by module instance "to_arrow" -> creator
// "to_arrow:wc_frame_to_arrow", product "frame", event cells 1..2) through the
// full single-provider read chain:
//
//   phlex_arrow_hdf_driver (drives exactly the file's event cells — no counts)
//     -> phlex_arrow_hdf_source        (reads HDF5 -> phlex_arrow::TableGroup)
//       -> wire_cell_phlex_arrow_from_arrow (TableGroup -> wcphlex::Frame)
//         -> wcp_frame_observer        (asserts each reconstructed Frame is non-null)
//
// The driver receives the SAME ArrowHdfSource instance (named in uses_sources)
// and yields exactly the data cells that source discovered by scanning the file
// (ddm-c3s.16), with NO preconfigured cell count — only the layer shape.  The
// from_arrow transform is the CONSUMER that creates demand for the source's
// product (a Phlex provider only runs when something demands its product); the
// observer is the end-cap that creates demand for the reconstructed Frame.
// frame_observer asserts (NDEBUG undef'd) so a null/missing frame aborts.
{
  driver: {
    cpp: 'phlex_arrow_hdf_driver',
    uses_sources: ['reader'],  // drive the cells this source found
    layers: ['event'],         // layer shape only; the source supplies the cells
  },
  sources: {
    reader: {
      cpp: 'phlex_arrow_hdf_source',
      input_file: 'integration_out.h5',
      creator: 'to_arrow:wc_frame_to_arrow',  // the writer's creator (addressing)
      product: 'frame',
      output_layer: 'event',
      output_creator: 'input',  // label on the emitted TableGroup
    },
  },
  modules: {
    from_arrow: {
      cpp: 'wire_cell_phlex_arrow_from_arrow',
      input_creator: 'input',
      input_layer: 'event',
      types: ['frame'],
    },
    observer: {
      cpp: 'wcp_frame_observer',
      input_layer: 'event',
      input_from: 'from_arrow',  // the convert module's instance name
    },
  },
}
