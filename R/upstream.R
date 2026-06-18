#' Report the pinned upstream minibwa source
#'
#' Rminibwa pins a specific upstream `minibwa` commit for native-library work.
#' `minibwa_upstream_info()` reports the installed copy of that provenance
#' metadata.
#'
#' @return A named list with fields such as `Component`, `Version`,
#'   `Repository`, `Commit`, and `PatchDirectory`.
#' @examples
#' minibwa_upstream_info()[c("Version", "Commit")]
#' @export
minibwa_upstream_info <- function() {
  path <- system.file("upstream", "minibwa.dcf", package = "Rminibwa", mustWork = TRUE)
  x <- read.dcf(path, all = TRUE)
  as.list(x[1L, , drop = TRUE])
}
