is_windows_path <- function(path) {
  .Platform$OS.type == "windows" && grepl("^[A-Za-z]:", path)
}

has_path_separator <- function(path) {
  grepl("/", path, fixed = TRUE) || grepl("\\\\", path)
}

#' Locate the minibwa executable
#'
#' `minibwa_path()` resolves the command-line executable used by the CLI-backed
#' helpers. By default it first consults the `RMINIBWA_MINIBWA` environment
#' variable and otherwise searches for `minibwa` on `PATH`.
#'
#' @param path Executable name or path.
#' @param must_work If `TRUE`, throw an error when the executable cannot be
#'   found.
#'
#' @return A scalar character path, or `NA_character_` when `must_work = FALSE`
#'   and no executable is found.
#' @export
minibwa_path <- function(path = Sys.getenv("RMINIBWA_MINIBWA", unset = "minibwa"),
                         must_work = TRUE) {
  if (!is.character(path) || length(path) != 1L || is.na(path) || !nzchar(path)) {
    stop("`path` must be a non-empty scalar character value.", call. = FALSE)
  }

  resolved <- ""
  if (has_path_separator(path) || is_windows_path(path)) {
    if (file.exists(path)) {
      resolved <- normalizePath(path, winslash = "/", mustWork = FALSE)
    }
  } else {
    resolved <- unname(Sys.which(path))
  }

  if (!nzchar(resolved)) {
    if (isTRUE(must_work)) {
      stop(
        "Could not find the minibwa executable. Install minibwa, add it to PATH, ",
        "or set RMINIBWA_MINIBWA=/path/to/minibwa.",
        call. = FALSE
      )
    }
    return(NA_character_)
  }

  resolved
}

#' Test whether the minibwa CLI is available
#'
#' @inheritParams minibwa_path
#' @return `TRUE` when an executable can be resolved, otherwise `FALSE`.
#' @export
minibwa_available <- function(path = Sys.getenv("RMINIBWA_MINIBWA", unset = "minibwa")) {
  !is.na(minibwa_path(path = path, must_work = FALSE))
}

check_status <- function(result, stderr_file, context) {
  status <- if (is.numeric(result) && length(result) == 1L) {
    as.integer(result)
  } else {
    attr(result, "status")
  }
  if (is.null(status)) status <- 0L
  if (!identical(as.integer(status), 0L)) {
    err <- if (file.exists(stderr_file)) readLines(stderr_file, warn = FALSE) else character()
    msg <- paste(c(sprintf("minibwa %s failed with status %s.", context, status), err), collapse = "\n")
    stop(msg, call. = FALSE)
  }
  invisible(result)
}

run_minibwa <- function(args, path = minibwa_path(), stdout = FALSE, context = "command") {
  stderr_file <- tempfile("rminibwa-stderr-")
  on.exit(unlink(stderr_file), add = TRUE)
  result <- system2(path, args = args, stdout = stdout, stderr = stderr_file)
  check_status(result, stderr_file, context = context)
  result
}

#' Run the minibwa CLI
#'
#' Low-level helper for invoking arbitrary `minibwa` commands from R. Prefer
#' `minibwa_index()` and `minibwa_map()` for common workflows.
#'
#' @param args Character vector of command-line arguments passed to `minibwa`.
#' @param stdout Passed to [system2()]. Use `TRUE` to capture standard output,
#'   `FALSE` to inherit it, or a file path to redirect it.
#' @param path Executable name or path.
#'
#' @return If `stdout = TRUE`, a character vector of captured output; otherwise
#'   an invisible process status.
#' @export
minibwa_cli <- function(args = character(), stdout = TRUE, path = minibwa_path()) {
  if (!is.character(args)) stop("`args` must be a character vector.", call. = FALSE)
  run_minibwa(args = args, path = path, stdout = stdout, context = "CLI command")
}

#' Report the minibwa CLI version
#'
#' @inheritParams minibwa_path
#' @return A scalar character version string.
#' @export
minibwa_version <- function(path = minibwa_path()) {
  out <- run_minibwa(args = "version", path = path, stdout = TRUE, context = "version")
  out <- out[nzchar(out)]
  if (!length(out)) return(NA_character_)
  out[[1L]]
}

validate_threads <- function(threads) {
  if (is.null(threads)) return(character())
  if (!is.numeric(threads) || length(threads) != 1L || is.na(threads) || threads < 1L) {
    stop("`threads` must be a positive scalar number.", call. = FALSE)
  }
  c("-t", as.character(as.integer(threads)))
}

#' Build a minibwa index through the CLI
#'
#' @param reference Path to a FASTA reference.
#' @param prefix Optional output prefix. When `NULL`, minibwa uses `reference` as
#'   the prefix.
#' @param threads Optional number of worker threads.
#' @param meth If `TRUE`, build a directional bisulfite sequencing index via
#'   `--meth`.
#' @param low_memory If `TRUE`, pass `-l` to minibwa index.
#' @param extra_args Additional CLI arguments inserted before positional
#'   arguments.
#' @param path Executable name or path.
#'
#' @return Invisibly, the expected index prefix.
#' @export
minibwa_index <- function(reference,
                          prefix = NULL,
                          threads = NULL,
                          meth = FALSE,
                          low_memory = FALSE,
                          extra_args = character(),
                          path = minibwa_path()) {
  if (!is.character(reference) || length(reference) != 1L || is.na(reference)) {
    stop("`reference` must be a scalar character path.", call. = FALSE)
  }
  reference <- normalizePath(reference, winslash = "/", mustWork = TRUE)
  if (!is.null(prefix) && (!is.character(prefix) || length(prefix) != 1L || is.na(prefix) || !nzchar(prefix))) {
    stop("`prefix` must be `NULL` or a non-empty scalar character path.", call. = FALSE)
  }
  if (!is.character(extra_args)) stop("`extra_args` must be a character vector.", call. = FALSE)

  args <- c(
    "index",
    validate_threads(threads),
    if (isTRUE(low_memory)) "-l",
    if (isTRUE(meth)) "--meth",
    extra_args,
    reference,
    if (!is.null(prefix)) prefix
  )
  run_minibwa(args = args, path = path, stdout = FALSE, context = "index")
  invisible(if (is.null(prefix)) reference else prefix)
}

#' Map reads through the minibwa CLI
#'
#' @param index Index prefix produced by `minibwa_index()` or the minibwa CLI.
#' @param reads One or more FASTA/FASTQ input paths. The common cases are one
#'   single-end file or two paired-end files.
#' @param output Optional output file. When `NULL`, standard output is captured
#'   and returned as a character vector.
#' @param format Output format: `"sam"` by default, or `"paf"` to pass `-f`.
#' @param threads Optional number of worker threads.
#' @param meth If `TRUE`, pass `--meth`.
#' @param hic If `TRUE`, pass `--hic`.
#' @param extra_args Additional CLI arguments inserted before positional
#'   arguments.
#' @param path Executable name or path.
#'
#' @return If `output = NULL`, captured alignment records as a character vector;
#'   otherwise the output path, invisibly.
#' @export
minibwa_map <- function(index,
                        reads,
                        output = NULL,
                        format = c("sam", "paf"),
                        threads = NULL,
                        meth = FALSE,
                        hic = FALSE,
                        extra_args = character(),
                        path = minibwa_path()) {
  format <- match.arg(format)
  if (!is.character(index) || length(index) != 1L || is.na(index) || !nzchar(index)) {
    stop("`index` must be a non-empty scalar character value.", call. = FALSE)
  }
  if (!is.character(reads) || !length(reads) || anyNA(reads)) {
    stop("`reads` must be a non-empty character vector of file paths.", call. = FALSE)
  }
  reads <- normalizePath(reads, winslash = "/", mustWork = TRUE)
  if (!is.null(output) && (!is.character(output) || length(output) != 1L || is.na(output) || !nzchar(output))) {
    stop("`output` must be `NULL` or a non-empty scalar character path.", call. = FALSE)
  }
  if (!is.character(extra_args)) stop("`extra_args` must be a character vector.", call. = FALSE)

  args <- c(
    "map",
    if (identical(format, "paf")) "-f",
    validate_threads(threads),
    if (isTRUE(meth)) "--meth",
    if (isTRUE(hic)) "--hic",
    if (!is.null(output)) c("-o", output),
    extra_args,
    index,
    reads
  )

  if (is.null(output)) {
    run_minibwa(args = args, path = path, stdout = TRUE, context = "map")
  } else {
    run_minibwa(args = args, path = path, stdout = FALSE, context = "map")
    invisible(output)
  }
}
