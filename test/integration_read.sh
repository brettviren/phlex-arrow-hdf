#!/usr/bin/env bash
#
# phlex-arrow-hdf end-to-end read-back integration test (ddm-c3s.17).
#
# Writes an HDF5 file with the full write chain, then reads it back through the
# single-provider read chain (phlex_arrow_hdf_source -> from_arrow transform ->
# wcp_frame_observer) and asserts the reconstructed frames are valid.  The
# observer aborts (assert) on a null/missing frame, so a clean exit means every
# cell's frame was read from HDF5 and reconstructed.
#
# Usage: integration_read.sh <prefix> <view> <write_jsonnet> <read_jsonnet>
#   (see integration_write.sh for the prefix/view convention)

set -euo pipefail

prefix=$(realpath "${1:?prefix}")
view=$(realpath "${2:?view}")
write_jsonnet=$(realpath "${3:?write_jsonnet}")
read_jsonnet=$(realpath "${4:?read_jsonnet}")

export PATH="${view}/bin:${PATH}"
export PHLEX_PLUGIN_PATH="${prefix}/lib:${view}/lib"
export LD_LIBRARY_PATH="${prefix}/lib:${view}/lib:${LD_LIBRARY_PATH:-}"

workdir=$(mktemp -d)
trap 'rm -rf "${workdir}"' EXIT
cp "${write_jsonnet}" "${workdir}/write.jsonnet"
cp "${read_jsonnet}" "${workdir}/read.jsonnet"
cd "${workdir}"

echo "## write phase"
phlex -c write.jsonnet >write.out 2>&1 || { cat write.out; echo "FAIL: write phase errored"; exit 1; }
[[ -f integration_out.h5 ]] || { echo "FAIL: write produced no integration_out.h5"; exit 1; }

echo "## read phase"
phlex -c read.jsonnet >read.out 2>&1 || { cat read.out; echo "FAIL: read phase errored (observer assert or missing provider)"; exit 1; }
cat read.out

# A provider that never fired would surface as a missing-provider error; a
# reconstructed-frame problem would trip the observer's assert (non-zero exit
# caught above).
grep -qiE "no provider found|error|assert" read.out \
  && { echo "FAIL: read phase reported a problem"; exit 1; }

# The file-driven driver (ddm-c3s.16) must discover and drive exactly the cells
# present in the file (integration_frame_to_hdf writes 2 event cells), with no
# preconfigured count.  Assert it drove 2 -- guards against a silent zero-cell
# pass (the observer is silent when it never runs).
grep -qE "phlex_arrow_hdf_driver: driving 2 data cells" read.out \
  || { echo "FAIL: file-driven driver did not discover the expected 2 cells"; cat read.out; exit 1; }

echo "PASS: file-driven driver discovered 2 cells; HDF5 -> TableGroup -> wcphlex::Frame read-back through a Phlex graph"
