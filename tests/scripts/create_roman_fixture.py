#!/usr/bin/env python3
"""
create_roman_fixture.py

Creates a small test fixture from a Roman Space Telescope ASDF file by
stripping out large image data arrays while preserving all metadata
(including the full WCS tree with its embedded small ndarrays).

Usage:
    create_roman_fixture.py <input.asdf> [-o output.asdf]

The output defaults to tests/fixtures/<input_stem>.asdf relative to the
repository root (two levels up from this script).
"""

import argparse
import os
import sys

import asdf
import numpy as np


LARGE_ARRAY_THRESHOLD = 1024 * 1024  # 1 MB


def strip_large_arrays(tree, threshold=LARGE_ARRAY_THRESHOLD):
    """Remove ndarray entries larger than threshold from the roman top-level dict."""
    roman = tree.get('roman')
    if roman is None:
        print("Warning: no 'roman' key found in tree", file=sys.stderr)
        return tree

    removed = []
    for key in list(roman.keys()):
        val = roman[key]
        if isinstance(val, np.ndarray) and val.nbytes > threshold:
            del roman[key]
            removed.append((key, val.shape, val.dtype, val.nbytes))

    if removed:
        for key, shape, dtype, nbytes in removed:
            print(f"  Removed roman[{key!r}]: shape={shape} dtype={dtype} "
                  f"({nbytes / 1024 / 1024:.1f} MB)")
    else:
        print("  No large arrays found to remove.")

    return tree


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(os.path.dirname(script_dir))
    fixtures_dir = os.path.join(repo_root, 'tests', 'fixtures')

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('input', metavar='input.asdf',
                        help='Roman ASDF file')
    parser.add_argument('-o', '--output', default=None,
                        metavar='output.asdf',
                        help='Output fixture path (default: tests/fixtures/<input_stem>.asdf)')
    parser.add_argument('--threshold', type=int, default=LARGE_ARRAY_THRESHOLD,
                        metavar='BYTES',
                        help='Arrays larger than this (bytes) are removed '
                             f'(default: {LARGE_ARRAY_THRESHOLD})')
    args = parser.parse_args()

    if args.output is None:
        stem = os.path.splitext(os.path.basename(args.input))[0]
        args.output = os.path.join(fixtures_dir, stem + '.asdf')

    print(f'Reading {args.input} ...')
    with asdf.open(args.input, lazy_load=False, memmap=False) as af:
        tree = dict(af.tree)
        # Make a mutable shallow copy of the roman subtree
        if 'roman' in tree:
            tree['roman'] = dict(tree['roman'])

        print('Stripping large arrays:')
        strip_large_arrays(tree, threshold=args.threshold)

        out_dir = os.path.dirname(args.output)
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)

        print(f'Writing {args.output} ...')
        new_af = asdf.AsdfFile(tree)
        new_af.write_to(args.output)

    size = os.path.getsize(args.output)
    print(f'Done. Output size: {size / 1024:.1f} KB')
    return 0


if __name__ == '__main__':
    sys.exit(main())
