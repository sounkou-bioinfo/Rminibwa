# Vendoring minibwa

The upstream `lh3/minibwa` repository is expected to move quickly.
Rminibwa therefore pins a specific upstream commit and keeps local
changes in a patch queue.

## Pin file

The upstream lock file is `tools/minibwa-upstream.dcf`:

``` sh
Rscript tools/vendor-minibwa.R status
```

It records the component name, upstream version, repository, commit,
archive URL, patch directory, and license note.

## Patch queue

Patches live in `tools/patches/minibwa/` and are applied in lexical
order. Keep patches small and tied to the exact pinned commit.

The current patch adds an opt-in `RMINIBWA_USE_SIMDE` branch to the KSW
include blocks. It does not change upstream behavior unless that macro
is defined.

## Developer checkout

Fetch the pinned upstream source into `.sync/minibwa`:

``` sh
tools/fetch-minibwa.sh
```

Build a local developer CLI with the SIMDe patch queue applied:

``` sh
tools/build-minibwa-cli.sh
export RMINIBWA_MINIBWA=$PWD/.local/bin/minibwa
```

By default the developer build uses:

- `gpl=0`, avoiding optional GPL components;
- `mimalloc=0`, using ordinary allocation for simpler local debugging;
- `omp=0`, avoiding OpenMP requirements during package work;
- `RsimdDispatch`’s installed SIMDe headers.

## Vendored source

When native bindings begin, stage the pinned source under
`src/vendor/minibwa`:

``` sh
Rscript tools/vendor-minibwa.R refresh
```

The script downloads the pinned GitHub archive, unpacks it, applies the
patch queue, verifies `MB_VERSION`, and writes
`src/vendor/minibwa/RMINIBWA_VENDOR.dcf` with archive checksum and patch
metadata.

Use `clean` to remove the staged source:

``` sh
Rscript tools/vendor-minibwa.R clean
```

Do not silently edit vendored upstream C. Update the patch queue and
rerun the vendor script instead.
