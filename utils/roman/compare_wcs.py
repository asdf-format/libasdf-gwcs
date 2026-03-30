#!/usr/bin/env python3
"""
compare_wcs.py

Statistically compare AST YamlChan WCS results against Python GWCS results.

Inputs:  ast_wcs_results.csv   (from roman_wcs_ast C program)
         gwcs_wcs_results.csv  (from roman_wcs_gwcs.py)

Output:  wcs_comparison_summary.txt  (per-detector and global statistics)
         Printed summary to stdout.
"""

import sys
import argparse
import csv
import numpy as np


def load_csv(path):
    """Load a results CSV; return (detectors, pixel_x, pixel_y, ra, dec) as numpy arrays"""
    detectors, pixel_x, pixel_y, ra, dec = [], [], [], [], []
    with open(path, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            detectors.append(row['detector'])
            pixel_x.append(float(row['pixel_x']))
            pixel_y.append(float(row['pixel_y']))
            ra.append(float(row['ra_deg']))
            dec.append(float(row['dec_deg']))
    return (np.array(detectors),
            np.array(pixel_x), np.array(pixel_y),
            np.array(ra),      np.array(dec))


def angular_separation_arcsec(ra1, dec1, ra2, dec2):
    """
    Vectorized great-circle angular separation in arcseconds (Vincenty formula)

    All inputs in degrees; works on scalars or numpy arrays.
    Uses the numerically stable Vincenty formula for a unit sphere (no
    ellipsoidal flattening -- appropriate for celestial coordinates).
    """
    ra1,  dec1 = np.radians(ra1),  np.radians(dec1)
    ra2,  dec2 = np.radians(ra2),  np.radians(dec2)
    dlon = ra2 - ra1
    x = np.sin(dlon) * np.cos(dec2)
    y = np.cos(dec1) * np.sin(dec2) - np.sin(dec1) * np.cos(dec2) * np.cos(dlon)
    num = np.sqrt(x*x + y*y)
    den = np.sin(dec1) * np.sin(dec2) + np.cos(dec1) * np.cos(dec2) * np.cos(dlon)
    return np.degrees(np.arctan2(num, den)) * 3600.0


def main():
    parser = argparse.ArgumentParser(
        description="Compare AST vs GWCS WCS CSV results")
    parser.add_argument("ast_csv", metavar="ast_results.csv")
    parser.add_argument("gwcs_csv", metavar="gwcs_results.csv")
    parser.add_argument("-o", "--output", default="wcs_comparison_summary.txt",
                        metavar="output.txt")
    args = parser.parse_args()

    ast_file = args.ast_csv
    gwcs_file = args.gwcs_csv
    out_file = args.output

    print(f'Loading {ast_file} ...')
    ast_det, ast_px, ast_py, ast_ra, ast_dec = load_csv(ast_file)
    print(f'  {len(ast_det)} rows')

    print(f'Loading {gwcs_file} ...')
    gwcs_det, gwcs_px, gwcs_py, gwcs_ra, gwcs_dec = load_csv(gwcs_file)
    print(f'  {len(gwcs_det)} rows')

    # Build string keys 'det:px:py' for set-based matching
    def make_keys(det, px, py):
        return np.array([f'{d}:{x:.6f}:{y:.6f}' for d, x, y in zip(det, px, py)])

    ast_keys = make_keys(ast_det, ast_px, ast_py)
    gwcs_keys = make_keys(gwcs_det, gwcs_px, gwcs_py)

    ast_key_set = set(ast_keys)
    gwcs_key_set = set(gwcs_keys)
    common = ast_key_set & gwcs_key_set
    ast_only = ast_key_set - gwcs_key_set
    gwcs_only = gwcs_key_set - ast_key_set

    print(f'\n  Matched: {len(common)} points')
    if ast_only:
        print(f'  AST-only (no gwcs match): {len(ast_only)}')
    if gwcs_only:
        print(f'  GWCS-only (no ast match): {len(gwcs_only)}')

    # Build a lookup from key -> gwcs row index, then gather matched pairs
    gwcs_index = {k: i for i, k in enumerate(gwcs_keys)}
    mask = np.isin(ast_keys, list(common))
    ast_idx = np.where(mask)[0]
    gwcs_idx = np.array([gwcs_index[k] for k in ast_keys[mask]])

    matched_det = ast_det[ast_idx]
    seps = angular_separation_arcsec(
        ast_ra[ast_idx],   ast_dec[ast_idx],
        gwcs_ra[gwcs_idx], gwcs_dec[gwcs_idx],
    )

    # Build output table
    lines = []
    lines.append('=' * 72)
    lines.append('AST YamlChan vs Python GWCS -- WCS Comparison Summary')
    lines.append('=' * 72)
    lines.append(f'{"Detector":<10} {"N":>5} {"Mean(″)":>10} {"Std(″)":>10} {"Med(″)":>10} {"Max(″)":>10}')
    lines.append('-' * 72)

    for det in sorted(set(matched_det)):
        s = seps[matched_det == det]
        lines.append(f'{det:<10} {len(s):>5} {s.mean():>10.4f} {s.std():>10.4f} '
                     f'{np.median(s):>10.4f} {s.max():>10.4f}')

    lines.append('-' * 72)
    lines.append(f'{"ALL":<10} {len(seps):>5} {seps.mean():>10.4f} {seps.std():>10.4f} '
                 f'{np.median(seps):>10.4f} {seps.max():>10.4f}')
    lines.append('=' * 72)
    lines.append('')
    lines.append('Units: arcseconds')
    # Roman WFI pixel scale: Spergel et al. 2015 (arXiv:1503.03757), Table 3-1;
    # also https://roman.gsfc.nasa.gov/science/WFI_technical.html
    lines.append('Roman WFI pixel scale: ~0.11 arcsec/pix  (Spergel et al. 2015, arXiv:1503.03757)')
    lines.append('')

    max_sep = seps.max()
    if max_sep < 0.001:
        lines.append('Assessment: EXCELLENT -- max separation < 1 mas')
    elif max_sep < 0.01:
        lines.append(f'Assessment: VERY GOOD -- max separation {max_sep*1000:.2f} mas')
    elif max_sep < 0.1:
        lines.append(f'Assessment: GOOD -- max separation {max_sep:.4f} arcsec')
    elif max_sep < 1.0:
        lines.append(f'Assessment: MARGINAL -- max separation {max_sep:.4f} arcsec (investigate)')
    else:
        lines.append(
            f'Assessment: POOR -- max separation {max_sep:.3f} arcsec (significant discrepancy)'
        )

    output = '\n'.join(lines)
    print('\n' + output)

    with open(out_file, 'w') as f:
        f.write(output + '\n')
    print(f'\nSummary written to {out_file}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
