
# Rminibwa

<!-- badges: start -->

[![R-CMD-check](https://github.com/sounkou-bioinfo/Rminibwa/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/sounkou-bioinfo/Rminibwa/actions/workflows/R-CMD-check.yaml)
[![R-universe](https://sounkou-bioinfo.r-universe.dev/badges/Rminibwa)](https://sounkou-bioinfo.r-universe.dev)
<!-- badges: end -->

Rminibwa integrates [minibwa](https://github.com/lh3/minibwa), Heng Li’s
genomic read aligner, with R through a packaged CLI, native C bindings,
and a pinned vendored source tree with patch provenance.

The native path follows the `RsimdDispatch`/`SIMDe` style: use upstream
minibwa algorithms, isolate SIMD-sensitive KSW kernels, stage backend
objects at configure time, and select portable `scalar`, `sse4`, or
`avx2` backends at runtime.

## Install

``` r
repos <- c(
  sounkou = "https://sounkou-bioinfo.r-universe.dev",
  CRAN = "https://cloud.r-project.org"
)
install.packages("RsimdDispatch", repos = repos)

# install.packages("remotes")
remotes::install_github("sounkou-bioinfo/Rminibwa", repos = repos)
```

Rminibwa builds and installs its own `minibwa` executable from the
vendored source. The native C-library backend uses GNU make and zlib.

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

## Native batch API

The native path keeps the hot alignment shape as a native columnar
batch. R gets raw-byte inputs and ALTREP column views; downstream
packages can consume the same batch through `inst/include/Rminibwa.h`
without copying it into a data frame.

``` r
library(Rminibwa)

td <- tempfile("rminibwa-readme-")
dir.create(td)
ref <- paste(rep("ACGT", 1000), collapse = "")
fa <- file.path(td, "ref.fa")
writeLines(c(">chr1", ref), fa, useBytes = TRUE)
prefix <- file.path(td, "idx")

mb_index_build(fa, prefix, threads = 1L)
idx <- mb_index_load(prefix)
aln <- charToRaw(substr(ref, 1L, 100L)) |>
  mb_map(idx, opt = mb_opts("sr", out_n = 0L), name = charToRaw("read1"))

c(
  n = mb_align_n(aln),
  first_tid = mb_align_col(aln, "tid")[[1]],
  first_qe = mb_align_col(aln, "qe")[[1]],
  cigar_bytes = length(mb_align_cigar_words(aln))
)
#>           n   first_tid    first_qe cigar_bytes 
#>          51           0         100         204
```

## Downstream C API with Rtinycc

`Rminibwa.h` is installed for downstream C consumers. This inline C
chunk uses `R_GetCCallable()` through the header wrappers, then reads
borrowed alignment columns directly. The `rtinycc` knitr engine displays
the C code and stores the chunk body in `rminibwa_capi_code` for the
next R chunk.

``` rtinycc
#define _Complex
#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <R.h>
#include <Rinternals.h>
#include <Rminibwa.h>

static int clamp_size_to_int(size_t x)
{
    return x > (size_t) INT_MAX ? INT_MAX : (int) x;
}

SEXP rminibwa_capi_summary(SEXP align_x)
{
    const RmbAlignBatch *batch = Rminibwa_align_from_sexp(align_x);
    const size_t n = Rminibwa_align_n(batch);
    const int32_t *tid = Rminibwa_align_i32_col(batch, "tid");
    const int32_t *qe = Rminibwa_align_i32_col(batch, "qe");
    const int64_t *ts = Rminibwa_align_i64_col(batch, "ts");
    size_t n_cigar_words = 0;
    (void) Rminibwa_align_cigar_words(batch, &n_cigar_words);

    SEXP out = PROTECT(Rf_allocVector(INTSXP, 4));
    INTEGER(out)[0] = clamp_size_to_int(n);
    INTEGER(out)[1] = n && tid ? tid[0] : NA_INTEGER;
    INTEGER(out)[2] = n && qe ? qe[0] : NA_INTEGER;
    INTEGER(out)[3] = clamp_size_to_int(n_cigar_words);

    SEXP names = PROTECT(Rf_allocVector(STRSXP, 4));
    SET_STRING_ELT(names, 0, Rf_mkChar("n"));
    SET_STRING_ELT(names, 1, Rf_mkChar("first_tid"));
    SET_STRING_ELT(names, 2, Rf_mkChar("first_qe"));
    SET_STRING_ELT(names, 3, Rf_mkChar("cigar_words"));
    Rf_setAttrib(out, R_NamesSymbol, names);

    /* Touch one 64-bit column so this example covers both integer-width paths. */
    if (n && ts && ts[0] < 0) INTEGER(out)[2] = NA_INTEGER;

    UNPROTECT(2);
    return out;
}
```

``` r
ffi <- Rtinycc::tcc_ffi() |>
  Rtinycc::tcc_include(system.file("include", package = "Rminibwa")) |>
  Rtinycc::tcc_source(rminibwa_capi_code) |>
  Rtinycc::tcc_bind(
    rminibwa_capi_summary = list(args = list("sexp"), returns = "sexp")
  ) |>
  Rtinycc::tcc_compile()

ffi$rminibwa_capi_summary(aln)
#>           n   first_tid    first_qe cigar_words 
#>          51           0         100          51
```

## Developer backend benchmarks

`make rdm MINIBWA_BINDINGS_ROOT=/path/to/minibwa-bindings` builds local
Python/Rust comparison artifacts and renders these opt-in developer
benchmarks. The workload uses a chrM-sized random reference and an
indel-mutated long read.

``` r
library(bench)
library(reticulate)

bench_td <- tempfile("rminibwa-bench-")
dir.create(bench_td)
set.seed(1)
bench_ref <- paste(sample(c("A", "C", "G", "T"), 16569, replace = TRUE), collapse = "")
delete_every <- function(x, every = 25L) {
  y <- strsplit(x, "", fixed = TRUE)[[1L]]
  paste(y[-seq(every, length(y), by = every)], collapse = "")
}
bench_fa <- file.path(bench_td, "ref.fa")
writeLines(c(">chr1", bench_ref), bench_fa, useBytes = TRUE)
bench_prefix <- file.path(bench_td, "idx")
mb_index_build(bench_fa, bench_prefix, threads = 1L)
bench_idx <- mb_index_load(bench_prefix)
bench_opt <- mb_opts("sr", out_n = 0L)
bench_query <- delete_every(substr(bench_ref, 1000L, 12000L), every = 25L)
bench_query_raw <- charToRaw(bench_query)
bench_name_raw <- charToRaw("read1")

internal_backends <- intersect(c("scalar", "sse4", "avx2"), simd_info()$available_backends)
ksw_counts <- do.call(rbind, lapply(internal_backends, function(backend) {
  simd_set_backend(backend)
  invisible(Rminibwa:::simd_counters(TRUE))
  invisible(Rminibwa:::mb_map_count(bench_query_raw, bench_idx, bench_opt, bench_name_raw))
  data.frame(backend = backend, as.list(Rminibwa:::simd_counters(FALSE)), check.names = FALSE)
}))
ksw_counts

rminibwa_internal <- do.call(rbind, lapply(internal_backends, function(backend) {
  simd_set_backend(backend)
  out <- bench::mark(
    count_only = Rminibwa:::mb_map_count(bench_query_raw, bench_idx, bench_opt, bench_name_raw),
    iterations = 100,
    check = FALSE,
    time_unit = "us"
  )
  out$backend <- backend
  out
}))
rminibwa_internal[, c("backend", "min", "median", "itr/sec", "mem_alloc")]

py_path <- Sys.getenv("RMINIBWA_BENCH_PYTHONPATH")
py_run_string(sprintf("import sys; sys.path.insert(0, %s)", shQuote(py_path)))
py_minibwa <- import("minibwa", convert = FALSE)
py_idx <- py_minibwa$Index$load(bench_prefix)
py_opt <- py_minibwa$Opts("sr")
invisible(py_opt$set_out_n(0L))

rust_lib <- Sys.getenv("RMINIBWA_BENCH_RUST_LIB")
dyn.load(rust_lib)
invisible(.C("rminibwa_bench_rust_init_c", bench_prefix))

simd_set_backend("avx2")
external_compare <- bench::mark(
  rminibwa_avx2_count = Rminibwa:::mb_map_count(bench_query_raw, bench_idx, bench_opt, bench_name_raw),
  rminibwa_avx2_batch = mb_align_n(mb_map(bench_query_raw, bench_idx, bench_opt, bench_name_raw)),
  python_pyo3 = length(py_minibwa$map(py_idx, py_opt, "read1", bench_query, "none")),
  rust_cdylib = .C("rminibwa_bench_rust_map_count_c", bench_query, out = integer(1L))$out,
  iterations = 100,
  check = FALSE,
  time_unit = "us"
)
external_compare[, c("expression", "min", "median", "itr/sec", "mem_alloc")]

invisible(.C("rminibwa_bench_rust_clear_c"))
dyn.unload(rust_lib)
simd_set_backend("auto")
unlink(bench_td, recursive = TRUE)
```

## Pinned upstream workflow

The upstream pin lives in `tools/minibwa-upstream.dcf` and is installed
as package metadata:

``` r
Rminibwa::minibwa_upstream_info()[c("Version", "Commit")]
#> $Version
#> [1] "0.2-r379-dirty"
#> 
#> $Commit
#> [1] "f0fd108456d459e5da3755f29c8700dc7f6fe0df"
```

The patch queue lives in `tools/patches/minibwa/`.

Build a local developer CLI from the pinned commit with the SIMDe patch
queue applied:

``` sh
tools/build-minibwa-cli.sh
```

Rminibwa itself installs a package-built `minibwa` executable during
package installation.

Refresh the vendored source tree with:

``` sh
Rscript tools/vendor-minibwa.R refresh
```

The vendoring script records the upstream commit, archive checksum, and
applied patches in `src/vendor/minibwa/RMINIBWA_VENDOR.dcf`.

## Shape

The native implementation is deliberately narrow:

1.  keep the CLI wrapper as the compatibility oracle;
2.  load `.l2b`/`.mbw` indexes into R external pointers;
3.  expose `mb_map()` through `.Call()` with raw-byte inputs and native
    columnar batches;
4.  dispatch the KSW alignment kernels through a staged
    `RsimdDispatch`/`SIMDe`-style runtime-selection pattern.

See the package vignettes for details.

## Development

``` sh
make rd       # roxygen docs and NAMESPACE
make readme   # evaluate README.Rmd and write README.md
make test
make check
make pkgdown
make asm      # inspect staged backend assembly
```
