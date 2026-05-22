#!/usr/bin/env python3
"""
create_roman_fixture.py

Creates a small test fixture from a Roman Space Telescope ASDF file by
stripping all top-level array data from the roman node while preserving
all metadata (including the full WCS tree with its embedded ndarrays).

Usage:
    create_roman_fixture.py <input.asdf> [-o output.asdf]
"""

import argparse
import os
import sys

import asdf
import numpy as np


def strip_arrays(tree):
    """Remove all ndarray entries from the roman top-level dict."""
    roman = tree.get('roman')
    if roman is None:
        print("Warning: no 'roman' key found in tree", file=sys.stderr)
        return tree

    removed = []
    for key in list(roman.keys()):
        if isinstance(roman[key], np.ndarray):
            del roman[key]
            removed.append(key)

    if removed:
        print(f"  Removed: {', '.join(removed)}")
    else:
        print("  No arrays found to remove.")

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
    args = parser.parse_args()

    if args.output is None:
        stem = os.path.splitext(os.path.basename(args.input))[0]
        args.output = os.path.join(fixtures_dir, stem + '.asdf')

    print(f'Reading {args.input} ...')
    with asdf.open(args.input, lazy_load=False, memmap=False) as af:
        tree = dict(af.tree)
        if 'roman' in tree:
            tree['roman'] = dict(tree['roman'])

        print('Stripping arrays:')
        strip_arrays(tree)

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
