#!/usr/bin/env python3
"""
roman_wcs_perf.py

Benchmark Python GWCS WCS evaluation on Roman Space Telescope ASDF files.

Mirrors the measurement structure of roman_wcs_perf.c:
  parse_cold  first asdf.open + wcs access + one forced eval (see note below)
              (reflects OS page-cache state at invocation time)
  parse_hot   same sequence immediately after, page cache definitely warm
  eval        wcs.pixel_to_world at N in {1,10,100,...,1M}, multiple reps

Note on parse timing: accessing the WCS key in an ASDF file triggers lazy
deserialization of the GWCS transform tree.  A single pixel_to_world call is
included in the parse window to ensure the full pipeline (including any
deferred construction) is counted.

Output CSV (long format, matches roman_wcs_perf.c):
  library,file,detector,phase,n_points,rep,time_s,blas_threads

Usage:
    roman_wcs_perf.py [-o output.csv] file1.asdf [file2.asdf ...]
"""

import argparse
import csv
import ctypes
import gc
import os
import sys
from time import perf_counter

import numpy as np

_libc = ctypes.CDLL(None, use_errno=True)

# glibc malloc tuning parameters (from <malloc.h>).
M_TRIM_THRESHOLD = -1
M_MMAP_THRESHOLD = -3

try:
    import asdf
    import gwcs          # noqa: F401 — registers converters
    import roman_datamodels  # noqa: F401 — registers top-level tag
except ImportError as exc:
    print(f'error: missing dependency: {exc}', file=sys.stderr)
    print('Install with: pip install asdf gwcs asdf-astropy asdf-wcs-schemas roman-datamodels',
          file=sys.stderr)
    sys.exit(1)


IMAGE_NX = 4088.0
IMAGE_NY = 4088.0
RAND_SEED = 42

# Upper bound is the active science area of a Roman WFI detector (4088 x 4088 px;
# full array is 4096 x 4096 with a 4-pixel reference pixel border on each edge).
N_SWEEP = [1,   10,  100,  1_000, 10_000, 100_000, 1_000_000, 10_000_000, 16_711_744]
REPS    = [100, 100,  50,     20,     10,       5,         3,          2,          2]

# Rep count when -N is given on the command line.
_N_OVERRIDE_REPS = 10


def _evict_file(filepath):
    """Evict filepath from the OS page cache via POSIX_FADV_DONTNEED."""
    try:
        fd = os.open(filepath, os.O_RDONLY)
        try:
            size = os.stat(filepath).st_size
            os.posix_fadvise(fd, 0, size, os.POSIX_FADV_DONTNEED)
        finally:
            os.close(fd)
    except (AttributeError, OSError):
        pass


def _load_wcs(filepath):
    """
    Open filepath and return (af, wcs, detector).
    The GWCS pipeline is fully initialised via a single warm-up eval call so
    that deferred construction cost is included in the parse timing window.
    Caller is responsible for closing af.
    """
    af = asdf.open(filepath, lazy_load=False, memmap=False)
    wcs = af['roman']['meta']['wcs']
    try:
        detector = af['roman']['meta']['instrument']['detector'].lower()
    except (KeyError, AttributeError):
        detector = 'unknown'

    # Force complete pipeline initialisation.
    wcs.pixel_to_world(np.array([2044.0]), np.array([2044.0]))
    return af, wcs, detector


def _median(values):
    s = sorted(values)
    return s[len(s) // 2]


def _proc_stats():
    """Return (minflt, majflt, vm_rss_kb) from /proc/self."""
    try:
        fields = open('/proc/self/stat').read().split()
        minflt = int(fields[9])
        majflt = int(fields[11])
    except Exception:
        minflt = majflt = 0
    try:
        vm_rss_kb = 0
        for line in open('/proc/self/status'):
            if line.startswith('VmRSS:'):
                vm_rss_kb = int(line.split()[1])
                break
    except Exception:
        vm_rss_kb = 0
    return minflt, majflt, vm_rss_kb


def bench_file(filepath, writer, fout, blas_threads, n_only=None,
               no_bounding_box=True, fast_malloc=False):
    """Benchmark one ASDF file. Returns True on success."""
    library = 'gwcs'

    # Cold parse--evict first to ensure page cache is cold.
    _evict_file(filepath)
    try:
        t0 = perf_counter()
        af_cold, _, detector = _load_wcs(filepath)
        cold_s = perf_counter() - t0
        af_cold.close()
    except Exception as exc:
        print(f'  error: cold parse failed: {exc}', file=sys.stderr)
        return False

    writer.writerow([
        library, filepath, detector, 'parse_cold', 0, 0,
        f'{cold_s:.9f}', blas_threads,
    ])
    fout.flush()

    # Hot parse (page cache warm; keep result for eval)
    try:
        t0 = perf_counter()
        af_hot, wcs, _ = _load_wcs(filepath)
        hot_s = perf_counter() - t0
    except Exception as exc:
        print(f'  error: hot parse failed: {exc}', file=sys.stderr)
        return False

    writer.writerow([
        library, filepath, detector, 'parse_hot', 0, 0,
        f'{hot_s:.9f}', blas_threads,
    ])
    fout.flush()

    print(f'  parse  cold={cold_s*1e3:.3f} ms  hot={hot_s*1e3:.3f} ms',
          file=sys.stderr)

    if no_bounding_box:
        # Disable ModelBoundingBox so the pipeline runs directly on all N
        # input points without mask/NaN overhead.  libasdf-gwcs/AST does no
        # bounding-box checking, so this gives a fairer comparison of raw
        # pipeline throughput.  Re-enable with --with-bounding-box.
        wcs.forward_transform.bounding_box = None

    # Eval sweep.
    if fast_malloc:
        # Set M_MMAP_THRESHOLD=256MB so intermediate numpy arrays use
        # brk()-based heap allocation instead of mmap(MAP_ANONYMOUS).
        # With mmap, each alloc/free cycle uses a fresh virtual address,
        # causing millions of demand-paging minor faults per pipeline call at
        # large N (measured as ~2.7M faults/call at N=10M on my machine,
        # contributing several seconds of kernel overhead).  brk()
        # reuses virtual addresses and physical pages across calls, avoiding
        # most of those faults.  Applied here rather than at import time so
        # that ASDF parsing above uses the default threshold and does not
        # inflate the brk heap before eval begins.
        _libc.mallopt(M_MMAP_THRESHOLD, 256 * 1024 * 1024)
        # Also disable heap trimming so freed brk pages are not returned to
        # the OS between N levels.  Without this, glibc trims the brk heap
        # after the N=10M eval completes (default M_TRIM_THRESHOLD=128KB is
        # far below the ~400MB freed), cold-faulting all pages again when
        # N=16.7M begins.
        _libc.mallopt(M_TRIM_THRESHOLD, 512 * 1024 * 1024)

    gc.disable()

    try:
        if n_only is not None:
            sweep = [(n_only, _N_OVERRIDE_REPS)]
        else:
            sweep = zip(N_SWEEP, REPS)
        for N, reps in sweep:
            # Reproducible random pixel coordinates; regenerate per N level.
            rng = np.random.default_rng(RAND_SEED)
            xin = rng.uniform(0.0, IMAGE_NX, N)
            yin = rng.uniform(0.0, IMAGE_NY, N)

            minflt0, majflt0, _ = _proc_stats()
            rep_times = []
            for rep in range(reps):
                t0 = perf_counter()
                wcs.pixel_to_world(xin, yin)
                elapsed = perf_counter() - t0
                rep_times.append(elapsed)
                writer.writerow([
                    library, filepath, detector, 'eval', N, rep,
                    f'{elapsed:.9f}', blas_threads,
                ])
            fout.flush()

            minflt1, majflt1, rss_kb = _proc_stats()
            med = _median(rep_times)
            print(f'  N={N:<8d}  reps={reps:<3d}  '
                  f'median={med*1e3:.3f} ms  ({N/med:.0f} px/s)  '
                  f'minflt=+{minflt1-minflt0}  majflt=+{majflt1-majflt0}  '
                  f'rss={rss_kb//1024} MB', file=sys.stderr)
    except Exception as exc:
        print(f'  error during eval sweep: {exc}', file=sys.stderr)
        af_hot.close()
        gc.enable()
        return False

    af_hot.close()
    gc.enable()
    return True


def main():
    parser = argparse.ArgumentParser(
        description='Benchmark Python GWCS WCS evaluation and write results to CSV'
    )
    parser.add_argument('files', nargs='+', metavar='file.asdf')
    parser.add_argument('-o', '--output', default='python_perf_results.csv',
                        metavar='output.csv')
    parser.add_argument('-N', '--n-points', type=int, default=None,
                        metavar='N',
                        help='benchmark only this N (skip parse timing, skip all other N)')
    parser.add_argument('--with-bounding-box', action='store_true', default=False,
                        help='re-enable ModelBoundingBox on the forward transform '
                             '(disabled by default; AST does no bounding-box checking '
                             'so the default gives a fairer throughput comparison)')
    parser.add_argument('--fast-malloc', action='store_true', default=False,
                        help='set glibc M_MMAP_THRESHOLD=256MB before eval so that '
                             'large numpy intermediate arrays use brk()-based heap '
                             'reuse instead of mmap()/munmap() per allocation; reduces '
                             'demand-paging overhead at large N on glibc Linux '
                             '(enabled by default in the Makefile; see source for details)')
    args = parser.parse_args()

    blas_threads = 1
    try:
        from threadpoolctl import threadpool_info
        for pool in threadpool_info():
            if pool.get('user_api') == 'blas':
                blas_threads = pool['num_threads']
                break
    except ImportError:
        pass

    print(f'BLAS threads: {blas_threads}', file=sys.stderr)

    n_success = 0
    nfiles = len(args.files)

    with open(args.output, 'w', newline='') as fout:
        writer = csv.writer(fout)
        writer.writerow([
            'library', 'file', 'detector',
            'phase', 'n_points', 'rep', 'time_s', 'blas_threads',
        ])
        for file_idx, filepath in enumerate(args.files, 1):
            print(f'[{file_idx}/{nfiles}] {filepath}', file=sys.stderr)
            if bench_file(filepath, writer, fout, blas_threads,
                          n_only=args.n_points,
                          no_bounding_box=not args.with_bounding_box,
                          fast_malloc=args.fast_malloc):
                n_success += 1

    print(f'\nSummary: {n_success}/{nfiles} files succeeded', file=sys.stderr)
    print(f'Output: {args.output}', file=sys.stderr)
    return 0 if n_success > 0 else 1


if __name__ == '__main__':
    sys.exit(main())
