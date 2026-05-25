# WCS Evaluation Performance Benchmark

This directory contains tools to benchmark [libasdf-gwcs](../../README.md)
(C) against the Python [GWCS](https://gwcs.readthedocs.io/) library on
real Roman Space Telescope ASDF calibration files.

## What is measured

Three phases are timed per file:

- **parse\_cold** -- open the file, deserialize the WCS tree, and create an
  eval context, with the file first evicted from the OS page cache via
  `POSIX_FADV_DONTNEED` to approximate a cold-cache read.
- **parse\_hot** -- the same sequence immediately after, with the file's pages
  warm in the page cache.
- **eval** -- coordinate evaluation (`asdf_gwcs_eval_2d` / `wcs.pixel_to_world`)
  over N randomly sampled pixel coordinate pairs, for
  N ∈ {1, 10, 100, 1000, 10000, 100000, 1000000}, with multiple
  repetitions per N.

All results are written to a CSV in long format
(`library, file, detector, phase, n_points, rep, time_s, blas_threads`).

## Scripts

| Script | Purpose |
|:---|:---|
| `roman_wcs_perf.c` | C benchmark (compiled to `roman_wcs_perf`) |
| `roman_wcs_perf.py` | Python benchmark |
| `summarize.py` | Merge CSVs, print tables, render plots |

### Running

The `Makefile` handles building, running, and plotting.  The most important
variables are:

- `PREFIX` -- installation prefix where libasdf-gwcs is installed
  (default: `~/.local`)
- `DATA_DIR` -- directory containing the Roman `*.asdf` calibration files
  (default: `../roman/.data`)
- `PKG_CONFIG_PATH` -- supplemental pkg-config search path, if libasdf or
  libyaml are not under `PREFIX`

```sh
# Build the C binary, create the Python venv, run both benchmarks, plot:
make PREFIX=/path/to/libasdf-gwcs DATA_DIR=/path/to/asdf/files

# Individual steps:
make build-c       # compile roman_wcs_perf only
make venv          # create Python venv with all dependencies
make run-c         # run C benchmark  -> results/c_perf_results.csv
make run-python    # run Python benchmark -> results/python_perf_results.csv
make plot          # merge CSVs and render plots into results/plots/
make clean         # remove CSVs and plots
make distclean     # also remove venv and compiled binary
```

See the comments at the top of [`Makefile`](Makefile) for the full list of
variables.

## Reference results

Measured on an Intel Core i7-7820HQ laptop (2.90 GHz, 8 cores, 31 GB RAM).
Python evaluation used OpenBLAS with 8 threads; the C library is
single-threaded throughout.  Both libraries built with full optimisation
(`-O2` or equivalent).

### Parse times (median across all detectors)

| Phase | GWCS (Py) [8t] | libasdf-gwcs (C) |
|:------|---:|---:|
| cold parse | 972.2 ms | 38.4 ms |
| hot parse | 926.6 ms | 24.1 ms |

![Parse time comparison](results/plots/parse_time.png)

The C library parses roughly **25x faster** than Python in both cases.
The cold/hot difference in C (~14 ms) reflects file I/O almost exclusively.
The Python cold/hot difference is highly variable across files and runs and
is harder to attribute precisely--it likely reflects a mix of page cache
state, internal caching within astropy/gwcs (unit registries, coordinate
frame singletons, schema validation), and Python interpreter effects. The
exact causes have not been fully investigated; the parse timings should be
treated as approximate.

### Eval throughput (median px/s)

| N | GWCS (Py) [8t] | libasdf-gwcs (C) | C/Py |
|--:|---:|---:|---:|
| 1 | 241 | 24,167 | 100.27 |
| 10 | 2,350 | 219,015 | 93.21 |
| 1e2 | 23,149 | 948,065 | 40.95 |
| 1e3 | 211,054 | 1,677,430 | 7.95 |
| 1e4 | 1,377,465 | 1,438,930 | 1.04 |
| 1e5 | 3,147,311 | 1,432,711 | 0.46 |
| 1e6 | 1,680,373 | 1,415,603 | 0.84 |

![Eval throughput vs N](results/plots/throughput.png)

![Eval time vs N](results/plots/eval_time.png)

## Analysis

### Small N: C is dramatically faster

At N=1 and N=10, the C library is **~100x faster**. This regime is dominated
by per-call overhead: Python's function call machinery, object allocation,
argument checking, and the NumPy array creation and ufunc dispatch overhead on
every `pixel_to_world` call. The C library incurs none of this--evaluation is a
direct function call into compiled code with pre-allocated scratch buffers.

This is the most practically important regime for many use cases. Opening a
handful of ASDF files and extracting a small cutout per file (the typical
interactive or pipeline-setup pattern) may involve relatively few coordinate
evaluations per call. In that scenario the C library is faster in every
dimension: parse time is ~25x lower and per-evaluation overhead is ~100x lower.

### Large N: Python overtakes C, then C recovers

The two implementations reach parity around N=10,000. Above that, Python
pulls ahead, peaking at roughly **2x faster** at N=100,000. By N=1,000,000
C recovers to within ~15% of Python throughput.

The Python advantage in the N=10,000--100,000 range is likely explained by
NumPy's vectorised evaluation. Once N is large enough, NumPy's compiled
ufuncs and--for affine transforms--BLAS routines dominate the runtime,
amortising all Python overhead. The C library currently evaluates each
transform with scalar loops via the
[AST](https://github.com/Starlink/ast) library, which does not use BLAS or
explicit SIMD intrinsics.

The partial C recovery at N=1,000,000 is consistent with the working set
exceeding L3 cache, at which point both implementations become
memory-bandwidth-bound and the vectorisation advantage narrows.

A rerun with `OPENBLAS_NUM_THREADS=1` (forcing NumPy to single-threaded BLAS)
produced almost the same crossover point and curve shape, with only a slight
reduction in Python throughput at large N. This confirms that the advantage is
primarily from NumPy's vectorised scalar loops and ufuncs rather than
multi-threaded matrix operations--the Roman WCS pipeline contains only one or
two affine stages, with most of the work in polynomial and projection
transforms.

### Recommendations for improving large-N C performance

The natural path to closing the large-N gap is to add BLAS and SIMD
acceleration to the AST library's transform evaluation:

- **Polynomial transforms**: Horner evaluation is already optimal
  algorithmically; a vectorised implementation (AVX2 or SVE, or compiler
  auto-vectorisation with appropriate hints) would achieve much better
  instruction throughput on large arrays.
- **Affine/matrix transforms**: Replacing scalar loops with `cblas_dgemv` /
  `cblas_dgemm` would match NumPy directly and benefit from multi-threaded
  BLAS at large N.
- **Spherical projection**: The trigonometric operations are memory-bandwidth
  bound at large N; SIMD implementations of `sin`/`cos`/`atan2` (e.g. via
  libmvec or SLEEF) would help here.

These would be contributions to AST itself rather than to libasdf-gwcs.
