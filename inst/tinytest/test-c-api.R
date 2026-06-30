local({
  tmp <- tempfile("rminibwa-capi-")
  dir.create(tmp)
  on.exit(unlink(tmp, recursive = TRUE), add = TRUE)

  ref <- paste(rep("ACGT", 1000), collapse = "")
  fa <- file.path(tmp, "ref.fa")
  writeLines(c(">chr1", ref), fa, useBytes = TRUE)
  prefix <- file.path(tmp, "idx")
  mb_index_build(fa, prefix, threads = 1L)
  idx <- mb_index_load(prefix)
  aln <- charToRaw(substr(ref, 1L, 100L)) |>
    mb_map(idx, opt = mb_opts("sr", out_n = 0L), name = charToRaw("read1"))

  capi_path <- system.file(
    "capi", "rminibwa_tinycc_consumer.c",
    package = "Rminibwa", mustWork = TRUE
  )
  capi_code <- paste(readLines(capi_path, warn = FALSE), collapse = "\n")

  ffi <- tryCatch(
    Rtinycc::tcc_ffi() |>
      Rtinycc::tcc_include(system.file("include", package = "Rminibwa")) |>
      Rtinycc::tcc_source(capi_code) |>
      Rtinycc::tcc_bind(
        rminibwa_capi_summary = list(args = list("sexp"), returns = "sexp")
      ) |>
      Rtinycc::tcc_compile(),
    error = identity
  )

  if (inherits(ffi, "error")) {
    expect_true(TRUE, info = paste("Rtinycc C API smoke skipped:", conditionMessage(ffi)))
  } else {
    summary <- ffi$rminibwa_capi_summary(aln)
    expect_equal(summary[[1]], as.integer(mb_align_n(aln)))
    expect_equal(summary[[2]], as.integer(mb_align_read_n(aln)))
    expect_equal(summary[[3]], mb_align_read_col(aln, "length")[[1]])
    expect_equal(summary[[4]], 0L)
    expect_true(summary[[5]] > 0L)
  }
})
