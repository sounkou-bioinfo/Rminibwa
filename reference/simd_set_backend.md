# Select the runtime SIMD backend

Rminibwa compiles staged KSW backend objects and switches among them at
runtime. Use `"auto"` for the best available backend, or force one of
`"scalar"`, `"sse4"`, or `"avx2"` for diagnostics and benchmarks. The
`"scalar"` backend is the portable SIMDe baseline with native intrinsics
disabled; compilers may still lower portable vector types efficiently.

## Usage

``` r
simd_set_backend(backend = "auto")
```

## Arguments

- backend:

  Character scalar backend name.

## Value

The selected backend, invisibly.
