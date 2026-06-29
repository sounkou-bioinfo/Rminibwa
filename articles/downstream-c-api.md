# Downstream C API

Rminibwa alignment batches are native external pointers with borrowed
column buffers. R users can inspect them through ALTREP vectors, but
downstream packages should consume them through the installed C header:

``` c
#include <Rminibwa.h>
```

The header resolves functions with `R_GetCCallable()`. A downstream
package therefore needs `LinkingTo: Rminibwa` for the header and an
ordinary R package runtime dependency to ensure Rminibwa is loaded
before the first call.

This vignette uses `Rtinycc` to compile the downstream consumer
in-process. The C source is displayed with Rtinycc’s C rendering helper
and compiled from the same `rminibwa_capi_code` object.

## Build a tiny alignment batch

``` r

library(Rminibwa)

td <- tempfile("rminibwa-capi-")
dir.create(td)
ref <- paste(rep("ACGT", 1000), collapse = "")
fa <- file.path(td, "ref.fa")
writeLines(c(">chr1", ref), fa, useBytes = TRUE)
prefix <- file.path(td, "idx")

mb_index_build(fa, prefix, threads = 1L)
idx <- mb_index_load(prefix)
aln <- charToRaw(substr(ref, 1L, 100L)) |>
  mb_map(idx, opt = mb_opts("sr", out_n = 0L), name = charToRaw("read1"))

mb_align_n(aln)
#> [1] 51
mb_align_col(aln, "tid")[[1]]
#> [1] 0
```

## The downstream C consumer

``` c
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

## Compile and call it with Rtinycc

``` r

ffi <- tryCatch(
  Rtinycc::tcc_ffi() |>
    Rtinycc::tcc_include(system.file("include", package = "Rminibwa")) |>
    Rtinycc::tcc_source(rminibwa_capi_code) |>
    Rtinycc::tcc_bind(
      rminibwa_capi_summary = list(args = list("sexp"), returns = "sexp")
    ) |>
    Rtinycc::tcc_compile(),
  error = identity
)

if (inherits(ffi, "error")) {
  cat("Rtinycc compilation is unavailable on this platform during vignette build:\n")
  cat(conditionMessage(ffi), "\n")
} else {
  ffi$rminibwa_capi_summary(aln)
}
#>           n   first_tid    first_qe cigar_words 
#>          51           0         100          51
```

The C function never asks R to materialize a data frame. It receives the
batch SEXP, obtains the opaque `RmbAlignBatch *`, and reads borrowed
`int32_t`, `int64_t`, and packed CIGAR buffers directly.
