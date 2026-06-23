#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "rminibwa_internal.h"
#include "vendor/minibwa/mbpriv.h"

extern int main_index(int argc, char **argv);

#define RMB_FLAG_PROPER        0x01
#define RMB_FLAG_PRIMARY       0x02
#define RMB_FLAG_SECONDARY     0x04
#define RMB_FLAG_SUPPLEMENTARY 0x08

static int is_true_scalar(SEXP x) {
    return TYPEOF(x) == LGLSXP && XLENGTH(x) == 1 && LOGICAL(x)[0] == TRUE;
}

static int is_false_scalar(SEXP x) {
    return TYPEOF(x) == LGLSXP && XLENGTH(x) == 1 && LOGICAL(x)[0] == FALSE;
}

static const char *chr_scalar(SEXP x, const char *what) {
    if (TYPEOF(x) != STRSXP || XLENGTH(x) != 1 || STRING_ELT(x, 0) == NA_STRING) {
        Rf_error("%s must be a non-missing character scalar", what);
    }
    return CHAR(STRING_ELT(x, 0));
}

static int int_scalar(SEXP x, const char *what, int default_value) {
    if (x == R_NilValue) return default_value;
    if (!Rf_isInteger(x) && !Rf_isReal(x)) Rf_error("%s must be a numeric scalar", what);
    if (XLENGTH(x) != 1) Rf_error("%s must be a numeric scalar", what);
    double val = Rf_asReal(x);
    if (!R_FINITE(val) || val < INT_MIN || val > INT_MAX) Rf_error("%s is out of range", what);
    return (int) val;
}

RmbIndex *rminibwa_index_from_sexp(SEXP x) {
    if (TYPEOF(x) != EXTPTRSXP) Rf_error("expected a minibwa index external pointer");
    RmbIndex *idx = (RmbIndex *) R_ExternalPtrAddr(x);
    if (idx == NULL || idx->ptr == NULL) Rf_error("minibwa index pointer is not valid");
    return idx;
}

RmbAlignBatch *rminibwa_align_mut_from_sexp(SEXP x) {
    if (TYPEOF(x) != EXTPTRSXP) Rf_error("expected a minibwa alignment batch external pointer");
    RmbAlignBatch *batch = (RmbAlignBatch *) R_ExternalPtrAddr(x);
    if (batch == NULL) Rf_error("minibwa alignment batch pointer is not valid");
    return batch;
}

static void idx_finalizer(SEXP xptr) {
    RmbIndex *idx = (RmbIndex *) R_ExternalPtrAddr(xptr);
    if (idx != NULL) {
        if (idx->ptr != NULL) mb_idx_destroy(idx->ptr);
        R_Free(idx);
        R_ClearExternalPtr(xptr);
    }
}

static void align_batch_free(RmbAlignBatch *x) {
    if (x == NULL) return;
    if (x->index_ref != R_NilValue) R_ReleaseObject(x->index_ref);
    R_Free(x->read_id);
    R_Free(x->target_id);
    R_Free(x->query_length);
    R_Free(x->query_start);
    R_Free(x->query_end);
    R_Free(x->target_length);
    R_Free(x->target_start);
    R_Free(x->target_end);
    R_Free(x->strand);
    R_Free(x->mapq);
    R_Free(x->score);
    R_Free(x->matches);
    R_Free(x->block_length);
    R_Free(x->flags);
    R_Free(x->n_sub);
    R_Free(x->cigar_offset);
    R_Free(x->cigar_n);
    R_Free(x->cigar_words);
    R_Free(x);
}

static void align_finalizer(SEXP xptr) {
    RmbAlignBatch *batch = (RmbAlignBatch *) R_ExternalPtrAddr(xptr);
    if (batch != NULL) {
        align_batch_free(batch);
        R_ClearExternalPtr(xptr);
    }
}

static SEXP set_class(SEXP x, const char *cls) {
    SEXP klass = PROTECT(Rf_mkString(cls));
    Rf_setAttrib(x, R_ClassSymbol, klass);
    UNPROTECT(1);
    return x;
}

SEXP rminibwa_align_xptr_new(RmbAlignBatch *batch) {
    SEXP xptr = PROTECT(R_MakeExternalPtr(batch, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(xptr, align_finalizer, TRUE);
    set_class(xptr, "rminibwa_align");
    UNPROTECT(1);
    return xptr;
}

static RmbAlignBatch *align_batch_alloc(size_t n, size_t n_cigar_words, SEXP index_ref) {
    RmbAlignBatch *x = R_Calloc(1, RmbAlignBatch);
    x->n = n;
    x->index_ref = index_ref;
    R_PreserveObject(index_ref);
#define ALLOC_I32(field) x->field = n ? R_Calloc(n, int32_t) : NULL
#define ALLOC_I64(field) x->field = n ? R_Calloc(n, int64_t) : NULL
    ALLOC_I32(read_id);
    ALLOC_I32(target_id);
    ALLOC_I32(query_length);
    ALLOC_I32(query_start);
    ALLOC_I32(query_end);
    ALLOC_I64(target_length);
    ALLOC_I64(target_start);
    ALLOC_I64(target_end);
    ALLOC_I32(strand);
    ALLOC_I32(mapq);
    ALLOC_I32(score);
    ALLOC_I32(matches);
    ALLOC_I32(block_length);
    ALLOC_I32(flags);
    ALLOC_I32(n_sub);
    ALLOC_I32(cigar_offset);
    ALLOC_I32(cigar_n);
#undef ALLOC_I32
#undef ALLOC_I64
    x->n_cigar_words = n_cigar_words;
    x->cigar_words = n_cigar_words ? R_Calloc(n_cigar_words, uint32_t) : NULL;
    return x;
}

static void free_hits(mb_hit_t *hit, int32_t n_hit) {
    if (hit == NULL) return;
    for (int32_t i = 0; i < n_hit; ++i) {
        if (hit[i].p != NULL) free(hit[i].p);
    }
    free(hit);
}

static int hit_flags(const mb_hit_t *h) {
    int flags = 0;
    if (h->proper_pair) flags |= RMB_FLAG_PROPER;
    if (h->parent != h->id) {
        flags |= RMB_FLAG_SECONDARY;
    } else if (!h->sam_pri) {
        flags |= RMB_FLAG_SUPPLEMENTARY;
    } else {
        flags |= RMB_FLAG_PRIMARY;
    }
    return flags;
}

static RmbAlignBatch *copy_hits_to_batch(mb_hit_t *hit, int32_t n_hit, int32_t qlen, SEXP index_ref) {
    RmbIndex *idx = rminibwa_index_from_sexp(index_ref);
    size_t n_cigar = 0;
    for (int32_t i = 0; i < n_hit; ++i) {
        if (hit[i].p != NULL && hit[i].p->n_cigar > 0) n_cigar += (size_t) hit[i].p->n_cigar;
    }
    RmbAlignBatch *out = align_batch_alloc((size_t) n_hit, n_cigar, index_ref);
    size_t off = 0;
    for (int32_t i = 0; i < n_hit; ++i) {
        mb_hit_t *h = &hit[i];
        out->read_id[i] = 0;
        out->target_id[i] = (int32_t) h->tid;
        out->query_length[i] = qlen;
        out->query_start[i] = h->qs;
        out->query_end[i] = h->qe;
        out->target_length[i] = mb_idx_ctg_len(idx->ptr, (int32_t) h->tid);
        out->target_start[i] = h->ts;
        out->target_end[i] = h->te;
        out->strand[i] = h->rev ? 1 : 0;
        out->mapq[i] = h->mapq;
        out->score[i] = h->score;
        out->matches[i] = h->mlen;
        out->block_length[i] = h->blen;
        out->flags[i] = hit_flags(h);
        out->n_sub[i] = h->n_sub;
        out->cigar_offset[i] = (int32_t) off;
        out->cigar_n[i] = 0;
        if (h->p != NULL && h->p->n_cigar > 0) {
            int32_t nc = h->p->n_cigar;
            out->cigar_n[i] = nc;
            for (int32_t j = 0; j < nc; ++j) out->cigar_words[off + (size_t) j] = h->p->cigar[j];
            off += (size_t) nc;
        }
    }
    return out;
}

static int opt_list_index(SEXP names, const char *key) {
    if (names == R_NilValue) return -1;
    for (R_xlen_t i = 0; i < XLENGTH(names); ++i) {
        SEXP nm = STRING_ELT(names, i);
        if (nm != NA_STRING && strcmp(CHAR(nm), key) == 0) return (int) i;
    }
    return -1;
}

static SEXP opt_value(SEXP opts, const char *key) {
    if (opts == R_NilValue) return R_NilValue;
    if (TYPEOF(opts) != VECSXP) Rf_error("opt must be a list returned by mb_opts()");
    int idx = opt_list_index(Rf_getAttrib(opts, R_NamesSymbol), key);
    return idx >= 0 ? VECTOR_ELT(opts, idx) : R_NilValue;
}

static void apply_options(SEXP opts_x, mb_opt_t *opt) {
    mb_opt_init(opt);
    SEXP preset_x = opt_value(opts_x, "preset");
    if (preset_x != R_NilValue) {
        const char *preset = chr_scalar(preset_x, "opt$preset");
        if (mb_opt_preset(opt, preset) != 0) Rf_error("unknown minibwa preset: %s", preset);
    }
    SEXP paired_x = opt_value(opts_x, "paired");
    if (is_true_scalar(paired_x)) opt->flag |= MB_F_PE;
    else if (is_false_scalar(paired_x)) opt->flag &= ~MB_F_PE;
    SEXP methylation_x = opt_value(opts_x, "methylation");
    if (is_true_scalar(methylation_x)) opt->flag |= MB_F_METH;
    else if (is_false_scalar(methylation_x)) opt->flag &= ~MB_F_METH;

#define SET_INT_FIELD(key, field) do { \
    SEXP v__ = opt_value(opts_x, key); \
    if (v__ != R_NilValue) opt->field = int_scalar(v__, "opt$" key, opt->field); \
} while (0)
    SET_INT_FIELD("out_n", out_n);
    SET_INT_FIELD("min_seed_len", min_len);
    SET_INT_FIELD("threads", n_thread);
    SET_INT_FIELD("match_score", a);
    SET_INT_FIELD("mismatch_penalty", b);
    SET_INT_FIELD("gap_open", q);
    SET_INT_FIELD("gap_extend", e);
#undef SET_INT_FIELD
    if (opt->n_thread < 1) opt->n_thread = 1;
}

SEXP RC_mb_index_load(SEXP prefix_x, SEXP meth_x) {
    const char *prefix = chr_scalar(prefix_x, "prefix");
    int meth = is_true_scalar(meth_x);
    mb_idx_t *ptr = mb_idx_load(prefix, meth);
    if (ptr == NULL) Rf_error("failed to load minibwa index at prefix: %s", prefix);
    RmbIndex *idx = R_Calloc(1, RmbIndex);
    idx->ptr = ptr;
    idx->meth = meth;
    SEXP xptr = PROTECT(R_MakeExternalPtr(idx, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(xptr, idx_finalizer, TRUE);
    set_class(xptr, "rminibwa_idx");
    UNPROTECT(1);
    return xptr;
}

SEXP RC_mb_index_build(SEXP fasta_x, SEXP prefix_x, SEXP meth_x, SEXP threads_x, SEXP low_memory_x) {
    const char *fasta = chr_scalar(fasta_x, "fasta");
    const char *prefix = chr_scalar(prefix_x, "prefix");
    int threads = int_scalar(threads_x, "threads", 1);
    if (threads < 1) Rf_error("threads must be positive");
    char threads_buf[32];
    snprintf(threads_buf, sizeof threads_buf, "%d", threads);
    char *argv[9];
    int argc = 0;
    argv[argc++] = "index";
    argv[argc++] = "-t";
    argv[argc++] = threads_buf;
    if (is_true_scalar(low_memory_x)) argv[argc++] = "-l";
    if (is_true_scalar(meth_x)) argv[argc++] = "--meth";
    argv[argc++] = (char *) fasta;
    argv[argc++] = (char *) prefix;
    argv[argc] = NULL;
    int rc = main_index(argc, argv);
    if (rc != 0) Rf_error("minibwa index build failed with status %d", rc);
    return Rf_ScalarLogical(TRUE);
}

SEXP RC_mb_index_contigs(SEXP index_x) {
    RmbIndex *idx = rminibwa_index_from_sexp(index_x);
    int32_t n = 0;
    while (mb_idx_ctg_name(idx->ptr, n) != NULL) ++n;
    SEXP names = PROTECT(Rf_allocVector(VECSXP, n));
    SEXP lengths = PROTECT(Rf_allocVector(REALSXP, n));
    for (int32_t i = 0; i < n; ++i) {
        const char *nm = mb_idx_ctg_name(idx->ptr, i);
        size_t len = strlen(nm);
        SEXP raw = PROTECT(Rf_allocVector(RAWSXP, (R_xlen_t) len));
        memcpy(RAW(raw), nm, len);
        SET_VECTOR_ELT(names, i, raw);
        UNPROTECT(1);
        REAL(lengths)[i] = (double) mb_idx_ctg_len(idx->ptr, i);
    }
    SEXP out = PROTECT(Rf_allocVector(VECSXP, 2));
    SET_VECTOR_ELT(out, 0, names);
    SET_VECTOR_ELT(out, 1, lengths);
    SEXP out_names = PROTECT(Rf_allocVector(STRSXP, 2));
    SET_STRING_ELT(out_names, 0, Rf_mkChar("name"));
    SET_STRING_ELT(out_names, 1, Rf_mkChar("length"));
    Rf_setAttrib(out, R_NamesSymbol, out_names);
    UNPROTECT(4);
    return out;
}

static const char *meth_name_to_code(SEXP meth_x, int *code) {
    const char *meth = chr_scalar(meth_x, "meth");
    if (strcmp(meth, "none") == 0) { *code = 0; return meth; }
    if (strcmp(meth, "c2t") == 0) { *code = 1; return meth; }
    if (strcmp(meth, "g2a") == 0) { *code = 2; return meth; }
    Rf_error("meth must be one of 'none', 'c2t', or 'g2a'");
    return meth;
}

static const char *map_name_from_sexp(SEXP name_x, char **name_buf) {
    const char *qname = "read0";
    *name_buf = NULL;
    if (name_x != R_NilValue) {
        if (TYPEOF(name_x) == RAWSXP) {
            R_xlen_t nl = XLENGTH(name_x);
            *name_buf = R_Calloc((size_t) nl + 1, char);
            memcpy(*name_buf, RAW(name_x), (size_t) nl);
            (*name_buf)[nl] = '\0';
            qname = *name_buf;
        } else if (TYPEOF(name_x) == STRSXP) {
            qname = chr_scalar(name_x, "name");
        } else {
            Rf_error("name must be NULL, raw, or a non-missing character scalar");
        }
    }
    return qname;
}

SEXP RC_mb_map_raw(SEXP x, SEXP index_x, SEXP opt_x, SEXP name_x, SEXP meth_x) {
    if (TYPEOF(x) != RAWSXP) Rf_error("x must be a raw vector of sequence bytes");
    R_xlen_t n = XLENGTH(x);
    if (n <= 0) Rf_error("x must not be empty");
    if (n > INT_MAX) Rf_error("x is longer than minibwa's int32 query length limit");
    RmbIndex *idx = rminibwa_index_from_sexp(index_x);
    mb_opt_t opt;
    apply_options(opt_x, &opt);
    int meth = 0;
    meth_name_to_code(meth_x, &meth);

    char *name_buf = NULL;
    const char *qname = map_name_from_sexp(name_x, &name_buf);
    int32_t n_hit = 0;
    mb_hit_t *hit = mb_map(&opt, idx->ptr, (int32_t) n, (const char *) RAW(x), meth, &n_hit, NULL, qname);
    if (name_buf != NULL) R_Free(name_buf);
    RmbAlignBatch *batch = copy_hits_to_batch(hit, n_hit, (int32_t) n, index_x);
    free_hits(hit, n_hit);
    return rminibwa_align_xptr_new(batch);
}

SEXP RC_mb_map_count_raw(SEXP x, SEXP index_x, SEXP opt_x, SEXP name_x, SEXP meth_x) {
    if (TYPEOF(x) != RAWSXP) Rf_error("x must be a raw vector of sequence bytes");
    R_xlen_t n = XLENGTH(x);
    if (n <= 0) Rf_error("x must not be empty");
    if (n > INT_MAX) Rf_error("x is longer than minibwa's int32 query length limit");
    RmbIndex *idx = rminibwa_index_from_sexp(index_x);
    mb_opt_t opt;
    apply_options(opt_x, &opt);
    int meth = 0;
    meth_name_to_code(meth_x, &meth);

    char *name_buf = NULL;
    const char *qname = map_name_from_sexp(name_x, &name_buf);
    int32_t n_hit = 0;
    mb_hit_t *hit = mb_map(&opt, idx->ptr, (int32_t) n, (const char *) RAW(x), meth, &n_hit, NULL, qname);
    if (name_buf != NULL) R_Free(name_buf);
    free_hits(hit, n_hit);
    return Rf_ScalarInteger(n_hit);
}

SEXP RC_mb_align_n(SEXP batch_x) {
    RmbAlignBatch *batch = rminibwa_align_mut_from_sexp(batch_x);
    return Rf_ScalarReal((double) batch->n);
}

static int is_i64_col(const char *name) {
    return strcmp(name, "ts") == 0 || strcmp(name, "te") == 0 || strcmp(name, "tlen") == 0 ||
           strcmp(name, "target_start") == 0 || strcmp(name, "target_end") == 0 || strcmp(name, "target_length") == 0;
}

SEXP RC_mb_align_col(SEXP batch_x, SEXP name_x) {
    const char *name = chr_scalar(name_x, "name");
    if (is_i64_col(name)) return rminibwa_align_altreal(batch_x, name);
    return rminibwa_align_altinteger(batch_x, name);
}

SEXP RC_mb_align_cigar_words(SEXP batch_x) {
    RmbAlignBatch *batch = rminibwa_align_mut_from_sexp(batch_x);
    R_xlen_t nbytes = (R_xlen_t) (batch->n_cigar_words * sizeof(uint32_t));
    SEXP out = PROTECT(Rf_allocVector(RAWSXP, nbytes));
    if (nbytes > 0) memcpy(RAW(out), batch->cigar_words, (size_t) nbytes);
    UNPROTECT(1);
    return out;
}

const RmbAlignBatch *Rminibwa_align_from_sexp(SEXP x) {
    return rminibwa_align_mut_from_sexp(x);
}

size_t Rminibwa_align_n(const RmbAlignBatch *x) {
    return x ? x->n : 0;
}

const int32_t *Rminibwa_align_i32_col(const RmbAlignBatch *x, const char *name) {
    if (x == NULL || name == NULL) return NULL;
    if (strcmp(name, "read") == 0 || strcmp(name, "read_id") == 0) return x->read_id;
    if (strcmp(name, "tid") == 0 || strcmp(name, "target_id") == 0) return x->target_id;
    if (strcmp(name, "qlen") == 0 || strcmp(name, "query_length") == 0) return x->query_length;
    if (strcmp(name, "qs") == 0 || strcmp(name, "query_start") == 0) return x->query_start;
    if (strcmp(name, "qe") == 0 || strcmp(name, "query_end") == 0) return x->query_end;
    if (strcmp(name, "strand") == 0) return x->strand;
    if (strcmp(name, "mapq") == 0) return x->mapq;
    if (strcmp(name, "score") == 0) return x->score;
    if (strcmp(name, "matches") == 0) return x->matches;
    if (strcmp(name, "block_length") == 0 || strcmp(name, "blen") == 0) return x->block_length;
    if (strcmp(name, "flags") == 0) return x->flags;
    if (strcmp(name, "n_sub") == 0) return x->n_sub;
    if (strcmp(name, "cigar_offset") == 0) return x->cigar_offset;
    if (strcmp(name, "cigar_n") == 0) return x->cigar_n;
    return NULL;
}

const int64_t *Rminibwa_align_i64_col(const RmbAlignBatch *x, const char *name) {
    if (x == NULL || name == NULL) return NULL;
    if (strcmp(name, "tlen") == 0 || strcmp(name, "target_length") == 0) return x->target_length;
    if (strcmp(name, "ts") == 0 || strcmp(name, "target_start") == 0) return x->target_start;
    if (strcmp(name, "te") == 0 || strcmp(name, "target_end") == 0) return x->target_end;
    return NULL;
}

const uint32_t *Rminibwa_align_cigar_words(const RmbAlignBatch *x, size_t *n_words) {
    if (n_words != NULL) *n_words = x ? x->n_cigar_words : 0;
    return x ? x->cigar_words : NULL;
}

const int32_t *Rminibwa_align_cigar_i32_col(const RmbAlignBatch *x, const char *name) {
    if (x == NULL || name == NULL) return NULL;
    if (strcmp(name, "cigar_offset") == 0) return x->cigar_offset;
    if (strcmp(name, "cigar_n") == 0) return x->cigar_n;
    return NULL;
}
