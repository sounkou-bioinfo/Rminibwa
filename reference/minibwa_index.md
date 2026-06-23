# Build a minibwa index through the CLI

Build a minibwa index through the CLI

## Usage

``` r
minibwa_index(
  reference,
  prefix = NULL,
  threads = NULL,
  meth = FALSE,
  low_memory = FALSE,
  extra_args = character(),
  path = minibwa_path()
)
```

## Arguments

- reference:

  Path to a FASTA reference.

- prefix:

  Optional output prefix. When `NULL`, minibwa uses `reference` as the
  prefix.

- threads:

  Optional number of worker threads.

- meth:

  If `TRUE`, build a directional bisulfite sequencing index via
  `--meth`.

- low_memory:

  If `TRUE`, pass `-l` to minibwa index.

- extra_args:

  Additional CLI arguments inserted before positional arguments.

- path:

  Executable name or path.

## Value

Invisibly, the expected index prefix.
