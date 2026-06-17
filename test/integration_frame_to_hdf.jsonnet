// phlex-arrow-hdf write-path integration workflow.
//
// Exercises the full N+M narrow-waist write chain in one Phlex graph:
//
//   wcp_frame_source            (wire-cell-phlex)        -> wcphlex::Frame
//     -> wire_cell_phlex_arrow_convert (wire-cell-phlex-arrow) -> TableGroup
//       -> phlex_arrow_hdf_output      (phlex-arrow-hdf)       -> HDF5 file
//
// The generate_layers driver makes two `event` data cells (numbers 1,2) under
// `job`; the source emits one synthetic frame per cell; the convert node turns
// each into a wc.frame TableGroup; the output module persists each under a
// group path reflecting the data-cell hierarchy.
//
// Run (plugins + their dependent libs must be on PHLEX_PLUGIN_PATH and the
// loader path):
//   PHLEX_PLUGIN_PATH=<prefix>/lib:<view>/lib \
//   LD_LIBRARY_PATH=<prefix>/lib:<view>/lib \
//   phlex -c integration_frame_to_hdf.jsonnet
{
  driver: {
    cpp: 'generate_layers',
    layers: { event: { parent: 'job', total: 2, starting_number: 1 } },
  },
  sources: {
    frame_source: { cpp: 'wcp_frame_source', output_layer: 'event' },
  },
  modules: {
    to_arrow: { cpp: 'wire_cell_phlex_arrow_convert', input_layer: 'event', types: ['frame'] },
    hdf_out: { cpp: 'phlex_arrow_hdf_output', output_file: 'integration_out.h5' },
  },
}
