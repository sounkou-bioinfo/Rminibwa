# Build a minibwa index with the native library

Builds index files at `prefix` using the vendored minibwa C library. The
package is GPL, so the native build includes minibwa's GPL-compatible
low-memory indexing path; `low_memory = FALSE` uses the default
`libsais` path.

## Usage

``` r
mb_index_build(fasta, prefix, meth = FALSE, threads = 1L, low_memory = FALSE)
```

## Arguments

- fasta:

  Reference FASTA path. Gzip-compressed input is supported by
  minibwa/zlib.

- prefix:

  Output prefix for `.l2b`, `.mbw`, and optionally `.meth.mbw`.

- meth:

  Build the directional methylation index as well.

- threads:

  Number of index-build threads requested by minibwa.

- low_memory:

  Use minibwa's low-memory BWT builder.

## Value

`prefix`, invisibly.
