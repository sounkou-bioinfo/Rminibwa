# Rminibwa

Rminibwa is a new R package/repo for exploring an R interface to
[minibwa](https://github.com/lh3/minibwa) and, later, a native runtime-SIMD
backend using the `RsimdDispatch` approach.

Current state: scaffold plus CLI wrappers. Native library bindings and dynamic
SIMD dispatch are intentionally still design work.

Design decision: even though SSE4.2 is common on modern x86_64, the native path
will still follow the `RsimdDispatch` package and use `SIMDe`-backed runtime
selection. That gives us a portable fallback, explicit backend reporting, and a
cleaner route to ARM/NEON and wasm experiments.

## CLI usage

```r
library(Rminibwa)

minibwa_available()
minibwa_version()

prefix <- minibwa_index("ref.fa", threads = 8)
aln <- minibwa_map(prefix, "reads.fq.gz", format = "paf", threads = 8)
```

Set a custom binary with:

```r
Sys.setenv(RMINIBWA_MINIBWA = "/path/to/minibwa")
```

For local development you can build an upstream CLI copy with:

```sh
tools/build-minibwa-cli.sh
export RMINIBWA_MINIBWA=$PWD/.local/bin/minibwa
```

## Shape discussion

See [`docs/shape.md`](docs/shape.md) and [`docs/simde-dispatch.md`](docs/simde-dispatch.md). Short version:

- ship a CLI-compatible layer first;
- add native C bindings around `mb_idx_load()`, `mb_map()`, and
  `mb_map_batch()` next;
- isolate the SIMD-sensitive KSW alignment kernels for `SIMDe` + runtime
  dispatch rather than rewriting minibwa.
