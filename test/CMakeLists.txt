add_executable(test_output_write test_output_write.cpp)
target_link_libraries(test_output_write PRIVATE
  phlex_arrow_hdf
  arrow_hdf::arrow_hdf
  phlex_arrow_common::phlex_arrow_common
  phlex::model
  Arrow::arrow_shared
  ${PHLEX_ARROW_HDF_RUNTIME_LIBS})
add_test(NAME output_write COMMAND test_output_write)

add_executable(test_input_read test_input_read.cpp)
target_link_libraries(test_input_read PRIVATE
  phlex_arrow_hdf
  arrow_hdf::arrow_hdf
  phlex_arrow_common::phlex_arrow_common
  phlex::model
  Arrow::arrow_shared
  ${PHLEX_ARROW_HDF_RUNTIME_LIBS})
add_test(NAME input_read COMMAND test_input_read)

add_executable(test_source test_source.cpp)
target_link_libraries(test_source PRIVATE
  phlex_arrow_hdf
  arrow_hdf::arrow_hdf
  phlex_arrow_common::phlex_arrow_common
  phlex::model
  phlex::core
  Arrow::arrow_shared
  ${PHLEX_ARROW_HDF_RUNTIME_LIBS})
add_test(NAME source COMMAND test_source)

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

  # Coverage-policy drop scenario (ddm-c3s.8): the output module claims only
  # 'deposet' while the source emits frames; assert the unclaimed 'frame'
  # product is logged as an intentional drop (never silent) and not written.
  add_test(NAME integration_drop
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/integration_drop.sh"
      "${CMAKE_INSTALL_PREFIX}"
      "${PHLEX_VIEW_PREFIX}"
      "${CMAKE_CURRENT_SOURCE_DIR}/integration_drop.jsonnet")

  # End-to-end read-back (ddm-c3s.17): write a file, then read it back through
  # phlex_arrow_hdf_source -> wire_cell_phlex_arrow_from_arrow -> frame_observer.
  # Exercises plugins from phlex-arrow-hdf + wire-cell-phlex-arrow + wire-cell-
  # phlex, so it requires all of them installed under CMAKE_INSTALL_PREFIX.
  add_test(NAME integration_read
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/integration_read.sh"
      "${CMAKE_INSTALL_PREFIX}"
      "${PHLEX_VIEW_PREFIX}"
      "${CMAKE_CURRENT_SOURCE_DIR}/integration_frame_to_hdf.jsonnet"
      "${CMAKE_CURRENT_SOURCE_DIR}/integration_read.jsonnet")
else()
  message(STATUS "phlex executable not found; skipping integration tests")
endif()
