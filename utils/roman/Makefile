#!/bin/make

# Drives the full AST vs GWCS WCS comparison pipeline for the RST calibration file datasets.
#
# Assumes AST installed under `AST_PREFIX` (defaults to `$(HOME)/.local)
#
#   make           # run full pipeline (prepare -> ast -> gwcs -> compare)
#   make prepare   # convert RST ASDF files -> inline ASDF files
#   make ast       # run AST step only
#   make gwcs      # run GWCS step only
#   make compare   # run comparison step only (requires ast + gwcs CSVs)
#   make clean     # remove generated CSV/txt files
#   make distclean # also remove inline files and the compiled roman_wcs_ast binary

REPO_ROOT       := .
REPO_ABS        := $(abspath $(REPO_ROOT))
AST_BIN         := $(REPO_ROOT)/roman_wcs_ast
AST_SRC         := $(REPO_ROOT)/roman_wcs_ast.c
PREPARE_SCRIPT  := $(REPO_ROOT)/prepare_roman_yaml.py
GWCS_SCRIPT     := $(REPO_ROOT)/roman_wcs_gwcs.py
CMP_SCRIPT      := $(REPO_ROOT)/compare_wcs.py

DATA_DIR    ?= $(REPO_ROOT)/.data
PATTERN     ?= *.asdf
INLINE_DIR  := $(DATA_DIR)/inline

ASDF_FILES := $(wildcard $(DATA_DIR)/$(PATTERN))
INLINE_FILES  := $(patsubst $(DATA_DIR)/%,$(INLINE_DIR)/%,$(ASDF_FILES))

AST_CSV  := $(DATA_DIR)/ast_wcs_results.csv
GWCS_CSV := $(DATA_DIR)/gwcs_wcs_results.csv
SUMMARY  := $(DATA_DIR)/wcs_comparison_summary.txt

# AST installation
# Override AST_PREFIX to point at a non-default installation, e.g.:
#   make AST_PREFIX=/opt/star
AST_PREFIX  ?= $(HOME)/.local
AST_LINK    := $(AST_PREFIX)/bin/ast_link

# Python virtualenv
# Requires Python 3.11+.  Looks for python3.11/3.12/3.13 before falling back
# to a version check on whatever python3 is in PATH.
VENV        := $(REPO_ROOT)/.venv
PYTHON      := $(VENV)/bin/python3

PYTHON3 := $(shell \
    for py in python3.13 python3.12 python3.11; do \
        command -v $$py >/dev/null 2>&1 && echo $$py && break; \
    done)

ifeq ($(PYTHON3),)
PYTHON3 := $(shell \
    py=python3; \
    $$py -c "import sys; sys.exit(0 if sys.version_info >= (3,11) else 1)" \
        2>/dev/null && echo $$py)
endif

.PHONY: all clean distclean venv prepare ast gwcs compare build-ast

all: $(SUMMARY)

$(PYTHON):
	@if [ -z "$(PYTHON3)" ]; then \
	    echo "ERROR: Python 3.11 or newer is required."; \
	    echo "       Install python3.11, python3.12, or python3.13 and retry."; \
	    exit 1; \
	fi
	$(PYTHON3) -m venv $(VENV)
	$(VENV)/bin/pip install --quiet --upgrade pip
	$(VENV)/bin/pip install --quiet \
	    asdf gwcs asdf-astropy asdf-wcs-schemas roman-datamodels

venv: $(PYTHON)

# Build the roman_wcs_ast binary in the repo root if not already present
$(AST_BIN): $(AST_SRC)
	gcc -I$(AST_PREFIX)/include $(AST_SRC) \
	    -L$(AST_PREFIX)/lib $$($(AST_LINK)) -o $@

build-ast: $(AST_BIN)

# Convert the binary-block ASDF files to inline format.
# Use a stamp file since the script generates one output per input in a batch.
$(INLINE_DIR)/.stamp: $(ASDF_FILES) | $(PYTHON)
	$(PYTHON) $(PREPARE_SCRIPT) -o $(INLINE_DIR) $^
	touch $@

prepare: $(INLINE_DIR)/.stamp

$(AST_CSV): $(AST_BIN) $(INLINE_DIR)/.stamp
	LD_LIBRARY_PATH=$(AST_PREFIX)/lib $(AST_BIN) -o $@ $(INLINE_FILES)

$(GWCS_CSV): $(ASDF_FILES) | $(PYTHON)
	$(PYTHON) $(GWCS_SCRIPT) -o $@ $(ASDF_FILES)

$(SUMMARY): $(AST_CSV) $(GWCS_CSV) | $(PYTHON)
	$(PYTHON) $(CMP_SCRIPT) -o $@ $(AST_CSV) $(GWCS_CSV)

ast:     $(AST_CSV)
gwcs:    $(GWCS_CSV)
compare: $(SUMMARY)

clean:
	rm -f $(AST_CSV) $(GWCS_CSV) $(SUMMARY)

distclean: clean
	rm -f $(INLINE_DIR)/.stamp $(INLINE_FILES) $(AST_BIN)
