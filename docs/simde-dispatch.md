# SIMDe dispatch plan

Decision: use the `RsimdDispatch`/`SIMDe` style for the native Rminibwa backend.
SSE4.2 is common enough to be a fast default on x86_64, but we still want one
source path that can also compile for portable fallback, ARM/NEON, and wasm.
For now `RsimdDispatch` is a suggested developer dependency; promote it to
`LinkingTo` when native `src/` code lands.

## Target boundary

Dispatch should start at the KSW alignment kernels rather than the whole mapper:

- `ksw_extz2_sse()`
- `ksw_extd2_sse()`
- possibly `ksw2_ll_sse.c` symbols if later code paths need them

Everything else should stay baseline-compiled once:

- index loading
- BWT/seeding
- chaining
- pairing
- formatting
- R external-pointer glue

## Source adaptation

Upstream currently selects native SSE or the small `s2n-lite.h` NEON bridge in
individual KSW files. For Rminibwa we should patch that layer to include SIMDe
instead, preferably behind a tiny project-local header, for example:

```c
/* rminibwa_simde.h */
#define SIMDE_ENABLE_NATIVE_ALIASES
#include <simde/x86/sse2.h>
#include <simde/x86/sse4.1.h>
```

Then the upstream `_mm_*` code can remain close to unchanged while the build
system chooses whether SIMDe lowers to native intrinsics or portable code.

## Variant symbols

Compile the same kernel sources more than once with symbol-renaming macros:

```c
#define ksw_extz2_sse rmb_ksw_extz2_portable
#define ksw_extd2_sse rmb_ksw_extd2_portable
```

and similarly for native backends such as `rmb_ksw_extz2_sse42`. A baseline
wrapper keeps the original symbols used by `align.c` and forwards through the
resolved operation table.

## Backend names

Initial names:

- `portable` — SIMDe fallback with no CPU-specific compile flags
- `sse42` — x86_64 native SSE4.2 / POPCNT build
- `neon` — ARM native NEON build where available
- `wasm128` — future wasm SIMD128 build if supported

Avoid exposing `avx2` unless we add real AVX2-width kernels. The upstream KSW
code is SSE-width, so an AVX2 label would be misleading unless the generated
machine code or a rewritten kernel actually benefits from AVX2.

## R surface

Expose the same style as `RsimdDispatch`:

```r
minibwa_simd_backend()
minibwa_simd_set_backend("auto")
minibwa_simd_info()
```

or reuse shorter names if the package has no other SIMD subsystem:

```r
simd_backend()
simd_set_backend("auto")
simd_info()
```

The info object should include compiled backends, CPU-supported backends,
selected backend, and per-operation selected backend.

## Validation

For every backend:

1. run the same native mapping fixture;
2. compare to the upstream CLI output at a pinned commit;
3. compare portable SIMDe vs native SSE/NEON results;
4. log selected backend and compiled backend metadata in tests.
