# Rminibwa 0.0.0.9000

- Bootstrap `Rminibwa` as a GPL (>= 2) R package wrapping pinned upstream
  `lh3/minibwa` C sources.
- Add native bindings for index build/load, raw-vector query mapping, native
  external-pointer alignment batches, ALTREP column views, and packed CIGAR
  accessors.
- Install `inst/include/Rminibwa.h` and register C-callable accessors so
  downstream packages can consume alignment batches directly from C.
- Add staged KSW SIMD dispatch following the `RsimdDispatch` pattern: portable
  scalar/SIMDe fallback, SSE4, and AVX2 backends are compiled as separate
  objects and selected at runtime without global ISA compiler flags.
- Add AVX2 dual-gap `extd2` support from the minibwa binding work and keep it
  as an optional runtime-dispatched backend.
- Vendor minibwa at commit `f0fd108456d459e5da3755f29c8700dc7f6fe0df` with a
  patch queue and provenance metadata.
- Add Rtinycc inline downstream-C examples in the README, vignette, and tests.
- Add developer-only benchmarks comparing internal staged backends and external
  Python/Rust bindings built with native AVX2 flags.
- Add pkgdown configuration and GitHub Actions workflows for standard R CMD
  check, Fedora/R-devel CRAN-style check, and GitHub Pages deployment.
