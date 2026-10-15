"""
This file contains code for generating many of the test files in this
repository.

The files are checked in to the repository for now as they as small, though
this could also be used if we want to generate some larger test files later.

It is stored here for reference, and requires the asdf Python library, gwcs,
numpy, etc.
"""

import argparse
import io
import sys
from pathlib import Path

import asdf
import numpy as np
from astropy import coordinates as coord
from astropy import units as u
from astropy.modeling import models
from gwcs import FITSImagingWCSTransform
from gwcs import coordinate_frames as cf
from gwcs import wcs as gwcs_wcs


FITS_ARRAYS = ('crpix', 'crval', 'cdelt', 'pc')
"""The arrays a fitswcs_imaging transform stores in ndarrays"""


def raw_fits_imaging_wcs():
    """
    Build a two-step FITS WCS gwcs and return it as a raw tagged tree.

    Returns the tree, its fitswcs_imaging transform node, and the values of
    that transform's arrays.  The caller stores arrays of its own into the
    node, which is to say under whatever datatype, byte order and shape it
    wants them written with.

    The arrays have to be substituted into the raw tagged tree because
    astropy's ``Parameter`` casts an assignment back to a native-order array
    of the parameter's own shape, so setting them on the transform itself
    cannot express any of these cases.
    """
    transform = FITSImagingWCSTransform(
        models.Pix2Sky_Gnomonic(),
        crpix=[12099.5, -88700.5],
        crval=[270.0, 64.60237301],
        cdelt=[1.52777778e-05, 1.52777778e-05],
        pc=[[1.0, 0.0], [-0.0, 1.0]])
    detector = cf.Frame2D(name='detector', axes_names=('x', 'y'),
                          unit=(u.pix, u.pix))
    sky = cf.CelestialFrame(name='icrs', reference_frame=coord.ICRS(),
                            unit=(u.deg, u.deg))
    wcs = gwcs_wcs.WCS([(detector, transform), (sky, None)])
    values = {name: np.asarray(getattr(transform, name).value)
              for name in FITS_ARRAYS}

    buf = io.BytesIO()
    asdf.AsdfFile({'wcs': wcs}).write_to(buf)
    buf.seek(0)

    with asdf.open(buf, _force_raw_types=True, lazy_load=False,
                   memmap=False) as af:
        tree = af.tree['wcs']

    return tree, tree['steps'][0]['transform'], values


def make_fitswcs_imaging_asdf():
    """
    Make an ASDF file holding a fitswcs_imaging WCS per array storage form

    Each WCS is identical apart from how its crpix/crval/cdelt/pc arrays are
    stored, so they can all be checked against the same expected values:

    ``lz4``
        The arrays in lz4-compressed blocks.
    ``big_endian``
        The arrays stored as big-endian float64.
    ``bad_shape``
        ``crval`` stored with a shape of ``[1]``, which a reader must reject.

    Note that ``bad_shape`` is invalid enough that the gwcs Python library
    raises on it, so this file can only be opened in Python with
    ``_force_raw_types=True``.
    """
    f = asdf.AsdfFile()

    f['lz4'], node, values = raw_fits_imaging_wcs()

    for name, value in values.items():
        node[name] = value
        f.set_array_compression(value, 'lz4')

    f['big_endian'], node, values = raw_fits_imaging_wcs()

    for name, value in values.items():
        node[name] = value.astype('>f8')

    f['bad_shape'], node, values = raw_fits_imaging_wcs()

    for name, value in values.items():
        node[name] = value

    node['crval'] = values['crval'][:1]

    return f


TEST_FILES = {
    'fitswcs_imaging.asdf': make_fitswcs_imaging_asdf,
}
"""Maps the names of test files to the function that generates it"""


def main(argv=None):
    parser = argparse.ArgumentParser(description='Generate test fixture files')
    parser.add_argument('filename', nargs='?', type=str,
                        help='The name of the test file to generate; if '
                             'omitted lists the names of the file this script '
                             'can generate')
    parser.add_argument('--fixtures-dir', type=Path,
                        default=Path('../fixtures'),
                        help='Path to the test fixtures directory')
    args = parser.parse_args(argv)

    if not args.filename:
        for filename in TEST_FILES:
            print(filename)

        return 0

    if args.filename not in TEST_FILES:
        parser.error(f'unknown test file: {args.filename}')

    asdf_file = TEST_FILES[args.filename]()
    asdf_file.write_to(args.fixtures_dir / args.filename)
    return 0


if __name__ == '__main__':
    sys.exit(main())
