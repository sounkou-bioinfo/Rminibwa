# Locate the minibwa executable

`minibwa_path()` resolves the command-line executable used by the
CLI-backed helpers. By default it first consults the `RMINIBWA_MINIBWA`
environment variable and otherwise searches for `minibwa` on `PATH`.

## Usage

``` r
minibwa_path(
  path = Sys.getenv("RMINIBWA_MINIBWA", unset = "minibwa"),
  must_work = TRUE
)
```

## Arguments

- path:

  Executable name or path.

- must_work:

  If `TRUE`, throw an error when the executable cannot be found.

## Value

A scalar character path, or `NA_character_` when `must_work = FALSE` and
no executable is found.
