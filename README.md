# Rminibwa

Rminibwa is a new R package/repo for exploring an R interface to
[minibwa](https://github.com/lh3/minibwa) and, later, a native runtime-SIMD
backend using the `RsimdDispatch` approach.

Current state: scaffold plus CLI wrappers. Native library bindings and dynamic
SIMD dispatch are intentionally still design work.

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

See [`docs/shape.md`](docs/shape.md). Short version:

- ship a CLI-compatible layer first;
- add native C bindings around `mb_idx_load()`, `mb_map()`, and
  `mb_map_batch()` next;
- isolate the SIMD-sensitive KSW alignment kernels for runtime dispatch rather
  than rewriting minibwa.
