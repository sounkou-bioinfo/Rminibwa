local({
  tmp <- tempfile("rminibwa-capi-")
  dir.create(tmp)
  on.exit(unlink(tmp, recursive = TRUE), add = TRUE)

  ref <- paste(rep("ACGT", 1000), collapse = "")
  fa <- file.path(tmp, "ref.fa")
  writeLines(c(">chr1", ref), fa, useBytes = TRUE)
  prefix <- file.path(tmp, "idx")
  mb_index_build(fa, prefix, threads = 1L)
  idx <- mb_index_load(prefix)
  aln <- charToRaw(substr(ref, 1L, 100L)) |>
    mb_map(idx, opt = mb_opts("sr", out_n = 0L), name = charToRaw("read1"))

  capi_code <- '
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
    if (n && ts && ts[0] < 0) INTEGER(out)[2] = NA_INTEGER;
    UNPROTECT(1);
    return out;
}
'

  ffi <- Rtinycc::tcc_ffi() |>
    Rtinycc::tcc_include(system.file("include", package = "Rminibwa")) |>
    Rtinycc::tcc_source(capi_code) |>
    Rtinycc::tcc_bind(
      rminibwa_capi_summary = list(args = list("sexp"), returns = "sexp")
    ) |>
    Rtinycc::tcc_compile()

  summary <- ffi$rminibwa_capi_summary(aln)
  expect_equal(summary[[1]], as.integer(mb_align_n(aln)))
  expect_equal(summary[[2]], 0L)
  expect_true(summary[[3]] > 0L)
  expect_true(summary[[4]] > 0L)
})
