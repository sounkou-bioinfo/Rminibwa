# Locate the packaged minibwa executable

`minibwa_path()` resolves the command-line executable used by the
CLI-backed helpers. By default it returns the executable built from the
vendored minibwa sources and installed with Rminibwa. Supplying `path`
is mainly for developer comparisons against another minibwa build.

## Usage

``` r
minibwa_path(path = NULL, must_work = TRUE)
```

## Arguments

- path:

  Optional executable name or path. When `NULL`, use Rminibwa's packaged
  minibwa executable.

- must_work:

  If `TRUE`, throw an error when the executable cannot be found.

## Value

A scalar character path, or `NA_character_` when `must_work = FALSE` and
no executable is found.
