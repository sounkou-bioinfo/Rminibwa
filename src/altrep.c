#include <R.h>
#include <Rinternals.h>
#include <R_ext/Altrep.h>
#include <R_ext/Rdynload.h>
#include <string.h>
#include <stdint.h>

#include "rminibwa_internal.h"

static R_altrep_class_t rmb_alt_i32_class;
static R_altrep_class_t rmb_alt_i64_real_class;

static const char *alt_col_name(SEXP x) {
    SEXP name = R_altrep_data2(x);
    if (TYPEOF(name) != STRSXP || XLENGTH(name) != 1 || STRING_ELT(name, 0) == NA_STRING) {
        Rf_error("invalid Rminibwa ALTREP column metadata");
    }
    return CHAR(STRING_ELT(name, 0));
}

static RmbAlignBatch *alt_batch(SEXP x) {
    return rminibwa_align_mut_from_sexp(R_altrep_data1(x));
}

static R_xlen_t alt_length(SEXP x) {
    return (R_xlen_t) alt_batch(x)->n;
}

static int alt_i32_elt(SEXP x, R_xlen_t i) {
    RmbAlignBatch *batch = alt_batch(x);
    const int32_t *col = Rminibwa_align_i32_col(batch, alt_col_name(x));
    if (col == NULL) Rf_error("unknown integer alignment column: %s", alt_col_name(x));
    if (i < 0 || (size_t) i >= batch->n) return NA_INTEGER;
    return col[i];
}

static R_xlen_t alt_i32_region(SEXP x, R_xlen_t i, R_xlen_t n, int *buf) {
    RmbAlignBatch *batch = alt_batch(x);
    const int32_t *col = Rminibwa_align_i32_col(batch, alt_col_name(x));
    if (col == NULL) Rf_error("unknown integer alignment column: %s", alt_col_name(x));
    if (i < 0 || n <= 0 || (size_t) i >= batch->n) return 0;
    size_t avail = batch->n - (size_t) i;
    size_t take = (size_t) n < avail ? (size_t) n : avail;
    memcpy(buf, col + i, take * sizeof(int));
    return (R_xlen_t) take;
}

static int alt_i32_no_na(SEXP x) {
    (void) x;
    return 1;
}

static double alt_i64_real_elt(SEXP x, R_xlen_t i) {
    RmbAlignBatch *batch = alt_batch(x);
    const int64_t *col = Rminibwa_align_i64_col(batch, alt_col_name(x));
    if (col == NULL) Rf_error("unknown int64 alignment column: %s", alt_col_name(x));
    if (i < 0 || (size_t) i >= batch->n) return NA_REAL;
    return (double) col[i];
}

static R_xlen_t alt_i64_real_region(SEXP x, R_xlen_t i, R_xlen_t n, double *buf) {
    RmbAlignBatch *batch = alt_batch(x);
    const int64_t *col = Rminibwa_align_i64_col(batch, alt_col_name(x));
    if (col == NULL) Rf_error("unknown int64 alignment column: %s", alt_col_name(x));
    if (i < 0 || n <= 0 || (size_t) i >= batch->n) return 0;
    size_t avail = batch->n - (size_t) i;
    size_t take = (size_t) n < avail ? (size_t) n : avail;
    for (size_t j = 0; j < take; ++j) buf[j] = (double) col[(size_t) i + j];
    return (R_xlen_t) take;
}

static int alt_real_no_na(SEXP x) {
    (void) x;
    return 1;
}

SEXP rminibwa_align_altinteger(SEXP batch_xptr, const char *name) {
    if (Rminibwa_align_i32_col(rminibwa_align_mut_from_sexp(batch_xptr), name) == NULL) {
        Rf_error("unknown integer alignment column: %s", name);
    }
    SEXP name_x = PROTECT(Rf_mkString(name));
    SEXP out = R_new_altrep(rmb_alt_i32_class, batch_xptr, name_x);
    UNPROTECT(1);
    return out;
}

SEXP rminibwa_align_altreal(SEXP batch_xptr, const char *name) {
    if (Rminibwa_align_i64_col(rminibwa_align_mut_from_sexp(batch_xptr), name) == NULL) {
        Rf_error("unknown int64 alignment column: %s", name);
    }
    SEXP name_x = PROTECT(Rf_mkString(name));
    SEXP out = R_new_altrep(rmb_alt_i64_real_class, batch_xptr, name_x);
    UNPROTECT(1);
    return out;
}

void rminibwa_init_altrep(DllInfo *dll) {
    rmb_alt_i32_class = R_make_altinteger_class("rminibwa_align_i32", "Rminibwa", dll);
    R_set_altrep_Length_method(rmb_alt_i32_class, alt_length);
    R_set_altinteger_Elt_method(rmb_alt_i32_class, alt_i32_elt);
    R_set_altinteger_Get_region_method(rmb_alt_i32_class, alt_i32_region);
    R_set_altinteger_No_NA_method(rmb_alt_i32_class, alt_i32_no_na);

    rmb_alt_i64_real_class = R_make_altreal_class("rminibwa_align_i64_real", "Rminibwa", dll);
    R_set_altrep_Length_method(rmb_alt_i64_real_class, alt_length);
    R_set_altreal_Elt_method(rmb_alt_i64_real_class, alt_i64_real_elt);
    R_set_altreal_Get_region_method(rmb_alt_i64_real_class, alt_i64_real_region);
    R_set_altreal_No_NA_method(rmb_alt_i64_real_class, alt_real_no_na);
}
