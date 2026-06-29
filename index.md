# Rminibwa

Rminibwa is an R interface to [minibwa](https://github.com/lh3/minibwa),
Heng Li’s genomic read aligner. It vendors a pinned upstream source
tree, builds a package-provided `minibwa` executable, and exposes a
native C-backed API for in-process alignment work.

The CLI wrappers are kept close to upstream behavior. The native
interface uses raw query bytes, external-pointer alignment batches,
ALTREP column views, and a small C header for downstream packages.
SIMD-sensitive KSW code is compiled as separate staged backends and
selected at runtime.

## Installation

``` r

install.packages(
  "Rminibwa",
  repos = c(
    "https://sounkou-bioinfo.r-universe.dev",
    "https://cloud.r-project.org"
  )
)
```

From a local checkout:

``` bash
R CMD INSTALL .
```

Rminibwa builds `minibwa` from the vendored source during package
installation. No external `minibwa` executable is needed for normal use.

## CLI wrapper

``` r

library(Rminibwa)

minibwa_version()
prefix <- minibwa_index("ref.fa", threads = 8)
aln <- minibwa_map(prefix, "reads.fq.gz", format = "paf", threads = 8)
```

[`minibwa_map()`](https://sounkou-bioinfo.github.io/Rminibwa/reference/minibwa_map.md)
captures output by default. Pass `output = "aln.sam"` or
`output = "aln.paf"` to write directly to a file.

## Native batches

The native path avoids data frames in the hot alignment shape.
[`mb_map()`](https://sounkou-bioinfo.github.io/Rminibwa/reference/mb_map.md)
returns an external-pointer batch; columns are exposed lazily to R and
can also be read from C.

``` r

library(Rminibwa)

td <- tempfile("rminibwa-readme-")
dir.create(td)
ref <- paste(rep("ACGT", 1000), collapse = "")
fa <- file.path(td, "ref.fa")
writeLines(c(">chr1", ref), fa, useBytes = TRUE)
prefix <- file.path(td, "idx")

mb_index_build(fa, prefix, threads = 1L)
idx <- mb_index_load(prefix)
aln <- charToRaw(substr(ref, 1L, 100L)) |>
  mb_map(idx, opt = mb_opts("sr", out_n = 0L), name = charToRaw("read1"))

c(
  n = mb_align_n(aln),
  first_tid = mb_align_col(aln, "tid")[[1]],
  first_qe = mb_align_col(aln, "qe")[[1]],
  cigar_bytes = length(mb_align_cigar_words(aln))
)
#>           n   first_tid    first_qe cigar_bytes 
#>          51           0         100         204
```

## C consumers

`Rminibwa.h` is installed under `inst/include` and resolves the runtime
API with `R_GetCCallable()`. Downstream packages can use
`LinkingTo: Rminibwa` for the header and import Rminibwa at runtime
before calling the function pointers.

``` c
#include <Rminibwa.h>

SEXP summarize_alignment(SEXP x)
{
    const RmbAlignBatch *batch = Rminibwa_align_from_sexp(x);
    size_t n = Rminibwa_align_n(batch);
    const int32_t *tid = Rminibwa_align_i32_col(batch, "tid");
    return Rf_ScalarInteger(n && tid ? tid[0] : NA_INTEGER);
}
```

A complete in-process consumer compiled with Rtinycc is in
`vignettes/downstream-c-api.Rmd` and
`inst/capi/rminibwa_tinycc_consumer.c`.

## SIMD dispatch

The package follows the RsimdDispatch style: compile portable and
ISA-specific objects separately, then select an available backend at
runtime. On x86_64 this can include SSE4 and AVX2. On other
architectures the portable backend is used.

``` r

simd_info()
#> <rminibwa SIMD dispatch>
#>   selected:  avx2
#>   requested: auto
#>   mode:      rminibwa-ksw-staged
#>   compiled:  scalar, sse4, avx2
#>   available: scalar, sse4, avx2
#>   cpu:       <none>
#>   target:    x86_64
```

AVX-512 is not built by default. It is useful for experiments, but it
would add a larger dispatch and CI surface. Use `make asm` to inspect
the actual instruction families in local builds.

## Benchmarks

`make rdm MINIBWA_BINDINGS_ROOT=/path/to/minibwa-bindings` renders the
optional benchmark tables below. The workload uses a chrM-sized random
reference and an indel-mutated read so KSW is exercised.

## Development

``` bash
make rd       # roxygen docs
make test0    # tinytest suite against the installed package
make check    # R CMD check --as-cran --no-manual
make pkgdown  # local site build
make asm      # inspect staged backend assembly
```
