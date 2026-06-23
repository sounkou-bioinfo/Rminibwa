# Copy the package shared library using the standard R extension variables.
files <- Sys.glob(paste0("*", SHLIB_EXT))
dest_lib <- file.path(R_PACKAGE_DIR, paste0("libs", R_ARCH))
dir.create(dest_lib, recursive = TRUE, showWarnings = FALSE)
file.copy(files, dest_lib, overwrite = TRUE)

if (file.exists("symbols.rds")) {
  file.copy("symbols.rds", dest_lib, overwrite = TRUE)
}

# Also install the package-built minibwa executable for CLI helpers.
exe <- if (.Platform$OS.type == "windows") "minibwa.exe" else "minibwa"
src <- file.path("rmb-cli", exe)
if (!file.exists(src)) {
  stop("built minibwa executable not found: ", src, call. = FALSE)
}

dest_bin <- file.path(R_PACKAGE_DIR, "bin")
dir.create(dest_bin, recursive = TRUE, showWarnings = FALSE)
dest <- file.path(dest_bin, exe)
if (!file.copy(src, dest, overwrite = TRUE)) {
  stop("failed to install minibwa executable to ", dest, call. = FALSE)
}
Sys.chmod(dest, mode = "0755")
