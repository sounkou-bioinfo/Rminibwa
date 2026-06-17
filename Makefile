.PHONY: document test build check clean

document:
	Rscript -e 'roxygen2::roxygenize(load_code = "source")'

test:
	Rscript -e 'tinytest::test_package("Rminibwa")'

build:
	R CMD build .

check: build
	R CMD check Rminibwa_*.tar.gz

clean:
	rm -rf Rminibwa_*.tar.gz Rminibwa.Rcheck .Rcheck
