# phlex-arrow-hdf

The thin per-technology Phlex glue for the **HDF5 write path** of the
narrow-waist file I/O design. Loading this plugin registers an HDF5 output
module that persists Arrow products by delegating to `arrow-hdf`.

- `phlex_arrow_hdf/ArrowHdfOutput.{h,cpp}` — the output logic (unit-testable):
  per store, select Arrow products (phlex-arrow-common), address each from the
  full structured `store.index()` (neutral components → `arrow_hdf::Address`
  here), and delegate the write to `arrow_hdf::Hdf5File`. Lazy single open file;
  registered at `concurrency::serial` (libhdf5 is not thread-safe).
- `phlex_arrow_hdf/OutputModule.cpp` — the Phlex plugin (`PHLEX_REGISTER_ALGORITHMS`,
  built as a dlopen MODULE `libphlex_arrow_hdf_output.so`). Config: `output_file`
  (required), `products` (optional suffix subset; default all Arrow products).

Dependencies: `arrow-hdf` (write path + Address), `phlex-arrow-common`
(write-side helpers), Phlex, Arrow, HDF5.

## Build

```bash
export CC="$(spack -e wcph location -i gcc@15)/bin/gcc"
export CXX="$(spack -e wcph location -i gcc@15)/bin/g++"
cmake --preset default -S source/phlex-arrow-hdf -B builds/phlex-arrow-hdf
cmake --build builds/phlex-arrow-hdf
ctest --test-dir builds/phlex-arrow-hdf
```

Requires `arrow-hdf` and `phlex-arrow-common` dev-installed to `/devel/ddm/install`.
