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
# Resolve THIS compiler's libstdc++ directly, independent of CMAKE_PREFIX_PATH
# order. A Spack build can place an older gcc-runtime earlier on the path; the
# previous prefix scan grabbed that, linking gcc@15 objects against gcc@11
# libstdc++ (missing GLIBCXX_3.4.31/.32). The compiler always yields its own.
execute_process(
  COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libstdc++.so
  OUTPUT_VARIABLE _stdc_real OUTPUT_STRIP_TRAILING_WHITESPACE)
file(REAL_PATH "${_stdc_real}" _stdc_real)
get_filename_component(_stdc_dir "${_stdc_real}" DIRECTORY)
set(PHLEX_ARROW_HDF_RUNTIME_LIBS "${_stdc_real}")
link_directories(BEFORE "${_stdc_dir}")
string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,-rpath,${_stdc_dir}")

# The output + input logic (unit-testable).
add_library(phlex_arrow_hdf SHARED
  phlex_arrow_hdf/ArrowHdfOutput.cpp
  phlex_arrow_hdf/ArrowHdfInput.cpp
  phlex_arrow_hdf/ArrowHdfSource.cpp
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
    phlex::core        # ArrowHdfSource: phlex::source, product_selector::match
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

# The Phlex read source plugin (dlopen MODULE): loading it registers the HDF5
# single-provider read source.
add_library(phlex_arrow_hdf_source MODULE
  phlex_arrow_hdf/SourceModule.cpp
)
target_link_libraries(phlex_arrow_hdf_source PRIVATE
  phlex_arrow_hdf
  phlex::module
  ${PHLEX_ARROW_HDF_RUNTIME_LIBS}
)

enable_testing()
add_subdirectory(test)

include(GNUInstallDirs)
# phlex-arrow-hdf is a PURE PLUGIN package: it produces a Phlex output plugin
# (dlopen MODULE) plus the implementation library it links.  Its headers are
# PRIVATE and are deliberately NOT installed and NO CMake Config is exported —
# nothing should build against this "final" package; loading the plugin is its
# only interface.  The implementation .so is still installed because the plugin
# links it at runtime.
install(TARGETS phlex_arrow_hdf
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(TARGETS phlex_arrow_hdf_output
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(TARGETS phlex_arrow_hdf_source
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
