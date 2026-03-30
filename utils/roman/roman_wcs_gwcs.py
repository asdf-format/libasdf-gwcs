#!/usr/bin/env python3
"""
roman_wcs_gwcs.py

Reads Roman Space Telescope WCS from ASDF calibration files using
the Python gwcs library and evaluates a 20x20 pixel grid, writing
pixel->sky coordinates to CSV.

Usage:
    roman_wcs_gwcs.py [-o output.csv] file1.asdf [file2.asdf ...]

Note:
    Much of this is not RST-specific and could be easily refactored
    to test otherwise arbitrary ASDF files containing a WCS.  As
    the current test target is RST though there are a few RST
    specifics, such as the path to the WCS, hard-coded for now.
"""

import sys
import os
import math
import csv
import argparse
import numpy as np

NGRID = 20
IMAGE_NX = 4088
IMAGE_NY = 4088


def build_pixel_grid():
    """Build the same 20x20 pixel grid as roman_wcs_ast.c."""
    xs = []
    ys = []
    for iy in range(NGRID):
        for ix in range(NGRID):
            x = 100.0 + (IMAGE_NX - 200.0) * ix / (NGRID - 1)
            y = 100.0 + (IMAGE_NY - 200.0) * iy / (NGRID - 1)
            xs.append(x)
            ys.append(y)
    return np.array(xs), np.array(ys)


def main():
    parser = argparse.ArgumentParser(
        description="Evaluate Roman WCS via gwcs and write pixel->sky CSV")
    parser.add_argument("files", nargs="+", metavar="file.asdf",
                        help="Roman ASDF file(s) to process")
    parser.add_argument("-o", "--output", default="gwcs_wcs_results.csv",
                        metavar="output.csv",
                        help="Output CSV path (default: gwcs_wcs_results.csv)")
    args = parser.parse_args()

    try:
        import asdf
        import gwcs  # noqa: F401 — needed to register converters
        import roman_datamodels  # noqa: F401 — needed for top-level ASDF tag
    except ImportError as e:
        print(f"ERROR: Missing dependency: {e}", file=sys.stderr)
        print("Install with: pip install asdf gwcs asdf-astropy asdf-wcs-schemas roman-datamodels",
              file=sys.stderr)
        return 1

    xin, yin = build_pixel_grid()
    npts = len(xin)

    n_success = 0
    n_fail = 0
    nfiles = len(args.files)

    with open(args.output, "w", newline="") as fout:
        writer = csv.writer(fout)
        writer.writerow(["filename", "detector", "pixel_x", "pixel_y", "ra_deg", "dec_deg"])

        for filepath in args.files:
            print(f"Processing {filepath} ...", end=" ", flush=True)

            try:
                with asdf.open(filepath, lazy_load=False, memmap=False) as af:
                    wcs = af["roman"]["meta"]["wcs"]
                    try:
                        det = af["roman"]["meta"]["instrument"]["detector"]
                        detector = det.lower()
                    except KeyError:
                        detector = os.path.basename(filepath)
                        print(f"\n  WARNING: no roman/meta/instrument/detector key; "
                              f"using basename: {detector}", file=sys.stderr)

                # pixel_to_world uses 0-based GWCS convention
                sky = wcs.pixel_to_world(xin, yin)

                ra = sky.ra.deg
                dec = sky.dec.deg

                n_bad = 0
                for i in range(npts):
                    if not (math.isfinite(ra[i]) and math.isfinite(dec[i])):
                        n_bad += 1
                        print(f"  WARNING: bad transform at pixel ({xin[i]:.1f}, {yin[i]:.1f})",
                              file=sys.stderr)
                        continue
                    writer.writerow([filepath, detector,
                                     f"{xin[i]:.6f}", f"{yin[i]:.6f}",
                                     f"{ra[i]:.10f}", f"{dec[i]:.10f}"])

                print(f"OK ({npts - n_bad}/{npts} valid)")
                n_success += 1

            except Exception as exc:
                print(f"FAILED: {exc}")
                n_fail += 1

    print(f"\nSummary: {n_success}/{nfiles} files succeeded")
    print(f"Output written to {args.output}")
    return 0 if n_success > 0 else 1


if __name__ == "__main__":
    sys.exit(main())
