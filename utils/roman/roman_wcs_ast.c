/*
 * roman_wcs_ast.c
 *
 * Read Roman Space Telescope WCS from inline ASDF files using AST YamlChan
 * and evaluate a 20x20 pixel grid, writing pixel->sky coordinates to CSV.
 *
 * Usage:
 *   roman_wcs_ast [-o output.csv] file1.asdf [file2.asdf ...]
 *
 * Compile (from repo root after building AST):
 *   gcc -I. -Isrc -DHAVE_CONFIG_H roman_wcs_ast.c \
 *       -L.libs $(./ast_link) -Wl,-rpath,.libs -o roman_wcs_ast
 *
 * Note:
 *   Like `roman_wcs_gwcs.py`, much of this is not RST-specific
 *   and could be easily refactored to test otherwise arbitrary
 *   ASDF files containing a WCS.  At the moment, however, this
 *   expects an input file produced by `prepare_roman_yaml.py`
 *   to work around some of the limitations AST has with reading
 *   ASDF files.
 *
 *   Later we will add some glue code in libasdf-gwcs for handling
 *   this shortcoming.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ast.h>

#define NGRID    20
#define IMAGE_NX 4088
#define IMAGE_NY 4088

/*
 * Scan the ASDF file for a top-level "detector: VALUE" line.
 * Copies the trimmed value into det_out (max det_len bytes incl. NUL).
 * Returns 1 on success, 0 if not found (det_out is left unchanged).
 */
static int read_detector_from_yaml(const char *filepath,
                                   char *det_out, size_t det_len) {
    char line[256];
    FILE *f = fopen(filepath, "r");
    if (!f)
        return 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "detector: ", 10) == 0) {
            char val[128];
            if (sscanf(line + 10, "%127s", val) == 1) {
                /* Strip surrounding quotes if present (ASDF may add them) */
                size_t vlen = strlen(val);
                char *start = val;
                if (vlen >= 2 && val[0] == '\'' && val[vlen - 1] == '\'') {
                    val[vlen - 1] = '\0';
                    start = val + 1;
                } else if (vlen >= 2 && val[0] == '"' && val[vlen - 1] == '"') {
                    val[vlen - 1] = '\0';
                    start = val + 1;
                }
                /* Lowercase for CSV output consistency */
                for (size_t idx = 0; start[idx]; idx++)
                    if (start[idx] >= 'A' && start[idx] <= 'Z')
                        start[idx] += 'a' - 'A';
                snprintf(det_out, det_len, "%s", start);
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

/*
 * Return the basename of path (pointer into path, no allocation).
 */
static const char *path_basename(const char *path) {
    const char *p = strrchr(path, '/');
    return p ? p + 1 : path;
}

int main(int argc, char *argv[]) {
    int npts = NGRID * NGRID;
    double xin[NGRID * NGRID], yin[NGRID * NGRID];
    double xout[NGRID * NGRID], yout[NGRID * NGRID];
    FILE *fout;
    int n_success = 0, n_fail = 0;
    const char *outpath = "ast_wcs_results.csv";
    int first_file_arg = 1;

    /* Parse -o option */
    if (argc >= 3 && strcmp(argv[1], "-o") == 0) {
        outpath = argv[2];
        first_file_arg = 3;
    }

    if (first_file_arg >= argc) {
        fprintf(stderr, "Usage: %s [-o output.csv] file1.asdf [file2.asdf ...]\n",
                argv[0]);
        return 1;
    }

    /* Open output CSV */
    fout = fopen(outpath, "w");
    if (!fout) {
        fprintf(stderr, "ERROR: Cannot open %s for writing\n", outpath);
        return 1;
    }
    fprintf(fout, "filename,detector,pixel_x,pixel_y,ra_deg,dec_deg\n");

    /* Build the 20x20 pixel grid (evenly spaced, ~100 to ~3988) */
    for (int iy = 0; iy < NGRID; iy++) {
        for (int ix = 0; ix < NGRID; ix++) {
            xin[iy * NGRID + ix] = 100.0 + (IMAGE_NX - 200.0) * ix / (NGRID - 1);
            yin[iy * NGRID + ix] = 100.0 + (IMAGE_NY - 200.0) * iy / (NGRID - 1);
        }
    }

    /* Loop over input files */
    for (int idx = first_file_arg; idx < argc; idx++) {
        const char *filepath = argv[idx];
        char detector[128];
        char attrs[600];
        AstYamlChan *chan;
        AstObject *obj;
        AstFrameSet *fs;
        AstMapping *map;
        int status_val = 0;
        int *status = &status_val;
        int nout_axes;

        /* Extract detector name from YAML */
        if (!read_detector_from_yaml(filepath, detector, sizeof(detector))) {
            fprintf(stderr, "  WARNING: no 'detector:' key in %s; using basename\n",
                    filepath);
            snprintf(detector, sizeof(detector), "%s", path_basename(filepath));
        }

        fprintf(stderr, "Processing %s (%s) ...\n", detector, filepath);

        /* Clear AST status before each file */
        astClearStatus;

        /* Create YamlChan with ASDF encoding and source file */
        snprintf(attrs, sizeof(attrs), "SourceFile=%s", filepath);
        chan = astYamlChan(NULL, NULL, attrs);

        if (!astOK) {
            fprintf(stderr, "  ERROR: astYamlChan failed for %s\n", detector);
            astClearStatus;
            n_fail++;
            continue;
        }

        /* Set YamlEncoding to ASDF */
        astSet(chan, "YamlEncoding=ASDF");

        if (!astOK) {
            fprintf(stderr, "  ERROR: astSet YamlEncoding failed for %s\n", detector);
            astClearStatus;
            astAnnul(chan);
            n_fail++;
            continue;
        }

        /* Read the WCS object */
        obj = astRead(chan);

        if (!astOK || obj == AST__NULL) {
            if (!astOK) {
                fprintf(stderr, "  ERROR: astRead failed for %s (AST status %d)\n",
                        detector, astStatus);
            } else {
                fprintf(stderr, "  ERROR: astRead returned NULL for %s\n", detector);
            }
            astClearStatus;
            if (chan) astAnnul(chan);
            n_fail++;
            continue;
        }

        /* Check it's a FrameSet */
        if (!astIsAFrameSet(obj)) {
            fprintf(stderr, "  ERROR: Read object is not a FrameSet for %s (class: %s)\n",
                    detector, astGetC(obj, "Class"));
            astAnnul(obj);
            astAnnul(chan);
            astClearStatus;
            n_fail++;
            continue;
        }

        fs = (AstFrameSet *) obj;

        /* Get the pixel->sky mapping (base frame -> current frame) */
        map = astGetMapping(fs, AST__BASE, AST__CURRENT);

        if (!astOK || map == AST__NULL) {
            fprintf(stderr, "  ERROR: astGetMapping failed for %s\n", detector);
            astClearStatus;
            astAnnul(fs);
            astAnnul(chan);
            n_fail++;
            continue;
        }

        /* Check number of output axes */
        nout_axes = astGetI(map, "Nout");
        if (nout_axes < 2) {
            fprintf(stderr, "  ERROR: mapping has only %d output axes for %s\n",
                    nout_axes, detector);
            astAnnul(map);
            astAnnul(fs);
            astAnnul(chan);
            astClearStatus;
            n_fail++;
            continue;
        }

        /* Transform pixel -> sky (forward direction) */
        astTran2(map, npts, xin, yin, 1, xout, yout);

        if (!astOK) {
            fprintf(stderr, "  ERROR: astTran2 failed for %s\n", detector);
            astClearStatus;
            astAnnul(map);
            astAnnul(fs);
            astAnnul(chan);
            n_fail++;
            continue;
        }

        /* Convert from radians to degrees and write CSV */
        for (int jdx = 0; jdx < npts; jdx++) {
            double ra_deg  = xout[jdx] * (180.0 / M_PI);
            double dec_deg = yout[jdx] * (180.0 / M_PI);

            if (xout[jdx] == AST__BAD || yout[jdx] == AST__BAD ||
                ra_deg != ra_deg || dec_deg != dec_deg) {
                fprintf(stderr, "  WARNING: bad transform at pixel (%.1f, %.1f) for %s\n",
                        xin[jdx], yin[jdx], detector);
                continue;
            }

            fprintf(fout, "%s,%s,%.6f,%.6f,%.10f,%.10f\n",
                    filepath, detector,
                    xin[jdx], yin[jdx], ra_deg, dec_deg);
        }

        fprintf(stderr, "  OK: wrote %d sky coords for %s\n", npts, detector);
        n_success++;

        astAnnul(map);
        astAnnul(fs);
        astAnnul(chan);
    }

    fclose(fout);

    int nfiles = argc - first_file_arg;
    fprintf(stderr, "\nSummary: %d/%d files succeeded\n", n_success, nfiles);
    fprintf(stderr, "Output written to %s\n", outpath);

    return (n_success == 0) ? 1 : 0;
}
