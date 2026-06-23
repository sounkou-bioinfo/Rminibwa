# Test whether the minibwa CLI is available

Test whether the minibwa CLI is available

## Usage

``` r
minibwa_available(path = Sys.getenv("RMINIBWA_MINIBWA", unset = "minibwa"))
```

## Arguments

- path:

  Executable name or path.

## Value

`TRUE` when an executable can be resolved, otherwise `FALSE`.
