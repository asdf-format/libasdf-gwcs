# Roman WCS AST Conformance Tests

Tools for evaluating Roman Space Telescope WCS transforms using
[Starlink AST](https://github.com/Starlink/ast)'s ASDF/YAML support and
comparing the results against the reference Python
[gwcs](https://gwcs.readthedocs.io/) implementation.

## What it does

The pipeline runs in three steps:

1. **`prepare_roman_yaml.py`** -- Converts the original Roman calibration ASDF
   files (which use binary array blocks, which AST cannot read) into new files
   containing just the WCS, with all arrays stored inline.

2. **`roman_wcs_ast.c`** -- C program that opens each inline ASDF file via
   AST's `YamlChan`, extracts the WCS `FrameSet`, evaluates a 20×20 pixel
   grid across the full detector, and writes the resulting sky coordinates
   (RA/Dec in degrees) to a CSV file.

3. **`roman_wcs_gwcs.py`** -- Python script that opens the original ASDF files
   via the gwcs library and evaluates the same pixel grid, writing an
   equivalent CSV.

4. **`compare_wcs.py`** — Loads both CSVs and computes per-detector and
   overall angular-separation statistics between the two implementations.

For convenience a `Makefile` is provided which orchestrates the analysis.

## Prerequisites

**AST** (version with YAML/ASDF support, built against libyaml); e.g.
```sh
$ ./configure --prefix=$HOME/.local --without-fortran
$ make -j$(nproc) install
```

**Python 3.11+** (required for the latest asdf-astropy). The Makefile will
create and populate a virtualenv automatically.

**Roman calibration files** by default in a `.data` directory but you can
override the data path by passing `DATA_DIR=...` to `make`.

## Usage

### Full pipeline (prepare inline files, run AST, run gwcs, compare):
```sh
$ make
```

None of these tools require the makefile to use either; it is provided
just for convenience.  You can read the individual tools' docstrings for
usage instructions.

### Individual steps:
make prepare   # .data/inline/ files from original Roman calibration files
make ast       # run roman_wcs_ast  -> ast_wcs_results.csv
make gwcs      # run roman_wcs_gwcs -> gwcs_wcs_results.csv
make compare   # run compare_wcs    -> wcs_comparison_summary.txt

### Override AST installation prefix (default: $HOME/.local):
```sh
$ make AST_PREFIX=/opt/star
```

## TODO

Maybe provide a conda environment for this.
