#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
checkout="${1:-.sync/minibwa}"

case "$checkout" in
  /*) ;;
  *) checkout="$root/$checkout" ;;
esac

if [ ! -d "$checkout" ]; then
  echo "minibwa checkout/source directory not found: $checkout" >&2
  exit 1
fi

patch_dir="$root/tools/patches/minibwa"
if [ ! -d "$patch_dir" ]; then
  echo "patch directory not found: $patch_dir" >&2
  exit 1
fi

shopt -s nullglob
patches=("$patch_dir"/*.patch)
if [ "${#patches[@]}" -eq 0 ]; then
  echo "No minibwa patches to apply."
  exit 0
fi

for patch in "${patches[@]}"; do
  name="$(basename "$patch")"
  if git -C "$checkout" apply --check "$patch" >/dev/null 2>&1; then
    echo "Applying $name"
    git -C "$checkout" apply --whitespace=nowarn "$patch"
  elif git -C "$checkout" apply --reverse --check "$patch" >/dev/null 2>&1; then
    echo "Already applied $name"
  else
    echo "Patch does not apply cleanly: $name" >&2
    git -C "$checkout" apply --check "$patch"
    exit 1
  fi
done
