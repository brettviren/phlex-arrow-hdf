#!/usr/bin/env bash
#
# phlex-arrow-hdf coverage-policy drop test (ddm-c3s.8).
#
# Runs the integration_drop.jsonnet workflow (output claims 'deposet' while the
# source emits frames) and asserts the intentional drop is NOT silent: the
# module logs "not persisting Arrow product 'frame'" and writes no file.
#
# Usage: integration_drop.sh <prefix> <view> <jsonnet>   (see integration_write.sh)

set -euo pipefail

prefix=$(realpath "${1:?prefix}")
view=$(realpath "${2:?view}")
jsonnet=$(realpath "${3:?jsonnet}")

export PATH="${view}/bin:${PATH}"
export PHLEX_PLUGIN_PATH="${prefix}/lib:${view}/lib"
export LD_LIBRARY_PATH="${prefix}/lib:${view}/lib:${LD_LIBRARY_PATH:-}"

workdir=$(mktemp -d)
trap 'rm -rf "${workdir}"' EXIT
cp "${jsonnet}" "${workdir}/wf.jsonnet"
cd "${workdir}"

echo "## running phlex (drop scenario)"
# Capture both streams: the drop notice goes to the module's diagnostic log.
phlex -c wf.jsonnet >phlex.out 2>&1 || { cat phlex.out; echo "FAIL: phlex errored"; exit 1; }
cat phlex.out

grep -q "not persisting Arrow product 'frame'" phlex.out \
  || { echo "FAIL: intentional drop of 'frame' was not logged (silent drop)"; exit 1; }

[[ ! -f dropped.h5 ]] || { echo "FAIL: dropped.h5 was written for an unclaimed product"; exit 1; }

echo "PASS: unclaimed product logged as an intentional drop and not written"
