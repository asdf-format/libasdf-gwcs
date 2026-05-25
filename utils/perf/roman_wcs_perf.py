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
import os
import sys
from time import perf_counter

import numpy as np

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

N_SWEEP = [1, 10, 100, 1_000, 10_000, 100_000, 1_000_000]
N_REPS = [50, 50, 30, 15, 8, 5, 3]


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


def bench_file(filepath, writer, fout, blas_threads):
    """Benchmark one ASDF file. Returns True on success."""
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
        'gwcs', filepath, detector, 'parse_cold', 0, 0,
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
        'gwcs', filepath, detector, 'parse_hot', 0, 0,
        f'{hot_s:.9f}', blas_threads,
    ])
    fout.flush()

    print(f'  parse  cold={cold_s*1e3:.3f} ms  hot={hot_s*1e3:.3f} ms',
          file=sys.stderr)

    # Eval sweep
    try:
        for N, reps in zip(N_SWEEP, N_REPS):
            # Reproducible random pixel coordinates; regenerate per N level.
            rng = np.random.default_rng(RAND_SEED)
            xin = rng.uniform(0.0, IMAGE_NX, N)
            yin = rng.uniform(0.0, IMAGE_NY, N)

            rep_times = []
            for rep in range(reps):
                t0 = perf_counter()
                wcs.pixel_to_world(xin, yin)
                elapsed = perf_counter() - t0
                rep_times.append(elapsed)
                writer.writerow([
                    'gwcs', filepath, detector, 'eval', N, rep,
                    f'{elapsed:.9f}', blas_threads,
                ])
            fout.flush()

            med = _median(rep_times)
            print(f'  N={N:<8d}  median={med*1e3:.3f} ms'
                  f'  ({N/med:.0f} px/s)', file=sys.stderr)
    except Exception as exc:
        print(f'  error during eval sweep: {exc}', file=sys.stderr)
        af_hot.close()
        return False

    af_hot.close()
    return True


def main():
    parser = argparse.ArgumentParser(
        description='Benchmark Python GWCS WCS evaluation and write results to CSV'
    )
    parser.add_argument('files', nargs='+', metavar='file.asdf')
    parser.add_argument('-o', '--output', default='python_perf_results.csv',
                        metavar='output.csv')
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
            if bench_file(filepath, writer, fout, blas_threads):
                n_success += 1

    print(f'\nSummary: {n_success}/{nfiles} files succeeded', file=sys.stderr)
    print(f'Output: {args.output}', file=sys.stderr)
    return 0 if n_success > 0 else 1


if __name__ == '__main__':
    sys.exit(main())
