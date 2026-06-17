add_executable(test_output_write test_output_write.cpp)
target_link_libraries(test_output_write PRIVATE
  phlex_arrow_hdf
  arrow_hdf::arrow_hdf
  phlex_arrow_common::phlex_arrow_common
  phlex::model
  Arrow::arrow_shared
  ${PHLEX_ARROW_HDF_RUNTIME_LIBS})
add_test(NAME output_write COMMAND test_output_write)
