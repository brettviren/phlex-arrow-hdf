cmake_minimum_required(VERSION 3.25)
project(phlex_arrow_hdf VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "14.1")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15")
      add_compile_options(-Wno-stringop-overflow)
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16")
      add_compile_options(-Wno-array-bounds)
    endif()
  endif()
endif()

add_compile_options(-Wall -Wpedantic -Werror)

# Dependencies: the pure HDF5 serializer (arrow-hdf), the Phlex glue
# (phlex-arrow-common), Phlex itself, Arrow, HDF5.  Find Phlex's
# un-found-dependency deps (TBB/Boost/spdlog) and Arrow/HDF5 first.
find_package(Arrow CONFIG REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C)
find_package(TBB REQUIRED)
find_package(Boost REQUIRED COMPONENTS json)
find_package(spdlog REQUIRED)
find_package(phlex REQUIRED)
find_package(arrow_hdf CONFIG REQUIRED)
find_package(phlex_arrow_common CONFIG REQUIRED)

# Link the Spack view's gcc15 libstdc++ explicitly (GLIBCXX/CXXABI resolution).
set(PHLEX_ARROW_HDF_RUNTIME_LIBS "")
foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
  if(EXISTS "${_prefix}/lib/libstdc++.so.6")
    file(REAL_PATH "${_prefix}/lib/libstdc++.so.6" _stdc_real)
    get_filename_component(_stdc_dir "${_stdc_real}" DIRECTORY)
    set(PHLEX_ARROW_HDF_RUNTIME_LIBS "${_stdc_real}")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,-rpath,${_stdc_dir}")
    break()
  endif()
endforeach()

# The output logic (unit-testable).
add_library(phlex_arrow_hdf SHARED
  phlex_arrow_hdf/ArrowHdfOutput.cpp
)
target_include_directories(phlex_arrow_hdf
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(phlex_arrow_hdf
  PUBLIC
    arrow_hdf::arrow_hdf
    phlex_arrow_common::phlex_arrow_common
    phlex::model
    Arrow::arrow_shared
    spdlog::spdlog
    ${PHLEX_ARROW_HDF_RUNTIME_LIBS}
)

# The Phlex plugin (dlopen MODULE): loading it registers the HDF5 output node.
add_library(phlex_arrow_hdf_output MODULE
  phlex_arrow_hdf/OutputModule.cpp
)
target_link_libraries(phlex_arrow_hdf_output PRIVATE
  phlex_arrow_hdf
  phlex::module
  ${PHLEX_ARROW_HDF_RUNTIME_LIBS}
)

enable_testing()
add_subdirectory(test)

include(GNUInstallDirs)
install(TARGETS phlex_arrow_hdf
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(TARGETS phlex_arrow_hdf_output
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY phlex_arrow_hdf/
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/phlex_arrow_hdf"
  FILES_MATCHING PATTERN "*.h")
