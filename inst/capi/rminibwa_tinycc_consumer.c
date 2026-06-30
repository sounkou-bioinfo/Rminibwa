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
    const size_t n_read = Rminibwa_align_n_read(batch);
    const int32_t *read_len = Rminibwa_align_read_i32_col(batch, "length");
    const int32_t *tid = Rminibwa_align_i32_col(batch, "tid");
    const int32_t *qe = Rminibwa_align_i32_col(batch, "qe");
    const int64_t *ts = Rminibwa_align_i64_col(batch, "ts");
    size_t n_cigar_words = 0;
    (void) Rminibwa_align_cigar_words(batch, &n_cigar_words);

    SEXP out = PROTECT(Rf_allocVector(INTSXP, 5));
    INTEGER(out)[0] = clamp_size_to_int(n);
    INTEGER(out)[1] = clamp_size_to_int(n_read);
    INTEGER(out)[2] = n_read && read_len ? read_len[0] : NA_INTEGER;
    INTEGER(out)[3] = n && tid ? tid[0] : NA_INTEGER;
    INTEGER(out)[4] = n && qe ? qe[0] : NA_INTEGER;

    SEXP names = PROTECT(Rf_allocVector(STRSXP, 5));
    SET_STRING_ELT(names, 0, Rf_mkChar("n"));
    SET_STRING_ELT(names, 1, Rf_mkChar("n_read"));
    SET_STRING_ELT(names, 2, Rf_mkChar("first_read_length"));
    SET_STRING_ELT(names, 3, Rf_mkChar("first_tid"));
    SET_STRING_ELT(names, 4, Rf_mkChar("first_qe"));
    Rf_setAttrib(out, R_NamesSymbol, names);

    /* Touch one 64-bit column so this example covers both integer-width paths. */
    if (n_cigar_words == 0) INTEGER(out)[4] = NA_INTEGER;
    if (n && ts && ts[0] < 0) INTEGER(out)[4] = NA_INTEGER;

    UNPROTECT(2);
    return out;
}
