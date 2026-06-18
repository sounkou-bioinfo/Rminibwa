
# Rminibwa

<!-- badges: start -->

[![R-CMD-check](https://github.com/sounkou-bioinfo/Rminibwa/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/sounkou-bioinfo/Rminibwa/actions/workflows/R-CMD-check.yaml)
[![R-universe](https://sounkou-bioinfo.r-universe.dev/badges/Rminibwa)](https://sounkou-bioinfo.r-universe.dev)
<!-- badges: end -->

Rminibwa is an R-facing package for integrating
[minibwa](https://github.com/lh3/minibwa), Heng Li’s short-read aligner,
with R. The package starts with command-line wrappers and keeps a pinned
upstream source and patch queue for native-library work.

The native path will follow the `RsimdDispatch`/`SIMDe` style: use
upstream minibwa algorithms, isolate SIMD-sensitive KSW kernels, and
select portable, SSE4.2, NEON, or future wasm backends at runtime.

## Install

``` r
# install.packages("remotes")
remotes::install_github("sounkou-bioinfo/Rminibwa")
```

The current public API needs an installed `minibwa` executable at
runtime. The planned native C-library backend will use GNU make and
zlib. Set a custom CLI binary with:

``` r
Sys.setenv(RMINIBWA_MINIBWA = "/path/to/minibwa")
```

## CLI API

``` r
library(Rminibwa)

minibwa_available()
minibwa_version()

prefix <- minibwa_index("ref.fa", threads = 8)
aln <- minibwa_map(prefix, "reads.fq.gz", format = "paf", threads = 8)
```

`minibwa_map()` returns captured output when `output = NULL` and writes
to a file when `output` is supplied.

## Pinned upstream workflow

The upstream pin lives in `tools/minibwa-upstream.dcf` and is installed
as package metadata:

``` r
Rminibwa::minibwa_upstream_info()[c("Version", "Commit")]
#> $Version
#> [1] "0.1-r363"
#> 
#> $Commit
#> [1] "0a3421434f9dbab4def012aef37946e8a9fc08f8"
```

The patch queue lives in `tools/patches/minibwa/`.

Build a local developer CLI from the pinned commit with the SIMDe patch
queue applied:

``` sh
tools/build-minibwa-cli.sh
export RMINIBWA_MINIBWA=$PWD/.local/bin/minibwa
```

Stage a vendored source tree when native bindings begin:

``` sh
Rscript tools/vendor-minibwa.R refresh
```

The vendoring script records the upstream commit, archive checksum, and
applied patches in `src/vendor/minibwa/RMINIBWA_VENDOR.dcf`.

## Shape

The planned native implementation is deliberately narrow:

1.  keep the CLI wrapper as the compatibility oracle;
2.  load `.l2b`/`.mbw` indexes into R external pointers;
3.  expose `mb_map()` and `mb_map_batch()` through `.Call()`;
4.  dispatch the KSW alignment kernels through the
    `RsimdDispatch`/`SIMDe` runtime-selection pattern.

See the package vignettes for details.

## Development

``` sh
make rd       # roxygen docs and NAMESPACE
make readme   # evaluate README.Rmd and write README.md
make test
make check
make pkgdown
```
