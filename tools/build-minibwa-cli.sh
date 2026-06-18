#!/usr/bin/env bash
set -euo pipefail

# Local developer helper only. This does not install or vendor minibwa into the
# R package. By default it builds the pinned upstream commit declared in
# tools/minibwa-upstream.dcf.
#
# By default this builds a SIMDe-backed portable minibwa binary using the
# RsimdDispatch vendored SIMDe headers. Set MINIBWA_SIMDE=0 to build upstream
# unchanged, or MINIBWA_ARCH=x86_64 to allow upstream x86_64 ISA flags.

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

commit="$(MINIBWA_DEST=.sync/minibwa tools/fetch-minibwa.sh)"

make_args=(
  "gpl=${MINIBWA_GPL:-0}"
  "mimalloc=${MINIBWA_MIMALLOC:-0}"
  "omp=${MINIBWA_OMP:-0}"
)

if [ "${MINIBWA_SIMDE:-1}" != "0" ]; then
  tools/apply-minibwa-patches.sh .sync/minibwa

  simde_inc="$(Rscript -e 'p <- system.file("include", package = "RsimdDispatch"); if (!nzchar(p)) quit(status = 1); cat(p)')"
  make_args+=(
    "ARCH=${MINIBWA_ARCH:-generic}"
    "CPPFLAGS=${CPPFLAGS:-} -DRMINIBWA_USE_SIMDE -I${simde_inc}"
  )
fi

make -C .sync/minibwa "${make_args[@]}"
mkdir -p .local/bin
cp .sync/minibwa/minibwa .local/bin/minibwa
printf 'Built minibwa %s at %s\n' "$commit" "$root/.local/bin/minibwa"
if [ "${MINIBWA_SIMDE:-1}" != "0" ]; then
  printf 'Build mode: SIMDe portable patch applied (RsimdDispatch headers).\n'
else
  printf 'Build mode: upstream source without SIMDe patch.\n'
fi
printf 'Use with: export RMINIBWA_MINIBWA=%s\n' "$root/.local/bin/minibwa"
