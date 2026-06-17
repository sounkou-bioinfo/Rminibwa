#!/usr/bin/env bash
set -euo pipefail

repo="${MINIBWA_REPO:-https://github.com/lh3/minibwa.git}"
ref="${MINIBWA_REF:-main}"
dest="${MINIBWA_DEST:-.sync/minibwa}"

if [ -d "$dest/.git" ]; then
  git -C "$dest" fetch --depth 1 origin "$ref"
  git -C "$dest" checkout FETCH_HEAD
else
  rm -rf "$dest"
  git clone --depth 1 --branch "$ref" "$repo" "$dest"
fi

git -C "$dest" rev-parse HEAD
