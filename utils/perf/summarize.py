#!/usr/bin/env python3
"""
summarize.py

Merge C and Python benchmark CSVs, print a summary table, and render
three matplotlib figures into --output-dir:

  eval_time.png       eval time (s) vs N (log-log)
  throughput.png      throughput (pts/s) vs N (log-log)
  parse_time.png      parse cold/hot comparison (bar chart)

Usage:
    summarize.py c.csv python.csv [--output-dir plots/]
"""

import argparse
import csv
import math
import os
import platform
import sys
from collections import defaultdict

import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def _get_system_info():
    cpu = 'unknown'
    try:
        with open('/proc/cpuinfo') as f:
            for line in f:
                if line.startswith('model name'):
                    cpu = line.split(':', 1)[1].strip()
                    break
    except OSError:
        cpu = platform.processor() or 'unknown'

    n_cores = os.cpu_count() or 0

    ram_gb = None
    try:
        with open('/proc/meminfo') as f:
            for line in f:
                if line.startswith('MemTotal'):
                    ram_gb = int(line.split()[1]) / 1024 ** 2
                    break
    except OSError:
        pass

    blas_info = None
    try:
        from threadpoolctl import threadpool_info
        for pool in threadpool_info():
            if pool.get('user_api') == 'blas':
                blas_info = {
                    'impl': pool.get('internal_api', 'unknown'),
                    'threads': pool['num_threads'],
                }
                break
    except ImportError:
        pass

    return {
        'cpu': cpu,
        'n_cores': n_cores,
        'ram_gb': ram_gb,
        'blas_info': blas_info,
    }


def _format_system_info(info):
    parts = [info['cpu'], f"{info['n_cores']} cores"]
    if info['ram_gb'] is not None:
        parts.append(f'{info["ram_gb"]:.0f} GB RAM')
    if info['blas_info'] is not None:
        _blas_names = {'openblas': 'OpenBLAS', 'mkl': 'MKL', 'blis': 'BLIS'}
        raw = info['blas_info']['impl']
        impl = _blas_names.get(raw, raw.upper())
        threads = info['blas_info']['threads']
        parts.append(f'{impl} ({threads} threads)')
    return '  |  '.join(parts)


def load_csv(path):
    rows = []
    with open(path, newline='') as f:
        for row in csv.DictReader(f):
            rows.append({
                'library':  row['library'],
                'file':     row['file'],
                'detector': row['detector'],
                'phase':    row['phase'],
                'n_points': int(row['n_points']),
                'rep':      int(row['rep']),
                'time_s':   float(row['time_s']),
            })
    return rows


def filter_rows(rows, *, phase=None, library=None, n_points=None):
    return [
        r for r in rows
        if (phase is None or r['phase'] == phase)
        and (library is None or r['library'] == library)
        and (n_points is None or r['n_points'] == n_points)
    ]


# Statistics helpers

def percentile_stats(values, p=(25, 50, 75)):
    arr = np.asarray(values, dtype=float)
    return tuple(float(np.percentile(arr, q)) for q in p)


def collect_eval_stats(rows, library):
    """Return dict {N: (p25, p50, p75)} for eval phase.

    Pools across all reps and detectors.
    """
    by_n = defaultdict(list)
    for r in filter_rows(rows, phase='eval', library=library):
        by_n[r['n_points']].append(r['time_s'])
    return {n: percentile_stats(times) for n, times in sorted(by_n.items())}


def collect_parse_stats(rows, library):
    """Return dict {phase: median} for parse_cold / parse_hot."""
    result = {}
    for phase in ('parse_cold', 'parse_hot'):
        times = [
            r['time_s']
            for r in filter_rows(rows, phase=phase, library=library)
        ]
        result[phase] = float(np.median(times)) if times else math.nan
    return result


# Text table

def _format_n(n):
    if n <= 10:
        return str(n)
    return f'1e{int(math.log10(n))}'


LIBRARY_LABEL = {
    'libasdf_gwcs': 'libasdf-gwcs (C)',
    'gwcs': 'GWCS (Py)',
}


def print_table(rows):
    libraries = sorted({r['library'] for r in rows})
    n_values = sorted(
        {r['n_points'] for r in filter_rows(rows, phase='eval')}
    )

    print('\n=== Parse times (median across all detectors) ===')
    hdr = (f"{'Phase':<14}"
           + ''.join(f'  {LIBRARY_LABEL.get(l, l):>20}' for l in libraries))
    print(hdr)
    print('-' * len(hdr))
    for phase in ('parse_cold', 'parse_hot'):
        label = 'cold parse' if 'cold' in phase else 'hot parse '
        line = f'{label:<14}'
        for lib in libraries:
            stats = collect_parse_stats(rows, lib)
            v = stats.get(phase, math.nan)
            line += f'  {v*1e3:>17.1f} ms'
        print(line)

    print('\n=== Eval throughput (median pts/s) ===')
    hdr = (f"{'N':>4}"
           + ''.join(f'  {LIBRARY_LABEL.get(l, l):>20}' for l in libraries))
    if len(libraries) == 2:
        hdr += f"  {'C/Py':>8}"
    print(hdr)
    print('-' * len(hdr))

    for N in n_values:
        medians = {}
        for lib in libraries:
            stats = collect_eval_stats(rows, lib)
            p25, p50, p75 = stats.get(N, (math.nan, math.nan, math.nan))
            medians[lib] = p50
        line = f'{_format_n(N):>4}'
        for lib in libraries:
            m = medians[lib]
            tps = N / m if m > 0 else math.nan
            line += f'  {tps:>20,.0f}'
        if len(libraries) == 2:
            c_med = medians.get('libasdf_gwcs', math.nan)
            py_med = medians.get('gwcs', math.nan)
            ratio = (py_med / c_med
                     if (c_med > 0 and py_med > 0) else math.nan)
            line += f'  {ratio:>8,.2f}'
        print(line)
    print()


# Plots

COLORS = {
    'libasdf_gwcs': '#1f77b4',
    'gwcs': '#ff7f0e',
}


def _add_sysinfo_footer(fig, sysinfo):
    fig.text(0.5, 0.01, _format_system_info(sysinfo),
             ha='center', va='bottom', fontsize=6.5, color='#555555')


def plot_eval_time(rows, libraries, output_dir, sysinfo=None):
    fig, ax = plt.subplots(figsize=(8, 5))

    for lib in libraries:
        stats = collect_eval_stats(rows, lib)
        if not stats:
            continue
        ns = np.array(sorted(stats))
        p25s = np.array([stats[n][0] for n in ns])
        p50s = np.array([stats[n][1] for n in ns])
        p75s = np.array([stats[n][2] for n in ns])
        color = COLORS.get(lib)
        label = LIBRARY_LABEL.get(lib, lib)
        ax.fill_between(ns, p25s, p75s, alpha=0.2, color=color)
        ax.plot(ns, p50s, marker='o', label=label, color=color)

    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('N (number of pixel coordinate pairs)')
    ax.set_ylabel('Eval time (s)')
    ax.set_title(
        'WCS eval time vs N  [median ± IQR across detectors & reps]')
    ax.legend()
    ax.grid(True, which='both', ls='--', alpha=0.4)
    if sysinfo:
        _add_sysinfo_footer(fig, sysinfo)
    fig.tight_layout(rect=[0, 0.04, 1, 1])
    path = os.path.join(output_dir, 'eval_time.png')
    fig.savefig(path, dpi=150)
    plt.close(fig)
    print(f'  wrote {path}')


def plot_throughput(rows, libraries, output_dir, sysinfo=None):
    fig, ax = plt.subplots(figsize=(8, 5))

    for lib in libraries:
        stats = collect_eval_stats(rows, lib)
        if not stats:
            continue
        ns    = np.array(sorted(stats))
        tp25s = np.array([ns[i] / stats[n][2] for i, n in enumerate(ns)])
        tp50s = np.array([ns[i] / stats[n][1] for i, n in enumerate(ns)])
        tp75s = np.array([ns[i] / stats[n][0] for i, n in enumerate(ns)])
        color = COLORS.get(lib, None)
        label = LIBRARY_LABEL.get(lib, lib)
        ax.fill_between(ns, tp25s, tp75s, alpha=0.2, color=color)
        ax.plot(ns, tp50s, marker='o', label=label, color=color)

    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('N (number of pixel coordinate pairs)')
    ax.set_ylabel('Throughput (pts / s)')
    ax.set_title(
        'WCS eval throughput vs N  [median ± IQR across detectors & reps]')
    ax.legend()
    ax.grid(True, which='both', ls='--', alpha=0.4)
    if sysinfo:
        _add_sysinfo_footer(fig, sysinfo)
    fig.tight_layout(rect=[0, 0.04, 1, 1])
    path = os.path.join(output_dir, 'throughput.png')
    fig.savefig(path, dpi=150)
    plt.close(fig)
    print(f'  wrote {path}')


def plot_parse_time(rows, libraries, output_dir, sysinfo=None):
    phases = ('parse_cold', 'parse_hot')
    x = np.arange(len(phases))
    width = 0.35
    offsets = np.linspace(-(len(libraries)-1)*width/2,
                          (len(libraries)-1)*width/2,
                          len(libraries))

    fig, ax = plt.subplots(figsize=(7, 5))

    for lib, offset in zip(libraries, offsets):
        stats = collect_parse_stats(rows, lib)
        values_ms = [stats.get(p, math.nan) * 1e3 for p in phases]
        color = COLORS.get(lib, None)
        label = LIBRARY_LABEL.get(lib, lib)
        ax.bar(x + offset, values_ms, width, label=label, color=color,
               alpha=0.85)

    ax.set_xticks(x)
    ax.set_xticklabels([
        'Cold parse\n(first open)',
        'Hot parse\n(page cache warm)',
    ])
    ax.set_ylabel('Time (ms)')
    ax.set_title('Parse time comparison  [median across detectors]')
    ax.legend()
    ax.grid(True, axis='y', ls='--', alpha=0.4)
    if sysinfo:
        _add_sysinfo_footer(fig, sysinfo)
    fig.tight_layout(rect=[0, 0.04, 1, 1])
    path = os.path.join(output_dir, 'parse_time.png')
    fig.savefig(path, dpi=150)
    plt.close(fig)
    print(f'  wrote {path}')


# Main

def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument('c_csv', metavar='C_CSV')
    parser.add_argument('python_csv', metavar='PYTHON_CSV')
    parser.add_argument('--output-dir', default='plots', metavar='DIR')
    args = parser.parse_args()

    rows = []
    for path in (args.c_csv, args.python_csv):
        if not os.path.exists(path):
            print(f'WARNING: {path} not found, skipping', file=sys.stderr)
            continue
        rows.extend(load_csv(path))

    if not rows:
        print('ERROR: no data loaded', file=sys.stderr)
        return 1

    libraries = sorted({r['library'] for r in rows})
    os.makedirs(args.output_dir, exist_ok=True)

    sysinfo = _get_system_info()
    print(f'\nSystem: {_format_system_info(sysinfo)}')

    print_table(rows)
    print(f'Writing plots to {args.output_dir}/')
    plot_eval_time(rows, libraries, args.output_dir, sysinfo)
    plot_throughput(rows, libraries, args.output_dir, sysinfo)
    plot_parse_time(rows, libraries, args.output_dir, sysinfo)
    return 0


if __name__ == '__main__':
    sys.exit(main())
