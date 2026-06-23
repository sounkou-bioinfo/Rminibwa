# Inspect native minibwa alignment batches

`mb_align_col()` returns ALTREP views over native alignment columns.
These views do not copy columns unless R forces materialization. C
consumers should prefer the installed `Rminibwa.h` API.

## Usage

``` r
mb_align_n(x)

mb_align_col(x, name)

mb_align_cigar_words(x)
```

## Arguments

- x:

  Native alignment batch returned by
  [`mb_map()`](https://sounkou-bioinfo.github.io/Rminibwa/reference/mb_map.md).

- name:

  Column name.

## Value

`mb_align_n()` returns the number of records. `mb_align_col()` returns
an ALTREP integer or real vector. `mb_align_cigar_words()` returns a raw
debug copy of packed minibwa CIGAR words.
