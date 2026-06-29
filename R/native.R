#' @useDynLib Rminibwa, .registration = TRUE
NULL

#' Build a minibwa index with the native library
#'
#' Builds index files at `prefix` using the vendored minibwa C library. The
#' package is GPL, so the native build includes minibwa's GPL-compatible
#' low-memory indexing path; `low_memory = FALSE` uses the default `libsais`
#' path.
#'
#' @param fasta Reference FASTA path. Gzip-compressed input is supported by
#'   minibwa/zlib.
#' @param prefix Output prefix for `.l2b`, `.mbw`, and optionally `.meth.mbw`.
#' @param meth Build the directional methylation index as well.
#' @param threads Number of index-build threads requested by minibwa.
#' @param low_memory Use minibwa's low-memory BWT builder.
#' @return `prefix`, invisibly.
#' @export
mb_index_build <- function(fasta, prefix, meth = FALSE, threads = 1L, low_memory = FALSE) {
  fasta <- rmb_path_scalar(fasta, "fasta", must_work = TRUE)
  prefix <- rmb_path_scalar(prefix, "prefix", must_work = FALSE)
  parent <- dirname(prefix)
  if (!dir.exists(parent)) {
    stop("output directory does not exist: ", parent, call. = FALSE)
  }
  meth <- rmb_logical_scalar(meth, "meth")
  low_memory <- rmb_logical_scalar(low_memory, "low_memory")
  threads <- rmb_positive_int_scalar(threads, "threads")
  invisible(.Call(RC_mb_index_build, fasta, prefix, meth, threads, low_memory))
  invisible(prefix)
}

#' Load a native minibwa index
#'
#' @param prefix Index prefix.
#' @param meth Load the methylation BWT index.
#' @return An external pointer handle to a native minibwa index.
#' @export
mb_index_load <- function(prefix, meth = FALSE) {
  prefix <- rmb_path_scalar(prefix, "prefix", must_work = FALSE)
  meth <- rmb_logical_scalar(meth, "meth")
  .Call(RC_mb_index_load, prefix, meth)
}

#' Return minibwa index contigs
#'
#' Names are returned as raw byte vectors to avoid forced string materialization.
#'
#' @param index Index handle returned by `mb_index_load()`.
#' @return A list with raw-vector `name` entries and numeric `length` values.
#' @export
mb_index_contigs <- function(index) {
  .Call(RC_mb_index_contigs, index)
}

#' Native minibwa options
#'
#' Returns a plain list consumed by native mapping functions. Only the selected
#' options are exposed initially; all other fields use minibwa defaults for the
#' chosen preset.
#'
#' @param preset One of `"adap"`, `"sr"`, or `"lr"`.
#' @param paired,methylation Optional logical overrides for minibwa flags.
#' @param out_n,min_seed_len,threads,match_score,mismatch_penalty,gap_open,gap_extend
#'   Optional integer overrides for common minibwa fields.
#' @return A plain list.
#' @export
mb_opts <- function(preset = c("adap", "sr", "lr"),
                    paired = NULL,
                    methylation = NULL,
                    out_n = NULL,
                    min_seed_len = NULL,
                    threads = NULL,
                    match_score = NULL,
                    mismatch_penalty = NULL,
                    gap_open = NULL,
                    gap_extend = NULL) {
  preset <- match.arg(preset)
  list(
    preset = preset,
    paired = rmb_nullable_logical_scalar(paired, "paired"),
    methylation = rmb_nullable_logical_scalar(methylation, "methylation"),
    out_n = rmb_nullable_int_scalar(out_n, "out_n"),
    min_seed_len = rmb_nullable_int_scalar(min_seed_len, "min_seed_len"),
    threads = rmb_nullable_positive_int_scalar(threads, "threads"),
    match_score = rmb_nullable_int_scalar(match_score, "match_score"),
    mismatch_penalty = rmb_nullable_int_scalar(mismatch_penalty, "mismatch_penalty"),
    gap_open = rmb_nullable_int_scalar(gap_open, "gap_open"),
    gap_extend = rmb_nullable_int_scalar(gap_extend, "gap_extend")
  )
}

#' Map raw sequence bytes with native minibwa
#'
#' `mb_map()` is pipe-friendly: the sequence bytes are the first argument.
#' The result is a native alignment batch external pointer. Use `mb_align_col()`
#' for ALTREP column views or pass the batch to downstream C consumers.
#'
#' @param x Raw vector of query sequence bytes.
#' @param index Index handle returned by `mb_index_load()`.
#' @param opt Options list from `mb_opts()`.
#' @param name Optional read name as raw bytes. Character names are accepted for
#'   convenience but raw bytes are preferred.
#' @param meth Methylation code: `"none"`, `"c2t"`, or `"g2a"`.
#' @return A native alignment batch external pointer.
#' @examples
#' # See vignettes for full index-backed examples.
#' @export
mb_map <- function(x, index, opt = mb_opts(), name = NULL, meth = c("none", "c2t", "g2a")) {
  if (!is.raw(x)) stop("x must be a raw vector of sequence bytes", call. = FALSE)
  meth <- match.arg(meth)
  if (!is.null(name) && !is.raw(name) && !(is.character(name) && length(name) == 1L && !is.na(name))) {
    stop("name must be NULL, raw bytes, or a non-missing character scalar", call. = FALSE)
  }
  .Call(RC_mb_map_raw, x, index, opt, name, meth)
}

# Internal count-only mapper used to keep developer benchmarks honest.
mb_map_count <- function(x, index, opt = mb_opts(), name = NULL, meth = c("none", "c2t", "g2a")) {
  if (!is.raw(x)) stop("x must be a raw vector of sequence bytes", call. = FALSE)
  meth <- match.arg(meth)
  if (!is.null(name) && !is.raw(name) && !(is.character(name) && length(name) == 1L && !is.na(name))) {
    stop("name must be NULL, raw bytes, or a non-missing character scalar", call. = FALSE)
  }
  .Call(RC_mb_map_count_raw, x, index, opt, name, meth)
}

#' Inspect native minibwa alignment batches
#'
#' `mb_align_col()` returns ALTREP views over native alignment columns. These
#' views do not copy columns unless R forces materialization. C consumers should
#' prefer the installed `Rminibwa.h` API.
#'
#' @param x Native alignment batch returned by `mb_map()`.
#' @param name Column name.
#' @return `mb_align_n()` returns the number of records. `mb_align_col()` returns
#'   an ALTREP integer or real vector. `mb_align_cigar_words()` returns a raw
#'   debug copy of packed minibwa CIGAR words.
#' @export
mb_align_n <- function(x) {
  .Call(RC_mb_align_n, x)
}

#' @rdname mb_align_n
#' @export
mb_align_col <- function(x, name) {
  if (!is.character(name) || length(name) != 1L || is.na(name)) {
    stop("name must be a non-missing character scalar", call. = FALSE)
  }
  .Call(RC_mb_align_col, x, name)
}

#' @rdname mb_align_n
#' @export
mb_align_cigar_words <- function(x) {
  .Call(RC_mb_align_cigar_words, x)
}

#' Select the runtime SIMD backend
#'
#' Rminibwa compiles staged KSW backend objects and switches among them at
#' runtime. Use `"auto"` for the best available backend, or force one of
#' `"scalar"`, `"sse4"`, or `"avx2"` for diagnostics and benchmarks. The
#' `"scalar"` backend is the portable SIMDe baseline with native intrinsics
#' disabled; compilers may still lower portable vector types efficiently.
#'
#' @param backend Character scalar backend name.
#' @return The selected backend, invisibly.
#' @export
simd_set_backend <- function(backend = "auto") {
  if (!is.character(backend) || length(backend) != 1L || is.na(backend) || !nzchar(backend)) {
    stop("backend must be a non-empty character scalar", call. = FALSE)
  }
  invisible(.Call(RC_simd_set_backend, backend))
}

#' Report the selected SIMD backend
#'
#' @return A character scalar.
#' @export
simd_backend <- function() {
  .Call(RC_simd_backend)
}

#' Report SIMD dispatch diagnostics
#'
#' @return An object of class `rminibwa_simd_info`: a named list with dispatch
#'   mode, requested backend, selected backend, compiled backends,
#'   CPU-supported backends, available backends, and target.
#' @export
simd_info <- function() {
  structure(.Call(RC_simd_info), class = "rminibwa_simd_info")
}

#' @export
print.rminibwa_simd_info <- function(x, ...) {
  collapse <- function(value) {
    if (!length(value)) return("<none>")
    paste(value, collapse = ", ")
  }
  cat("<rminibwa SIMD dispatch>\n")
  cat("  selected:  ", x$selected_backend, "\n", sep = "")
  cat("  requested: ", x$requested_backend, "\n", sep = "")
  cat("  mode:      ", x$dispatch_mode, "\n", sep = "")
  cat("  compiled:  ", collapse(x$compiled_backends), "\n", sep = "")
  cat("  available: ", collapse(x$available_backends), "\n", sep = "")
  cat("  cpu:       ", collapse(x$cpu_backends), "\n", sep = "")
  cat("  target:    ", x$target, "\n", sep = "")
  invisible(x)
}

# Internal diagnostic used by developer benchmarks.
simd_counters <- function(reset = FALSE) {
  reset <- rmb_logical_scalar(reset, "reset")
  .Call(RC_simd_counters, reset)
}

rmb_path_scalar <- function(x, what, must_work) {
  if (!is.character(x) || length(x) != 1L || is.na(x) || !nzchar(x)) {
    stop(what, " must be a non-empty character scalar", call. = FALSE)
  }
  normalizePath(x, winslash = "/", mustWork = must_work)
}

rmb_logical_scalar <- function(x, what) {
  if (!is.logical(x) || length(x) != 1L || is.na(x)) {
    stop(what, " must be TRUE or FALSE", call. = FALSE)
  }
  x
}

rmb_nullable_logical_scalar <- function(x, what) {
  if (is.null(x)) return(NULL)
  rmb_logical_scalar(x, what)
}

rmb_positive_int_scalar <- function(x, what) {
  x <- rmb_nullable_positive_int_scalar(x, what)
  if (is.null(x)) stop(what, " must be supplied", call. = FALSE)
  x
}

rmb_nullable_positive_int_scalar <- function(x, what) {
  if (is.null(x)) return(NULL)
  x <- rmb_nullable_int_scalar(x, what)
  if (x < 1L) stop(what, " must be positive", call. = FALSE)
  x
}

rmb_nullable_int_scalar <- function(x, what) {
  if (is.null(x)) return(NULL)
  if (!is.numeric(x) || length(x) != 1L || is.na(x)) {
    stop(what, " must be an integer scalar", call. = FALSE)
  }
  as.integer(x)
}
