# Build 21 WCS conformance test results

This directory contains results from running the conformance tests on Build 21
of the Roman L2 calibration files; one for each of 18 detectors.  This does not
contain the ASDF files themselves which would be too large to maintain in git,
nor the intermediate files produced by the conformance test.

It does however contain the result files from running the tests:

- `ast_wcs_results.csv` contains the results from running the WCS tests
  against AST: each line lists the file, the detector, and demonstrates
  a sampling of pixel coordinates and their transformed world coordinates
  as output by AST.

- `gwcs_wcs_results.csv` is in the same format, with the same coordinate
  transforms as output by GWCS.

- `wcs_comparison_summary.txt` is the output of the `compare_wcs.py` script,
  and simply shows a summary of the differences between the two libraries.
  In this case any difference was completely negligible.  The "EXCELLENT"
  assessment means that if there were any difference between the two libraries
  at all it is less than 1 milliarcsec, well below the Roman WFI pixel
  resolution of 0.11 arcsec/pix.

The only visible difference between the two outputs is latitude wrapping
convention, with GWCS defaulting to wrapping at 360deg, and AST defaulting
to 180deg, but this trivial coordinate difference is accounted for in the
comparison analysis.
