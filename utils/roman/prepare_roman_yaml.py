#!/usr/bin/env python3
"""
prepare_roman_yaml.py

Opens each Roman Space Telescope calibration ASDF file, extracts the WCS
subtree, and writes a new minimal ASDF file with all ndarrays stored
inline (no binary blocks).  The resulting files can be read by AST's
YamlChan, which does not support ASDF binary blocks.

Usage:
    prepare_roman_yaml.py [-o output_dir] file1.asdf [file2.asdf ...]
"""

import os
import sys
import argparse
import asdf


def main():
    parser = argparse.ArgumentParser(
        description='Convert Roman ASDF files to inline-array format')
    parser.add_argument('files', nargs='+', metavar='file.asdf',
                        help='Roman ASDF file(s) to convert')
    parser.add_argument('-o', '--output-dir', default='inline',
                        metavar='dir',
                        help='Output directory (default: inline)')
    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    n_ok = 0
    n_fail = 0
    nfiles = len(args.files)

    for infile in args.files:
        outfile = os.path.join(args.output_dir, os.path.basename(infile))
        print(f'  {os.path.basename(infile)} ...', end=' ', flush=True)

        try:
            # Open with lazy_load=False so all binary blocks are resolved
            # into numpy arrays before we close the file handle.
            with asdf.open(infile, lazy_load=False, memmap=False) as af:
                wcs = af['roman']['meta']['wcs']
                detector = af['roman']['meta']['instrument']['detector']

                # Wrap in a minimal tree and write with inline array storage.
                # Put detector first so roman_wcs_ast.c can find it quickly.
                new_af = asdf.AsdfFile({'detector': detector, 'wcs': wcs})
                new_af.write_to(outfile, all_array_storage='inline')

            print('OK')
            n_ok += 1

        except Exception as exc:
            print(f'FAILED: {exc}')
            n_fail += 1

    print(f'\nDone: {n_ok}/{nfiles} OK, {n_fail} failed.')
    print(f'Inline files written to {args.output_dir}/')
    return 0 if n_ok > 0 else 1


if __name__ == '__main__':
    sys.exit(main())
