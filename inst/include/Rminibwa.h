#ifndef RMINIBWA_H
#define RMINIBWA_H

#include <stddef.h>
#include <stdint.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RmbAlignBatch RmbAlignBatch;
typedef struct RmbIndex RmbIndex;

/* Borrowed pointers. They remain valid while the owning R external pointer is
 * protected and alive. Downstream packages should not free or mutate them. */
typedef const RmbAlignBatch *(*Rminibwa_align_from_sexp_fn)(SEXP x);
typedef size_t (*Rminibwa_align_n_fn)(const RmbAlignBatch *x);
typedef size_t (*Rminibwa_align_n_read_fn)(const RmbAlignBatch *x);
typedef const int32_t *(*Rminibwa_align_read_i32_col_fn)(const RmbAlignBatch *x, const char *name);
typedef const int32_t *(*Rminibwa_align_i32_col_fn)(const RmbAlignBatch *x, const char *name);
typedef const int64_t *(*Rminibwa_align_i64_col_fn)(const RmbAlignBatch *x, const char *name);
typedef const uint32_t *(*Rminibwa_align_cigar_words_fn)(const RmbAlignBatch *x, size_t *n_words);
typedef const int32_t *(*Rminibwa_align_cigar_i32_col_fn)(const RmbAlignBatch *x, const char *name);

#ifdef RMINIBWA_BUILDING

const RmbAlignBatch *Rminibwa_align_from_sexp(SEXP x);
size_t Rminibwa_align_n(const RmbAlignBatch *x);
size_t Rminibwa_align_n_read(const RmbAlignBatch *x);
const int32_t *Rminibwa_align_read_i32_col(const RmbAlignBatch *x, const char *name);
const int32_t *Rminibwa_align_i32_col(const RmbAlignBatch *x, const char *name);
const int64_t *Rminibwa_align_i64_col(const RmbAlignBatch *x, const char *name);
const uint32_t *Rminibwa_align_cigar_words(const RmbAlignBatch *x, size_t *n_words);
const int32_t *Rminibwa_align_cigar_i32_col(const RmbAlignBatch *x, const char *name);

#else

static inline Rminibwa_align_from_sexp_fn Rminibwa_get_align_from_sexp(void)
{
    return (Rminibwa_align_from_sexp_fn) R_GetCCallable("Rminibwa", "Rminibwa_align_from_sexp");
}

static inline Rminibwa_align_n_fn Rminibwa_get_align_n(void)
{
    return (Rminibwa_align_n_fn) R_GetCCallable("Rminibwa", "Rminibwa_align_n");
}

static inline Rminibwa_align_n_read_fn Rminibwa_get_align_n_read(void)
{
    return (Rminibwa_align_n_read_fn) R_GetCCallable("Rminibwa", "Rminibwa_align_n_read");
}

static inline Rminibwa_align_read_i32_col_fn Rminibwa_get_align_read_i32_col(void)
{
    return (Rminibwa_align_read_i32_col_fn) R_GetCCallable("Rminibwa", "Rminibwa_align_read_i32_col");
}

static inline Rminibwa_align_i32_col_fn Rminibwa_get_align_i32_col(void)
{
    return (Rminibwa_align_i32_col_fn) R_GetCCallable("Rminibwa", "Rminibwa_align_i32_col");
}

static inline Rminibwa_align_i64_col_fn Rminibwa_get_align_i64_col(void)
{
    return (Rminibwa_align_i64_col_fn) R_GetCCallable("Rminibwa", "Rminibwa_align_i64_col");
}

static inline Rminibwa_align_cigar_words_fn Rminibwa_get_align_cigar_words(void)
{
    return (Rminibwa_align_cigar_words_fn) R_GetCCallable("Rminibwa", "Rminibwa_align_cigar_words");
}

static inline Rminibwa_align_cigar_i32_col_fn Rminibwa_get_align_cigar_i32_col(void)
{
    return (Rminibwa_align_cigar_i32_col_fn) R_GetCCallable("Rminibwa", "Rminibwa_align_cigar_i32_col");
}

static inline const RmbAlignBatch *Rminibwa_align_from_sexp(SEXP x)
{
    return Rminibwa_get_align_from_sexp()(x);
}

static inline size_t Rminibwa_align_n(const RmbAlignBatch *x)
{
    return Rminibwa_get_align_n()(x);
}

static inline size_t Rminibwa_align_n_read(const RmbAlignBatch *x)
{
    return Rminibwa_get_align_n_read()(x);
}

static inline const int32_t *Rminibwa_align_read_i32_col(const RmbAlignBatch *x, const char *name)
{
    return Rminibwa_get_align_read_i32_col()(x, name);
}

static inline const int32_t *Rminibwa_align_i32_col(const RmbAlignBatch *x, const char *name)
{
    return Rminibwa_get_align_i32_col()(x, name);
}

static inline const int64_t *Rminibwa_align_i64_col(const RmbAlignBatch *x, const char *name)
{
    return Rminibwa_get_align_i64_col()(x, name);
}

static inline const uint32_t *Rminibwa_align_cigar_words(const RmbAlignBatch *x, size_t *n_words)
{
    return Rminibwa_get_align_cigar_words()(x, n_words);
}

static inline const int32_t *Rminibwa_align_cigar_i32_col(const RmbAlignBatch *x, const char *name)
{
    return Rminibwa_get_align_cigar_i32_col()(x, name);
}

#endif

#ifdef __cplusplus
}
#endif

#endif
