add_executable(test_output_write test_output_write.cpp)
target_link_libraries(test_output_write PRIVATE
  phlex_arrow_hdf
  arrow_hdf::arrow_hdf
  phlex_arrow_common::phlex_arrow_common
  phlex::model
  Arrow::arrow_shared
  ${PHLEX_ARROW_HDF_RUNTIME_LIBS})
add_test(NAME output_write COMMAND test_output_write)

# Cross-package write-path integration test: drives the `phlex` binary over a
# workflow chaining wcp_frame_source -> wire_cell_phlex_arrow_convert ->
# phlex_arrow_hdf_output, then validates the produced HDF5 layout (data-cell
# hierarchy, the four reserved wc.frame member groups, and the wc.frame type
# marker).  Requires all three plugins (and their shared libs) installed under
# CMAKE_INSTALL_PREFIX, so run it after `cmake --install` of wire-cell-phlex,
# wire-cell-phlex-arrow, and this package.  The Spack view (providing the phlex
# binary, h5ls/h5dump, and the HDF5/Arrow runtime) is inferred from the
# CMAKE_PREFIX_PATH entry that provides bin/phlex.
find_program(PHLEX_EXECUTABLE phlex)
if(PHLEX_EXECUTABLE)
  get_filename_component(_phlex_bindir "${PHLEX_EXECUTABLE}" DIRECTORY)
  get_filename_component(PHLEX_VIEW_PREFIX "${_phlex_bindir}" DIRECTORY)
  add_test(NAME integration_write
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/integration_write.sh"
      "${CMAKE_INSTALL_PREFIX}"
      "${PHLEX_VIEW_PREFIX}"
      "${CMAKE_CURRENT_SOURCE_DIR}/integration_frame_to_hdf.jsonnet")
else()
  message(STATUS "phlex executable not found; skipping integration_write test")
endif()
