# Native minibwa options

Returns a plain list consumed by native mapping functions. Only the
selected options are exposed initially; all other fields use minibwa
defaults for the chosen preset.

## Usage

``` r
mb_opts(
  preset = c("adap", "sr", "lr"),
  paired = NULL,
  methylation = NULL,
  out_n = NULL,
  min_seed_len = NULL,
  threads = NULL,
  match_score = NULL,
  mismatch_penalty = NULL,
  gap_open = NULL,
  gap_extend = NULL
)
```

## Arguments

- preset:

  One of `"adap"`, `"sr"`, or `"lr"`.

- paired, methylation:

  Optional logical overrides for minibwa flags.

- out_n, min_seed_len, threads, match_score, mismatch_penalty, gap_open,
  gap_extend:

  Optional integer overrides for common minibwa fields.

## Value

A plain list.
