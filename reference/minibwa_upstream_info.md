# Report the pinned upstream minibwa source

Rminibwa pins a specific upstream `minibwa` commit for native-library
work. `minibwa_upstream_info()` reports the installed copy of that
provenance metadata.

## Usage

``` r
minibwa_upstream_info()
```

## Value

A named list with fields such as `Component`, `Version`, `Repository`,
`Commit`, and `PatchDirectory`.

## Examples

``` r
minibwa_upstream_info()[c("Version", "Commit")]
#> $Version
#> [1] "0.3-r391"
#> 
#> $Commit
#> [1] "a679d17cb822c6cca67ab78c439f9c9826f1b07d"
#> 
```
