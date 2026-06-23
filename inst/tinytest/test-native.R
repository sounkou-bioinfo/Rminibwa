local({
  tmp <- tempfile("rminibwa-native-")
  dir.create(tmp)
  on.exit(unlink(tmp, recursive = TRUE), add = TRUE)

  ref <- paste(rep("ACGT", 1000), collapse = "")
  fa <- file.path(tmp, "ref.fa")
  writeLines(c(">chr1", ref), fa, useBytes = TRUE)
  prefix <- file.path(tmp, "idx")

  expect_silent(mb_index_build(fa, prefix, threads = 1L))
  idx <- mb_index_load(prefix)
  contigs <- mb_index_contigs(idx)
  expect_equal(rawToChar(contigs$name[[1]]), "chr1")
  expect_equal(contigs$length[[1]], 4000)

  info <- simd_info()
  expect_true("scalar" %in% info$compiled_backends)
  expect_true(all(info$compiled_backends %in% c("scalar", "sse4", "avx2")))
  expect_true(all(info$available_backends %in% info$compiled_backends))
  query <- charToRaw(substr(ref, 1L, 100L))
  for (backend in intersect(c("scalar", "sse4", "avx2"), info$available_backends)) {
    simd_set_backend(backend)
    aln <- query |>
      mb_map(idx, opt = mb_opts("sr", out_n = 0L), name = charToRaw("r1"))

    expect_true(inherits(aln, "rminibwa_align"))
    expect_true(mb_align_n(aln) > 0)
    expect_equal(mb_align_col(aln, "tid")[[1]], 0L)
    expect_true(mb_align_col(aln, "qe")[[1]] > 0L)
    expect_true(mb_align_col(aln, "te")[[1]] > mb_align_col(aln, "ts")[[1]])
    expect_true(length(mb_align_cigar_words(aln)) > 0L)
  }
  set.seed(42)
  ref2 <- paste(sample(c("A", "C", "G", "T"), 20000, replace = TRUE), collapse = "")
  fa2 <- file.path(tmp, "ref2.fa")
  writeLines(c(">chr1", ref2), fa2, useBytes = TRUE)
  prefix2 <- file.path(tmp, "idx2")
  expect_silent(mb_index_build(fa2, prefix2, threads = 1L))
  idx2 <- mb_index_load(prefix2)
  q2 <- strsplit(substr(ref2, 1000L, 1499L), "", fixed = TRUE)[[1L]]
  q2 <- paste(q2[-seq(25L, length(q2), by = 25L)], collapse = "")
  q2_raw <- charToRaw(q2)
  nm2_raw <- charToRaw("ksw")
  simd_set_backend("auto")
  invisible(Rminibwa:::simd_counters(TRUE))
  n_count <- Rminibwa:::mb_map_count(q2_raw, idx2, mb_opts("sr", out_n = 0L), nm2_raw)
  cnt <- Rminibwa:::simd_counters(FALSE)
  aln2 <- mb_map(q2_raw, idx2, mb_opts("sr", out_n = 0L), nm2_raw)
  expect_equal(n_count, as.integer(mb_align_n(aln2)))
  expect_true(cnt[["extd2"]] > 0)

  simd_set_backend("auto")
})
