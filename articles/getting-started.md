# Getting Started

Rminibwa starts as a small R interface to the `minibwa` executable. The
package can be installed and checked without `minibwa`, but mapping
functions need an executable at runtime.

``` r

library(Rminibwa)
minibwa_available()
```

Set a custom executable with an environment variable:

``` r

Sys.setenv(RMINIBWA_MINIBWA = "/path/to/minibwa")
minibwa_version()
```

Index a reference and map reads:

``` r

prefix <- minibwa_index("ref.fa", threads = 8)
aln <- minibwa_map(prefix, "reads.fq.gz", format = "paf", threads = 8)
```

Write SAM or PAF output directly to a file by setting `output`:

``` r

minibwa_map(prefix, c("read1.fq.gz", "read2.fq.gz"), output = "aln.sam")
```

The CLI layer is intentionally thin. It should stay close to upstream
behavior and serve as the validation oracle for future native bindings.
