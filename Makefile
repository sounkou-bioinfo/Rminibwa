# h/t to @jimhester and @yihui for this parse block:
# https://github.com/yihui/knitr/blob/dc5ead7bcfc0ebd2789fe99c527c7d91afb3de4a/Makefile#L1-L4
# Note the portability change as suggested in the manual:
# https://cran.r-project.org/doc/manuals/r-release/R-exts.html#Writing-portable-packages
PKGNAME := $(shell sed -n 's/Package: *\([^ ]*\)/\1/p' DESCRIPTION)
PKGVERS := $(shell sed -n 's/Version: *\([^ ]*\)/\1/p' DESCRIPTION)
MINIBWA_BINDINGS_ROOT ?= /tmp/minibwa-bindings-recon
RUSTFLAGS_AVX2 ?= -C target-cpu=native -C target-feature=+avx2
MINIBWA_NATIVE_CFLAGS ?= -DHAVE_KALLOC
RMINIBWA_BENCH_PYTHONPATH ?= $(CURDIR)/bench/python
RMINIBWA_BENCH_RUST_LIB ?= $(CURDIR)/bench/rust-shim/target/release/librminibwa_bench_rust.so
RETICULATE_PYTHON ?= $(shell command -v python3)

all: check

rd:
	R -e 'roxygen2::roxygenize(load_code = "source")'

readme:
	R -e 'rmarkdown::render("README.Rmd", output_format = rmarkdown::github_document(), output_file = "README.md")'

vig:
	R -e "tools::buildVignettes(dir = '.')"

vig-md:
	R -e "for (f in Sys.glob('vignettes/*.Rmd')) { out <- sub('\\\\.Rmd$$', '.md', f); rmarkdown::render(f, output_format = rmarkdown::md_document(variant = 'gfm'), output_file = basename(out), output_dir = dirname(out), quiet = FALSE, envir = new.env(parent = globalenv())) }"

pkgdown:
	R -e 'pkgdown::build_site()'

vendor:
	Rscript tools/vendor-minibwa.R refresh

vendor-status:
	Rscript tools/vendor-minibwa.R status

sync-upstream:
	@mkdir -p inst/upstream
	@cp tools/minibwa-upstream.dcf inst/upstream/minibwa.dcf
	@echo 'Upstream metadata synced.'

check-upstream-sync:
	@diff -q tools/minibwa-upstream.dcf inst/upstream/minibwa.dcf >/dev/null || \
		(echo 'ERROR: upstream metadata drift detected' && exit 1)

minibwa-cli:
	tools/build-minibwa-cli.sh

build:
	R CMD build .

check: build
	R CMD check --as-cran --no-manual $(PKGNAME)_$(PKGVERS).tar.gz

install_deps:
	R \
	-e 'options(repos = c(sounkou = "https://sounkou-bioinfo.r-universe.dev", CRAN = "https://cloud.r-project.org"))' \
	-e 'if (!requireNamespace("remotes", quietly = TRUE)) install.packages("remotes")' \
	-e 'remotes::install_deps(dependencies = TRUE)'

install: build
	R CMD INSTALL $(PKGNAME)_$(PKGVERS).tar.gz

install2:
	R CMD INSTALL --no-configure .

install3:
	R CMD INSTALL .

clean:
	@rm -rf $(PKGNAME)_$(PKGVERS).tar.gz $(PKGNAME)_$(PKGVERS).tgz $(PKGNAME).Rcheck .Rcheck src/vendor/_archives config.log src/*.o src/*.so src/*.dll src/*.dylib

# Development targets
dev-install:
	R CMD INSTALL --preclean .

test1:
	R -e "tinytest::test_package('$(PKGNAME)', testdir = 'inst/tinytest', ncpu = 1L)"

test2:
	R -e "tinytest::test_package('$(PKGNAME)', testdir = 'inst/tinytest', ncpu = 2L)"

test0:
	R -e "tinytest::test_package('$(PKGNAME)', testdir = 'inst/tinytest')"

test: install
	R -e "tinytest::test_package('$(PKGNAME)', testdir = 'inst/tinytest')"

bench-python:
	rm -rf $(RMINIBWA_BENCH_PYTHONPATH)
	mkdir -p $(RMINIBWA_BENCH_PYTHONPATH)
	cd $(MINIBWA_BINDINGS_ROOT)/minibwa-py && \
		CFLAGS='$(MINIBWA_NATIVE_CFLAGS)' RUSTFLAGS='$(RUSTFLAGS_AVX2)' python3 -m pip install --root-user-action=ignore --no-deps --force-reinstall --target $(RMINIBWA_BENCH_PYTHONPATH) .

bench-rust:
	cd bench/rust-shim && CFLAGS='$(MINIBWA_NATIVE_CFLAGS)' RUSTFLAGS='$(RUSTFLAGS_AVX2)' cargo build --release

test-bench-env:
	test -d $(RMINIBWA_BENCH_PYTHONPATH)
	test -f $(RMINIBWA_BENCH_RUST_LIB)

rdm: dev-install bench-python bench-rust test-bench-env
	RMINIBWA_RUN_BENCHMARKS=true \
	RETICULATE_PYTHON=$(RETICULATE_PYTHON) \
	RMINIBWA_BENCH_PYTHONPATH=$(RMINIBWA_BENCH_PYTHONPATH) \
	RMINIBWA_BENCH_RUST_LIB=$(RMINIBWA_BENCH_RUST_LIB) \
	R -e 'rmarkdown::render("README.Rmd", output_format = rmarkdown::github_document(), output_file = "README.md")'

.PHONY: all rd readme vig vig-md pkgdown vendor vendor-status sync-upstream check-upstream-sync minibwa-cli build check install_deps install install2 install3 clean dev-install test1 test2 test0 test bench-python bench-rust test-bench-env rdm
