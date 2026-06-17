#!/usr/bin/env bash
set -euo pipefail

# Local developer helper only. This does not install or vendor minibwa into the
# R package. Use MINIBWA_REF=<tag-or-commit> to pin an upstream revision.

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

commit="$(MINIBWA_DEST=.sync/minibwa tools/fetch-minibwa.sh)"
make -C .sync/minibwa gpl=0
mkdir -p .local/bin
cp .sync/minibwa/minibwa .local/bin/minibwa
printf 'Built minibwa %s at %s\n' "$commit" "$root/.local/bin/minibwa"
printf 'Use with: export RMINIBWA_MINIBWA=%s\n' "$root/.local/bin/minibwa"
