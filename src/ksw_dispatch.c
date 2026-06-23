#include <R.h>
#include <Rinternals.h>
#include <stdint.h>
#include <string.h>

#include "rminibwa_internal.h"
#include "vendor/minibwa/ksw2.h"
#include "rminibwa_simd_config.h"

#ifndef RMB_HAVE_SSE4
#define RMB_HAVE_SSE4 0
#endif
#ifndef RMB_HAVE_AVX2
#define RMB_HAVE_AVX2 0
#endif

#define RMB_KSW_EXTZ2_ARGS void *km, int qlen, const uint8_t *query, int tlen, const uint8_t *target, int8_t m, const int8_t *mat, int8_t q, int8_t e, int w, int zdrop, int end_bonus, int flag, ksw_extz_t *ez
#define RMB_KSW_EXTD2_ARGS void *km, int qlen, const uint8_t *query, int tlen, const uint8_t *target, int8_t m, const int8_t *mat, int8_t gapo, int8_t gape, int8_t gapo2, int8_t gape2, int w, int zdrop, int end_bonus, int flag, ksw_extz_t *ez

typedef void (*rmb_ksw_extz2_fn)(RMB_KSW_EXTZ2_ARGS);
typedef void (*rmb_ksw_extd2_fn)(RMB_KSW_EXTD2_ARGS);
typedef void *(*rmb_ksw_ll_qinit_fn)(void *km, int size, int qlen, const uint8_t *query, int m, const int8_t *mat);
typedef ksw_llrst_t (*rmb_ksw_ll_core_fn)(void *q_, int tlen, const uint8_t *target, int gapo, int gape, int xtra);
typedef int (*rmb_ksw_ll_i16_fn)(void *q, int tlen, const uint8_t *target, int gapo, int gape, int *qe, int *te);

/* Scalar/SIMDe fallback, always compiled. */
void rmb_ksw_extz2_scalar(RMB_KSW_EXTZ2_ARGS);
void rmb_ksw_extd2_scalar(RMB_KSW_EXTD2_ARGS);
void *rmb_ksw_ll_qinit_scalar(void *km, int size, int qlen, const uint8_t *query, int m, const int8_t *mat);
ksw_llrst_t rmb_ksw_ll_u8_core_scalar(void *q_, int tlen, const uint8_t *target, int gapo, int gape, int xtra);
ksw_llrst_t rmb_ksw_ll_i16_core_scalar(void *q_, int tlen, const uint8_t *target, int gapo, int gape, int xtra);
int rmb_ksw_ll_i16_scalar(void *q, int tlen, const uint8_t *target, int gapo, int gape, int *qe, int *te);

#if RMB_HAVE_SSE4
void rmb_ksw_extz2_sse4(RMB_KSW_EXTZ2_ARGS);
void rmb_ksw_extd2_sse4(RMB_KSW_EXTD2_ARGS);
void *rmb_ksw_ll_qinit_sse4(void *km, int size, int qlen, const uint8_t *query, int m, const int8_t *mat);
ksw_llrst_t rmb_ksw_ll_u8_core_sse4(void *q_, int tlen, const uint8_t *target, int gapo, int gape, int xtra);
ksw_llrst_t rmb_ksw_ll_i16_core_sse4(void *q_, int tlen, const uint8_t *target, int gapo, int gape, int xtra);
int rmb_ksw_ll_i16_sse4(void *q, int tlen, const uint8_t *target, int gapo, int gape, int *qe, int *te);
#endif

#if RMB_HAVE_AVX2
void rmb_ksw_extz2_avx2(RMB_KSW_EXTZ2_ARGS);
void rmb_ksw_extd2_avx2(RMB_KSW_EXTD2_ARGS);
void *rmb_ksw_ll_qinit_avx2(void *km, int size, int qlen, const uint8_t *query, int m, const int8_t *mat);
ksw_llrst_t rmb_ksw_ll_u8_core_avx2(void *q_, int tlen, const uint8_t *target, int gapo, int gape, int xtra);
ksw_llrst_t rmb_ksw_ll_i16_core_avx2(void *q_, int tlen, const uint8_t *target, int gapo, int gape, int xtra);
int rmb_ksw_ll_i16_avx2(void *q, int tlen, const uint8_t *target, int gapo, int gape, int *qe, int *te);
#endif

typedef struct RmbKswBackend {
    const char *name;
    int compiled;
    int (*cpu_supported)(void);
    rmb_ksw_extz2_fn extz2;
    rmb_ksw_extd2_fn extd2;
    rmb_ksw_ll_qinit_fn ll_qinit;
    rmb_ksw_ll_core_fn ll_u8_core;
    rmb_ksw_ll_core_fn ll_i16_core;
    rmb_ksw_ll_i16_fn ll_i16;
} RmbKswBackend;

static int rmb_cpu_has_scalar(void) { return 1; }

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define RMB_X86 1
#else
#define RMB_X86 0
#endif

#if RMB_X86 && (defined(__GNUC__) || defined(__clang__))
#include <cpuid.h>
static int rmb_cpuid(unsigned int leaf, unsigned int subleaf, unsigned int *a, unsigned int *b, unsigned int *c, unsigned int *d) {
    return __get_cpuid_count(leaf, subleaf, a, b, c, d) != 0;
}
static unsigned long long rmb_xgetbv(unsigned int index) {
    unsigned int eax = 0, edx = 0;
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
    return ((unsigned long long)edx << 32) | eax;
}
static int rmb_cpu_has_sse4(void) {
    unsigned int a = 0, b = 0, c = 0, d = 0;
    return rmb_cpuid(1, 0, &a, &b, &c, &d) && ((c & (1u << 19)) != 0);
}
static int rmb_cpu_has_avx2(void) {
    unsigned int a = 0, b = 0, c = 0, d = 0;
    if (!rmb_cpuid(1, 0, &a, &b, &c, &d)) return 0;
    if ((c & (1u << 27)) == 0 || (c & (1u << 28)) == 0) return 0;
    if ((rmb_xgetbv(0) & 0x6u) != 0x6u) return 0;
    if (!rmb_cpuid(7, 0, &a, &b, &c, &d)) return 0;
    return (b & (1u << 5)) != 0;
}
#else
static int rmb_cpu_has_sse4(void) { return 0; }
static int rmb_cpu_has_avx2(void) { return 0; }
#endif

static const RmbKswBackend rmb_backends[] = {
    {"scalar", 1, rmb_cpu_has_scalar, rmb_ksw_extz2_scalar, rmb_ksw_extd2_scalar, rmb_ksw_ll_qinit_scalar, rmb_ksw_ll_u8_core_scalar, rmb_ksw_ll_i16_core_scalar, rmb_ksw_ll_i16_scalar},
#if RMB_HAVE_SSE4
    {"sse4", 1, rmb_cpu_has_sse4, rmb_ksw_extz2_sse4, rmb_ksw_extd2_sse4, rmb_ksw_ll_qinit_sse4, rmb_ksw_ll_u8_core_sse4, rmb_ksw_ll_i16_core_sse4, rmb_ksw_ll_i16_sse4},
#else
    {"sse4", 0, rmb_cpu_has_sse4, NULL, NULL, NULL, NULL, NULL, NULL},
#endif
#if RMB_HAVE_AVX2
    {"avx2", 1, rmb_cpu_has_avx2, rmb_ksw_extz2_avx2, rmb_ksw_extd2_avx2, rmb_ksw_ll_qinit_avx2, rmb_ksw_ll_u8_core_avx2, rmb_ksw_ll_i16_core_avx2, rmb_ksw_ll_i16_avx2},
#else
    {"avx2", 0, rmb_cpu_has_avx2, NULL, NULL, NULL, NULL, NULL, NULL},
#endif
};

static const RmbKswBackend *rmb_active_backend = NULL;
static char rmb_requested_backend[16] = "auto";
static char rmb_selected_backend[16] = "uninitialized";
static uint64_t rmb_count_extz2 = 0;
static uint64_t rmb_count_extd2 = 0;
static uint64_t rmb_count_ll_qinit = 0;
static uint64_t rmb_count_ll_u8_core = 0;
static uint64_t rmb_count_ll_i16_core = 0;
static uint64_t rmb_count_ll_i16 = 0;

static void rmb_reset_counters(void) {
    rmb_count_extz2 = 0;
    rmb_count_extd2 = 0;
    rmb_count_ll_qinit = 0;
    rmb_count_ll_u8_core = 0;
    rmb_count_ll_i16_core = 0;
    rmb_count_ll_i16 = 0;
}

static size_t rmb_backend_count(void) {
    return sizeof(rmb_backends) / sizeof(rmb_backends[0]);
}

static const RmbKswBackend *rmb_find_backend(const char *backend) {
    for (size_t i = 0; i < rmb_backend_count(); ++i) {
        if (strcmp(backend, rmb_backends[i].name) == 0) return &rmb_backends[i];
    }
    return NULL;
}

static int rmb_backend_available(const RmbKswBackend *backend) {
    return backend != NULL && backend->compiled && backend->cpu_supported != NULL && backend->cpu_supported();
}

static const RmbKswBackend *rmb_auto_backend(void) {
    const RmbKswBackend *b;
    b = rmb_find_backend("avx2");
    if (rmb_backend_available(b)) return b;
    b = rmb_find_backend("sse4");
    if (rmb_backend_available(b)) return b;
    return rmb_find_backend("scalar");
}

static void rmb_select_backend(const RmbKswBackend *backend, const char *requested) {
    if (backend == NULL || !backend->compiled) Rf_error("Rminibwa SIMD backend is not compiled");
    if (!rmb_backend_available(backend)) Rf_error("Rminibwa SIMD backend '%s' is not supported on this CPU/runtime", backend->name);
    rmb_active_backend = backend;
    snprintf(rmb_requested_backend, sizeof rmb_requested_backend, "%s", requested);
    snprintf(rmb_selected_backend, sizeof rmb_selected_backend, "%s", backend->name);
}

void rminibwa_ksw_init_dispatch(void) {
    if (rmb_active_backend == NULL) rmb_select_backend(rmb_auto_backend(), "auto");
}

void rminibwa_ksw_set_backend(const char *backend) {
    if (backend == NULL || backend[0] == '\0') Rf_error("backend must be a non-empty string");
    if (strcmp(backend, "auto") == 0) {
        rmb_select_backend(rmb_auto_backend(), "auto");
        return;
    }
    const RmbKswBackend *b = rmb_find_backend(backend);
    if (b == NULL) Rf_error("unknown Rminibwa SIMD backend '%s'", backend);
    if (!b->compiled) Rf_error("Rminibwa SIMD backend '%s' was not compiled into this build", backend);
    rmb_select_backend(b, backend);
}

const char *rminibwa_ksw_backend(void) {
    rminibwa_ksw_init_dispatch();
    return rmb_selected_backend;
}

static SEXP rmb_backend_vec(int want_compiled, int want_cpu, int want_available) {
    size_t n = 0;
    for (size_t i = 0; i < rmb_backend_count(); ++i) {
        const RmbKswBackend *b = &rmb_backends[i];
        int ok = 1;
        if (want_compiled) ok = ok && b->compiled;
        if (want_cpu) ok = ok && b->cpu_supported();
        if (want_available) ok = ok && rmb_backend_available(b);
        if (ok) ++n;
    }
    SEXP out = PROTECT(Rf_allocVector(STRSXP, (R_xlen_t)n));
    R_xlen_t j = 0;
    for (size_t i = 0; i < rmb_backend_count(); ++i) {
        const RmbKswBackend *b = &rmb_backends[i];
        int ok = 1;
        if (want_compiled) ok = ok && b->compiled;
        if (want_cpu) ok = ok && b->cpu_supported();
        if (want_available) ok = ok && rmb_backend_available(b);
        if (ok) SET_STRING_ELT(out, j++, Rf_mkChar(b->name));
    }
    UNPROTECT(1);
    return out;
}

SEXP RC_simd_set_backend(SEXP backend_s) {
    if (TYPEOF(backend_s) != STRSXP || XLENGTH(backend_s) != 1 || STRING_ELT(backend_s, 0) == NA_STRING) {
        Rf_error("backend must be a non-missing character scalar");
    }
    rminibwa_ksw_set_backend(CHAR(STRING_ELT(backend_s, 0)));
    return Rf_mkString(rminibwa_ksw_backend());
}

SEXP RC_simd_backend(void) {
    return Rf_mkString(rminibwa_ksw_backend());
}

SEXP RC_simd_counters(SEXP reset_s) {
    int do_reset = TYPEOF(reset_s) == LGLSXP && XLENGTH(reset_s) == 1 && LOGICAL(reset_s)[0] == TRUE;
    SEXP out = PROTECT(Rf_allocVector(REALSXP, 6));
    SEXP names = PROTECT(Rf_allocVector(STRSXP, 6));
    const char *nms[] = {"extz2", "extd2", "ll_qinit", "ll_u8_core", "ll_i16_core", "ll_i16"};
    double vals[] = {
        (double) rmb_count_extz2,
        (double) rmb_count_extd2,
        (double) rmb_count_ll_qinit,
        (double) rmb_count_ll_u8_core,
        (double) rmb_count_ll_i16_core,
        (double) rmb_count_ll_i16
    };
    for (int i = 0; i < 6; ++i) {
        REAL(out)[i] = vals[i];
        SET_STRING_ELT(names, i, Rf_mkChar(nms[i]));
    }
    Rf_setAttrib(out, R_NamesSymbol, names);
    if (do_reset) rmb_reset_counters();
    UNPROTECT(2);
    return out;
}

SEXP RC_simd_info(void) {
    rminibwa_ksw_init_dispatch();
    SEXP out = PROTECT(Rf_allocVector(VECSXP, 7));
    SEXP names = PROTECT(Rf_allocVector(STRSXP, 7));
    const char *nms[] = {"dispatch_mode", "requested_backend", "selected_backend", "compiled_backends", "cpu_supported_backends", "available_backends", "target"};
    for (int i = 0; i < 7; ++i) SET_STRING_ELT(names, i, Rf_mkChar(nms[i]));
    SET_VECTOR_ELT(out, 0, Rf_mkString("rminibwa-ksw-staged"));
    SET_VECTOR_ELT(out, 1, Rf_mkString(rmb_requested_backend));
    SET_VECTOR_ELT(out, 2, Rf_mkString(rmb_selected_backend));
    SET_VECTOR_ELT(out, 3, rmb_backend_vec(1, 0, 0));
    SET_VECTOR_ELT(out, 4, rmb_backend_vec(0, 1, 0));
    SET_VECTOR_ELT(out, 5, rmb_backend_vec(0, 0, 1));
#if defined(__x86_64__) || defined(_M_X64)
    SET_VECTOR_ELT(out, 6, Rf_mkString("x86_64"));
#elif defined(__aarch64__) || defined(_M_ARM64)
    SET_VECTOR_ELT(out, 6, Rf_mkString("aarch64"));
#else
    SET_VECTOR_ELT(out, 6, Rf_mkString("unknown"));
#endif
    Rf_setAttrib(out, R_NamesSymbol, names);
    UNPROTECT(2);
    return out;
}

void ksw_extz2_sse(RMB_KSW_EXTZ2_ARGS) {
    rminibwa_ksw_init_dispatch();
    ++rmb_count_extz2;
    rmb_active_backend->extz2(km, qlen, query, tlen, target, m, mat, q, e, w, zdrop, end_bonus, flag, ez);
}

void ksw_extd2_sse(RMB_KSW_EXTD2_ARGS) {
    rminibwa_ksw_init_dispatch();
    ++rmb_count_extd2;
    rmb_active_backend->extd2(km, qlen, query, tlen, target, m, mat, gapo, gape, gapo2, gape2, w, zdrop, end_bonus, flag, ez);
}

void *ksw_ll_qinit(void *km, int size, int qlen, const uint8_t *query, int m, const int8_t *mat) {
    rminibwa_ksw_init_dispatch();
    ++rmb_count_ll_qinit;
    return rmb_active_backend->ll_qinit(km, size, qlen, query, m, mat);
}

ksw_llrst_t ksw_ll_u8_core(void *q_, int tlen, const uint8_t *target, int gapo, int gape, int xtra) {
    rminibwa_ksw_init_dispatch();
    ++rmb_count_ll_u8_core;
    return rmb_active_backend->ll_u8_core(q_, tlen, target, gapo, gape, xtra);
}

ksw_llrst_t ksw_ll_i16_core(void *q_, int tlen, const uint8_t *target, int gapo, int gape, int xtra) {
    rminibwa_ksw_init_dispatch();
    ++rmb_count_ll_i16_core;
    return rmb_active_backend->ll_i16_core(q_, tlen, target, gapo, gape, xtra);
}

int ksw_ll_i16(void *q, int tlen, const uint8_t *target, int gapo, int gape, int *qe, int *te) {
    rminibwa_ksw_init_dispatch();
    ++rmb_count_ll_i16;
    return rmb_active_backend->ll_i16(q, tlen, target, gapo, gape, qe, te);
}
