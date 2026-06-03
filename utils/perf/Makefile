#!/bin/make

# Performance benchmark: libasdf-gwcs vs Python GWCS
#
# Measures cold/hot parse times and eval throughput across a sweep of
# input sizes (N = 1 to 16M points) on Roman ASDF calibration files.
#
# Prerequisites
# -------------
#   - libasdf-gwcs (and libasdf) installed and visible via pkg-config
#   - Python 3.11+
#   - Roman ASDF files in DATA_DIR (defaults to ../roman/.data)
#
# Usage
# -----
#   make                          # build, run both benchmarks, generate plots
#   make build-c                  # compile roman_wcs_perf only
#   make venv                     # create Python venv with requirements
#   make run-c                    # run C benchmark  -> C_CSV
#   make run-python               # run Python benchmark -> PYTHON_CSV
#   make plot                     # merge CSVs and render plots into PLOT_DIR
#   make clean                    # remove CSVs and plots
#   make distclean                # also remove venv and compiled binary
#
# Overrides
# ---------
#   make PREFIX=/opt/local          # where libasdf-gwcs is installed
#   make DATA_DIR=/path/to/asdf     # directory containing *.asdf files
#   make PKG_CONFIG_PATH=...        # supplement the default search path
#   make TASKSET_CORE=0             # pin both benchmarks to a single CPU core

REPO_ROOT := $(abspath ../..)

# Installation prefix for libasdf-gwcs (and libasdf if co-installed there).
PREFIX ?= $(HOME)/.local

_PC_PATH := $(PREFIX)/lib/pkgconfig:$(PKG_CONFIG_PATH)
export PKG_CONFIG_PATH := $(_PC_PATH)

_PC := PKG_CONFIG_PATH="$(_PC_PATH)" pkg-config

GWCS_CFLAGS := $(shell $(_PC) --cflags libasdf libasdf-gwcs 2>/dev/null)
GWCS_LIBS   := $(shell $(_PC) --libs libasdf libasdf-gwcs 2>/dev/null)
GWCS_LIBDIR := $(shell $(_PC) --variable=libdir libasdf-gwcs 2>/dev/null)

CC      ?= gcc
CFLAGS  := -O2 -std=c11 -Wall $(GWCS_CFLAGS)
LDFLAGS := $(GWCS_LIBS)
ifneq ($(GWCS_LIBDIR),)
LDFLAGS += -Wl,-rpath,$(GWCS_LIBDIR)
endif

# Optional core pinning: make TASKSET_CORE=0 pins both benchmarks to core 0.
# Reduces scheduler-migration noise and ensures comparable L1/L2 cache state.
TASKSET_CORE ?=
_TASKSET     := $(if $(TASKSET_CORE),taskset -c $(TASKSET_CORE),)

PERF_BIN    := roman_wcs_perf
PERF_SRC    := roman_wcs_perf.c
PROFILE_BIN := wcs_profile
PROFILE_SRC := wcs_profile.c

DATA_DIR   ?= $(REPO_ROOT)/utils/roman/.data
ASDF_FILES := $(wildcard $(DATA_DIR)/*.asdf)

RESULTS_DIR := $(CURDIR)/results
C_CSV       := $(RESULTS_DIR)/c_perf_results.csv
PYTHON_CSV  := $(RESULTS_DIR)/python_perf_results.csv
PLOT_DIR    := $(RESULTS_DIR)/plots

VENV    := .venv
PYTHON  := $(VENV)/bin/python3

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

.PHONY: all build-c build-profile venv run-c run-python plot clean distclean

all: build-c venv run-c run-python plot

$(PYTHON):
	@if [ -z "$(PYTHON3)" ]; then \
	    echo "error: Python 3.11 or newer is required."; \
	    exit 1; \
	fi
	$(PYTHON3) -m venv $(VENV)
	$(VENV)/bin/pip install --quiet --upgrade pip
	$(VENV)/bin/pip install --quiet \
	    asdf gwcs asdf-astropy asdf-wcs-schemas roman-datamodels \
	    matplotlib threadpoolctl

venv: $(PYTHON)

$(PERF_BIN): $(PERF_SRC)
	@if [ -z "$(GWCS_CFLAGS)" ]; then \
	    echo "error: pkg-config cannot find libasdf or libasdf-gwcs."; \
	    echo "  Install both to PREFIX=$(PREFIX) or set PKG_CONFIG_PATH."; \
	    exit 1; \
	fi
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

build-c: $(PERF_BIN)

$(PROFILE_BIN): $(PROFILE_SRC)
	@if [ -z "$(GWCS_CFLAGS)" ]; then \
	    echo "error: pkg-config cannot find libasdf or libasdf-gwcs."; \
	    echo "  Install both to PREFIX=$(PREFIX) or set PKG_CONFIG_PATH."; \
	    exit 1; \
	fi
	$(CC) $(CFLAGS) -g $< $(LDFLAGS) -o $@

build-profile: $(PROFILE_BIN)

$(RESULTS_DIR):
	mkdir -p $@

$(C_CSV): $(PERF_BIN) | $(RESULTS_DIR)
	@if [ -z "$(ASDF_FILES)" ]; then \
	    echo "error: no *.asdf files found in DATA_DIR=$(DATA_DIR)"; \
	    exit 1; \
	fi
	LD_LIBRARY_PATH=$(GWCS_LIBDIR):$$LD_LIBRARY_PATH \
	    $(_TASKSET) ./$(PERF_BIN) -o $@ $(ASDF_FILES)

run-c: $(C_CSV)

$(PYTHON_CSV): | $(PYTHON) $(RESULTS_DIR)
	@if [ -z "$(ASDF_FILES)" ]; then \
	    echo "error: no *.asdf files found in DATA_DIR=$(DATA_DIR)"; \
	    exit 1; \
	fi
	$(_TASKSET) $(PYTHON) roman_wcs_perf.py \
	    --fast-malloc \
	    -o $@ $(ASDF_FILES)

run-python: $(PYTHON_CSV)

$(PLOT_DIR): | $(RESULTS_DIR)
	mkdir -p $@

plot: | $(PYTHON) $(PLOT_DIR)
	@missing=""; \
	[ -f "$(C_CSV)" ] || missing="$$missing $(C_CSV)"; \
	[ -f "$(PYTHON_CSV)" ] || missing="$$missing $(PYTHON_CSV)"; \
	if [ -n "$$missing" ]; then \
	    echo "error: missing CSV files:$$missing"; \
	    echo "  Run 'make run-c' and/or 'make run-python' first."; \
	    exit 1; \
	fi
	$(PYTHON) summarize.py $(C_CSV) $(PYTHON_CSV) --output-dir $(PLOT_DIR)

clean:
	rm -f $(C_CSV) $(PYTHON_CSV)
	rm -rf $(PLOT_DIR)

distclean: clean
	rm -f $(PERF_BIN) $(PROFILE_BIN)
	rm -rf $(VENV) $(RESULTS_DIR)
