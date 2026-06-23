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
