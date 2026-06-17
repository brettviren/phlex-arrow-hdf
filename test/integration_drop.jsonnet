// phlex-arrow-hdf coverage-policy drop scenario (ddm-c3s.8).
//
// The source emits frames, but the output module's claim list is ['deposet'].
// The frame Arrow product is therefore NOT claimed by this module: the module
// must LOG the intentional drop (never silent) and write no file.
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
    hdf_out: { cpp: 'phlex_arrow_hdf_output', output_file: 'dropped.h5', products: ['deposet'] },
  },
}
