#!/usr/bin/env bash
#
# phlex-arrow-hdf write-path integration test.
#
# Runs the integration_frame_to_hdf.jsonnet workflow through the `phlex` binary
# and asserts the produced HDF5 file has the expected narrow-waist layout:
# the data-cell hierarchy (event/1, event/2) as groups, a per-product `frame`
# group carrying the arrow.group.type = "wc.frame" marker, and the four
# reserved wc.frame member groups (traces/frame_tags/trace_tags/cmm), each with
# an Arrow schema blob.
#
# Usage: integration_write.sh <prefix> <view> <jsonnet>
#   <prefix>  install prefix holding the three plugins + their shared libs
#   <view>    Spack view prefix providing phlex, h5ls, h5dump, HDF5/Arrow libs
#   <jsonnet> the workflow file to run
#
# This is a cross-package test: it requires wire-cell-phlex,
# wire-cell-phlex-arrow, and phlex-arrow-hdf all installed under <prefix>.

set -euo pipefail

prefix=${1:?prefix}
view=${2:?view}
jsonnet=${3:?jsonnet}

export PATH="${view}/bin:${PATH}"
export PHLEX_PLUGIN_PATH="${prefix}/lib:${view}/lib"
export LD_LIBRARY_PATH="${prefix}/lib:${view}/lib:${LD_LIBRARY_PATH:-}"

workdir=$(mktemp -d)
trap 'rm -rf "${workdir}"' EXIT
cp "${jsonnet}" "${workdir}/wf.jsonnet"
cd "${workdir}"

echo "## running phlex"
phlex -c wf.jsonnet

out=integration_out.h5
[[ -f "${out}" ]] || { echo "FAIL: ${out} not produced"; exit 1; }

echo "## h5ls -r"
listing=$(h5ls -r "${out}")
echo "${listing}"

fail() { echo "FAIL: $1"; exit 1; }
have() { grep -q "$1" <<<"${listing}" || fail "missing $1"; }

# Data-cell hierarchy: both event cells present as groups.
have "^/event/1 .*Group"
have "^/event/2 .*Group"

# For each event cell, the four reserved wc.frame member groups and a schema
# blob in each (the creator group name is escaped, so match loosely on suffix).
for ev in 1 2; do
  for member in traces frame_tags trace_tags cmm; do
    have "/event/${ev}/.*/frame/${member} .*Group"
    have "/event/${ev}/.*/frame/${member}/__arrow_schema__ .*Dataset"
  done
done

echo "## type marker (arrow.group.type)"
markers=$(h5dump -N "arrow.group.type" "${out}")
echo "${markers}" | grep -q '"wc.frame"' || fail 'arrow.group.type != "wc.frame"'
# One marker per event cell.
n=$(grep -c '"wc.frame"' <<<"${markers}")
[[ "${n}" -ge 2 ]] || fail "expected >=2 wc.frame markers, got ${n}"

echo "PASS: frame -> wc.frame TableGroup -> HDF5 (hierarchy + members + type marker)"
