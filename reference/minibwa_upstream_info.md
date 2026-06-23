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
#> [1] "0.2-r379-dirty"
#> 
#> $Commit
#> [1] "f0fd108456d459e5da3755f29c8700dc7f6fe0df"
#> 
```
