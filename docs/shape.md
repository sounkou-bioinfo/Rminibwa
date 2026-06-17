# Rminibwa shape

This repository is a home for an R-facing `minibwa` integration, not a rewrite of
`minibwa`. The goal is to reuse Heng Li's upstream C code and expose it through
R in a way that can also exercise our runtime SIMD dispatch framework.

## Initial layers

1. **CLI layer first**
   - `minibwa_index()` and `minibwa_map()` wrap an installed `minibwa` binary.
   - This gives immediate exact-output compatibility with upstream.
   - It is also the validation oracle for later native bindings.

2. **Native library layer next**
   - Load a `.l2b`/`.mbw` index once into an R external pointer.
   - Map one read or a batch of reads in-process through `mb_map()` and
     `mb_map_batch()`.
   - Return either PAF/SAM-compatible text or a compact data frame of hits.
   - Keep R-specific allocation, error handling, and object lifetimes outside
     the upstream core.

3. **Runtime SIMD dispatch layer**
   - Start by isolating the alignment kernels (`ksw2_extz2_sse.c`,
     `ksw2_extd2_sse.c`, `ksw2_ll_sse.c`) behind a small operation table.
   - Compile only those SIMD-sensitive translation units per backend where
     possible; keep index loading, seeding, chaining, formatting, and R glue
     baseline-compiled once.
   - Reuse `RsimdDispatch` conventions for `auto`, explicit backend selection,
     compiled-backend reporting, and CPU-guarded runtime selection.

## Proposed public R API

### CLI-compatible API

```r
minibwa_available()
minibwa_path()
minibwa_version()
minibwa_index(reference, prefix = NULL, threads = NULL, meth = FALSE)
minibwa_map(index, reads, output = NULL, format = c("sam", "paf"), threads = NULL)
minibwa_cli(args)
```

### Native API sketch

```r
idx <- mb_index_load(prefix, meth = FALSE)
mb_index_info(idx)
mb_map_one(idx, sequence, name = "read1", preset = "adap")
mb_map_batch(idx, sequences, names = names(sequences), paired = FALSE)
mb_index_unload(idx)
```

Native return shape should be deliberately boring at first:

- `qname`, `qlen`, `qs`, `qe`, `strand`
- `tname`, `tlen`, `ts`, `te`
- `matches`, `block_len`, `mapq`
- `is_primary`, `cigar`, optional tags

That is close to PAF and easy to compare against CLI output.

## Upstream and vendoring strategy

- Keep upstream source provenance explicit: repository URL, commit, checksum,
  and local patches.
- Prefer a `.sync/minibwa` checkout during development and a reproducible vendor
  script before checking source into `src/vendor/minibwa`.
- Default to `gpl=0` for vendored builds unless we explicitly choose a GPL
  package license. Upstream `minibwa` itself is MIT, but its optional low-memory
  BWT code includes GPL components.
- Keep patches as named files under `tools/patches/`; do not silently edit
  vendored C.

## SIMD questions to resolve

- Do we need a scalar fallback for CRAN/HPC portability, or is the upstream
  SSE4.2/NEON requirement acceptable?
- Should dispatch happen only at the KSW alignment kernels, or at a larger
  `mb_map_batch()` backend boundary?
- Can we convert the SSE/NEON compatibility layer to SIMDe to get cleaner wasm
  and portable fallback behavior?
- Which backend names should R expose: `host`, `sse2`, `sse41`, `neon`,
  `wasm128`, `scalar`?
- Should the native API emit text identical to CLI formatting, structured R
  records, or both?

## Validation plan

1. Build upstream `minibwa` at a pinned commit and run the bundled chrM example.
2. Add R tests that call the CLI wrappers only when `minibwa` is available.
3. Once native bindings exist, compare native output to CLI output on the same
   index/read fixtures.
4. Once multiple backends exist, compare each backend to the default upstream
   backend and include backend metadata in test logs.
