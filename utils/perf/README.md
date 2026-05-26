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
  N ∈ {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 16711744}, with
  multiple repetitions per N. The upper bound (16711744 = 4088 x 4088) is
  the active science area of a single Roman WFI detector (the full array is
  4096 x 4096, with a 4-pixel border of reference pixels on each edge).

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
Both libraries run single-threaded and are built with full optimisation
(`-O2` or equivalent).  Python's NumPy links against OpenBLAS, but the
WCS pipeline does not perform matrix operations large enough to trigger
OpenBLAS's internal parallelisation threshold, so multi-threading is not
a factor in these results. Of course, the nature of the problem is such
that it could be easily parallelized.

### Parse times (median across all detectors)

| Phase | GWCS (Py) | libasdf-gwcs (C) |
|:------|---:|---:|
| cold parse | 1410.9 ms | 20.0 ms |
| hot parse | 1386.7 ms | 10.4 ms |

![Parse time comparison](results/plots/parse_time.png)

The C library parses roughly **70x faster** than Python in both cases.
The cold/hot difference in C (~10 ms) reflects file I/O almost exclusively.
The Python cold/hot difference varies considerably across runs--in this run
it was unusually small (~24 ms), while earlier runs showed differences of
100 ms or more. It likely reflects a mix of page cache state, internal
caching within astropy/gwcs (unit registries, coordinate frame singletons,
schema validation), and Python interpreter effects. The exact causes have
not been fully investigated; the parse timings should be treated as
approximate.

### Eval throughput (median px/s)

| N | GWCS (Py) | libasdf-gwcs (C) | C/Py |
|--:|---:|---:|---:|
| 1 | 221 | 22,332 | 101.01 |
| 10 | 2,177 | 209,024 | 96.00 |
| 1e2 | 21,656 | 1,132,638 | 52.30 |
| 1e3 | 194,391 | 1,954,881 | 10.06 |
| 1e4 | 1,290,846 | 1,643,255 | 1.27 |
| 1e5 | 2,982,696 | 1,597,975 | 0.54 |
| 1e6 | 1,458,837 | 1,573,563 | 1.08 |
| 1e7 | 359,954 | 1,571,500 | 4.37 |
| 1.7e7 | 504,571 | 1,551,368 | 3.07 |

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
dimension: parse time is ~70x lower and per-evaluation overhead is ~100x lower.

### Large N: a brief Python window, then C dominates

The two implementations reach parity around N=10,000. NumPy pulls ahead
at N=100,000 (~1.85x faster), which is the only point in the sweep where
Python leads. From N=1,000,000 onward C is faster, and by N=10,000,000
C is **over 4x faster**.

The brief Python advantage around N=100,000 is explained by NumPy's
vectorised ufuncs: once N is large enough to amortise Python call overhead,
NumPy's compiled inner loops outpace AST's scalar per-point evaluation.
The C library evaluates each transform point-by-point via the
[AST](https://github.com/Starlink/ast) library, which does not use SIMD
intrinsics.

The key to C's recovery at large N is that AST evaluates the full pipeline
for one point at a time, so its memory footprint beyond the input/output
arrays is effectively constant (just pipeline coefficients, a few KB).
NumPy, by contrast, materialises each pipeline stage as a complete N-element
intermediate array. For a pipeline with K stages this means approximately
K x N x 8 bytes of working set--at N=10,000,000 with even 10 stages that
is ~800 MB, far exceeding both L3 cache (8 MB) and likely saturating
memory bandwidth. Python's throughput drops precipitously from ~3M px/s at
N=100,000 to ~360K px/s at N=10,000,000 likely for this reason, while C's
throughput remains flat around 1.55M px/s, consistent with a scalar loop
reading/writing four arrays (xin, yin, xout, yout) sequentially from RAM at
a steady rate.

At full active-area scale (N = 4088 x 4088 = 16,711,744 pixels per WFI
detector), C maintains a ~3x throughput advantage over Python.

The NumPy advantage at medium N could be closed by adding SIMD vectorisation
to AST's transform evaluation (see recommendations below); the large-N
advantage is structural and stems from the fundamentally lower memory
footprint of the point-by-point evaluation model.

### Recommendations for closing the medium-N gap

The natural path to eliminating Python's remaining advantage around N=100,000
is to add SIMD vectorisation to AST's transform evaluation:

- **Polynomial transforms**: Horner evaluation is already optimal
  algorithmically; a vectorised implementation (AVX2 or SVE, or compiler
  auto-vectorisation with appropriate hints) would achieve much better
  instruction throughput on large arrays.
- **Affine/matrix transforms**: Replacing scalar loops with `cblas_dgemv` /
  `cblas_dgemm` would match NumPy directly and maybe benefit from
  multi-threaded BLAS at large N.
- **Spherical projection**: The trigonometric operations are memory-bandwidth
  bound at large N; SIMD implementations of `sin`/`cos`/`atan2` (e.g. via
  libmvec or SLEEF) would help here.

These would be contributions to AST itself rather than to libasdf-gwcs.

## History

This is a living document updated as new benchmark runs are performed.

### 2026-05-26

First fully optimised benchmark run: both libasdf-gwcs and libasdf confirmed
built with `-O2` (previous runs may have inadvertently used debug flags on
libasdf).  The N sweep was extended to cover 10,000,000 and 16,711,744
(= 4088 x 4088, the active science area of a single Roman Wide Field
Instrument detector; the full array is 4096 x 4096 with a 4-pixel reference
pixel border on each edge), confirming that the C library maintains a 3-4x
throughput advantage over Python at full-detector scale where NumPy's per-stage
intermediate array allocation exhausts memory bandwidth.

**Software versions:**

| Component | Version |
|:---|:---|
| libasdf-gwcs | [a8ce6e91](https://github.com/asdf-format/libasdf-gwcs/commit/a8ce6e91b701c0b0f06e86bc796ec5fe75ddfef9) |
| libasdf | [02c375ee](https://github.com/asdf-format/libasdf/commit/02c375ee2fbd6ce5c921d6e1ad3b09815ffd8c19) |
| libfyaml | 1.0.0-alpha4 |
| Python | 3.12.3 |
| NumPy | 2.4.6 |
| asdf (Python) | 5.3.0 |
| astropy | 7.2.0 |
| gwcs | 1.0.3 |
