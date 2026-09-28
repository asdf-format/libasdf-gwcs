#include <glob.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <asdf.h>
#include <asdf/gwcs/backend.h>
#include <asdf/gwcs/eval.h>
#include <asdf/gwcs/grid.h>
#include <asdf/gwcs/gwcs.h>

#include "config.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include "munit.h"
#include "util.h"


typedef struct {
    char   detector[32];
    double pixel_x, pixel_y;
    double ra_deg, dec_deg;
} wcs_ref_row_t;


/** Load reference WCS transform results from CSV */
static wcs_ref_row_t *wcs_ref_load(const char *path, size_t *n_out) {
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;

    size_t cap = 512, n = 0;
    wcs_ref_row_t *rows = malloc(cap * sizeof(*rows));
    if (!rows) {
        fclose(f);
        return NULL;
    }

    char line[256];
    if (!fgets(line, sizeof(line), f)) {  /* skip header */
        fclose(f);
        free(rows);
        return NULL;
    }

    while (fgets(line, sizeof(line), f)) {
        if (n == cap) {
            cap *= 2;
            wcs_ref_row_t *tmp = realloc(rows, cap * sizeof(*rows));
            if (!tmp) {
                fclose(f);
                free(rows);
                return NULL;
            }
            rows = tmp;
        }
        /* Format: filename,detector,pixel_x,pixel_y,ra_deg,dec_deg */
        char det[32];
        double px, py, ra, dec;
        if (sscanf(line, "%*[^,],%31[^,],%lf,%lf,%lf,%lf",
                   det, &px, &py, &ra, &dec) == 5) {
            memcpy(rows[n].detector, det, 32);
            rows[n].pixel_x = px;
            rows[n].pixel_y = py;
            rows[n].ra_deg  = ra;
            rows[n].dec_deg = dec;
            n++;
        }
    }
    fclose(f);
    *n_out = n;
    return rows;
}


static void wcs_ref_free(wcs_ref_row_t *rows) {
    free(rows);
}


static int wcs_ref_find_detector(
        const wcs_ref_row_t *rows, size_t n, const char *det) {
    for (size_t idx = 0; idx < n; idx++) {
        if (strcmp(rows[idx].detector, det) == 0)
            return (int)idx;
    }
    return -1;
}


static const char *detector_from_filename(const char *path) {
    static char det[32];
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    const char *p = base;
    while (*p) {
        if (strncmp(p, "_wfi", 4) == 0) {
            const char *start = p + 1;
            const char *end = strchr(start, '_');
            if (!end)
                end = strchr(start, '.');
            if (!end)
                end = start + strlen(start);
            size_t len = (size_t)(end - start);
            if (len < sizeof(det)) {
                memcpy(det, start, len);
                det[len] = '\0';
                return det;
            }
        }
        p++;
    }
    return NULL;
}


static double angular_sep_arcsec(
        double ra1, double dec1, double ra2, double dec2) {
    static const double DEG2RAD = M_PI / 180.0;
    double r1 = ra1 * DEG2RAD, d1 = dec1 * DEG2RAD;
    double r2 = ra2 * DEG2RAD, d2 = dec2 * DEG2RAD;
    double dlon = r2 - r1;
    double x = sin(dlon) * cos(d2);
    double y = cos(d1) * sin(d2) - sin(d1) * cos(d2) * cos(dlon);
    double num = sqrt(x * x + y * y);
    double den = sin(d1) * sin(d2) + cos(d1) * cos(d2) * cos(dlon);
    return atan2(num, den) * (180.0 / M_PI) * 3600.0;
}


/* Pixel grid matching roman_wcs_ast.c */
#define NGRID           20
#define IMAGE_NX        4088
#define IMAGE_NY        4088
#define NPTS            (NGRID * NGRID)

/* 1 mas -- EXCELLENT threshold same as the Roman benchmarks */
#define MAX_SEP_ARCSEC  0.001


MU_TEST(test_asdf_gwcs_grid2d) {
    /* 3×2 sampled grid: x in [1,5], y in [2,4] */
    asdf_gwcs_grid2d_t g = { .x0=1.0, .y0=2.0, .x1=5.0, .y1=4.0, .nx=3, .ny=2 };

    /* caller-allocated path */
    double xs[6], ys[6];
    double *xsp = xs, *ysp = ys;
    assert_int(asdf_gwcs_grid2d_fill(&g, &xsp, &ysp), ==, ASDF_GWCS_OK);
    assert_ptr(xsp, ==, xs);
    /* row 0: y=2, x=1,3,5 */
    assert_double(xs[0], ==, 1.0);
    assert_double(ys[0], ==, 2.0);
    assert_double(xs[1], ==, 3.0);
    assert_double(ys[1], ==, 2.0);
    assert_double(xs[2], ==, 5.0);
    assert_double(ys[2], ==, 2.0);
    /* row 1: y=4, x=1,3,5 */
    assert_double(xs[3], ==, 1.0);
    assert_double(ys[3], ==, 4.0);
    assert_double(xs[4], ==, 3.0);
    assert_double(ys[4], ==, 4.0);
    assert_double(xs[5], ==, 5.0);
    assert_double(ys[5], ==, 4.0);

    /* allocating path */
    double *xa = NULL, *ya = NULL;
    assert_int(asdf_gwcs_grid2d_fill(&g, &xa, &ya), ==, ASDF_GWCS_OK);
    assert_not_null(xa);
    assert_not_null(ya);
    assert_double(xa[0], ==, 1.0);
    assert_double(ya[0], ==, 2.0);
    assert_double(xa[5], ==, 5.0);
    assert_double(ya[5], ==, 4.0);
    free(xa);
    free(ya);

    /* dense 4×4 from (0,0): x1=3, y1=3 */
    asdf_gwcs_grid2d_t d = { .x0=0.0, .y0=0.0, .x1=3.0, .y1=3.0, .nx=4, .ny=4 };
    double dx[16], dy[16];
    double *dxp = dx, *dyp = dy;
    assert_int(asdf_gwcs_grid2d_fill(&d, &dxp, &dyp), ==, ASDF_GWCS_OK);
    assert_double(dx[0],  ==, 0.0);
    assert_double(dy[0],  ==, 0.0);
    assert_double(dx[5],  ==, 1.0);
    assert_double(dy[5],  ==, 1.0);
    assert_double(dx[15], ==, 3.0);
    assert_double(dy[15], ==, 3.0);

    return MUNIT_OK;
}


MU_TEST(test_asdf_gwcs_backend_get_nonexistent) {
    const asdf_gwcs_backend_t *b = asdf_gwcs_backend_get("nonexistent_backend");
    assert_null(b);
    return MUNIT_OK;
}


MU_TEST(test_asdf_gwcs_backend_get_ast_yaml) {
    const asdf_gwcs_backend_t *b = asdf_gwcs_backend_get("ast_yaml");
    if (!b)
        return MUNIT_SKIP;
    assert_string_equal(b->name, "ast_yaml");
    assert_not_null(b->pipeline);
    assert_not_null(b->pipeline->create);
    return MUNIT_OK;
}


MU_TEST(test_asdf_gwcs_eval_2d_roman_l2) {
    const asdf_gwcs_backend_t *backend = asdf_gwcs_backend_get("ast_yaml");
    if (!backend)
        return MUNIT_SKIP;

    const char *path = get_fixture_file_path("roman_l2_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_t *wcs = NULL;
    assert_int(asdf_get_gwcs(file, "roman/meta/wcs", &wcs), ==, ASDF_VALUE_OK);
    assert_not_null(wcs);

    asdf_gwcs_err_t err = ASDF_GWCS_OK;
    asdf_gwcs_eval_t *eval = asdf_gwcs_eval_create(file, wcs, backend, &err);
    assert_int(err, ==, ASDF_GWCS_OK);
    assert_not_null(eval);

    /* 4×4 pixel grid */
    static const double xin[4] = {0.0, 1.0, 2.0, 3.0};
    static const double yin[4] = {0.0, 1.0, 2.0, 3.0};
    double xout[4], yout[4];

    assert_int(asdf_gwcs_eval_2d(eval, xin, yin, xout, yout, 4),
               ==, ASDF_GWCS_OK);

    /* Sanity check: sky coordinates should be finite */
    for (size_t idx = 0; idx < 4; idx++) {
        assert_true(isfinite(xout[idx]));
        assert_true(isfinite(yout[idx]));
    }

    asdf_gwcs_eval_destroy(eval);
    asdf_gwcs_destroy(wcs);
    asdf_close(file);
    return MUNIT_OK;
}


/**
 * This test basically re-implements the compare_wcs.py script from the
 * exploratory Roman WCS Conformance Tests under `util/roman`. It compares
 * results of running the libasdf-gwcs evaluation code against the reference
 * results from those tests, both against AST (which ought to match since
 * the code in this library uses AST for now, and is effectively the same
 * as the reference tests) and against results generated by GWCS in Python.
 *
 * Load the reference CSV, run the 20×20 pixel grid against every Roman
 * build21 fixture, and assert max angular separation < MAX_SEP_ARCSEC.
 */
static MunitResult run_build21_comparison(const char *csv_relpath) {
    const asdf_gwcs_backend_t *backend = asdf_gwcs_backend_get("ast_yaml");
    if (!backend)
        return MUNIT_SKIP;

    const char *csv_path = get_fixture_file_path(csv_relpath);
    size_t n_ref = 0;
    wcs_ref_row_t *ref = wcs_ref_load(csv_path, &n_ref);
    if (!ref)
        return MUNIT_SKIP;

    /* 20×20 pixel grid matching roman_wcs_ast.c */
    asdf_gwcs_grid2d_t grid = {
        .x0 = 100.0, .y0 = 100.0,
        .x1 = 100.0 + (IMAGE_NX - 200.0),
        .y1 = 100.0 + (IMAGE_NY - 200.0),
        .nx = NGRID, .ny = NGRID
    };

    char pattern[PATH_MAX];
    snprintf(pattern, sizeof(pattern), "%s/roman/build21/*.asdf", FIXTURES_DIR);
    glob_t gl;
    if (glob(pattern, 0, NULL, &gl) != 0 || gl.gl_pathc == 0) {
        globfree(&gl);
        wcs_ref_free(ref);
        return MUNIT_SKIP;
    }

    double max_sep = 0.0;
    MunitResult result = MUNIT_OK;

    for (size_t fidx = 0; fidx < gl.gl_pathc && result == MUNIT_OK; fidx++) {
        const char *fpath = gl.gl_pathv[fidx];
        const char *det = detector_from_filename(fpath);
        if (!det)
            continue;

        int ref_start = wcs_ref_find_detector(ref, n_ref, det);
        if (ref_start < 0)
            continue;

        asdf_file_t *file = asdf_open(fpath, "r");
        if (!file) {
            result = MUNIT_FAIL;
            break;
        }

        asdf_gwcs_t *wcs = NULL;
        if (asdf_get_gwcs(file, "roman/meta/wcs", &wcs) != ASDF_VALUE_OK || !wcs) {
            asdf_close(file);
            result = MUNIT_FAIL;
            break;
        }

        asdf_gwcs_err_t err = ASDF_GWCS_OK;
        asdf_gwcs_eval_t *eval = asdf_gwcs_eval_create(file, wcs, backend, &err);
        if (!eval || err != ASDF_GWCS_OK) {
            asdf_gwcs_destroy(wcs);
            asdf_close(file);
            result = MUNIT_FAIL;
            break;
        }

        double *xout = NULL, *yout = NULL;
        if (asdf_gwcs_eval_grid2d(eval, &grid, &xout, &yout) != ASDF_GWCS_OK) {
            asdf_gwcs_eval_destroy(eval);
            asdf_gwcs_destroy(wcs);
            asdf_close(file);
            result = MUNIT_FAIL;
            break;
        }

        for (size_t jdx = 0; jdx < NPTS; jdx++) {
            const wcs_ref_row_t *row = &ref[(size_t)ref_start + jdx];
            double sep = angular_sep_arcsec(xout[jdx], yout[jdx],
                                            row->ra_deg, row->dec_deg);
            if (sep > max_sep)
                max_sep = sep;
        }

        free(xout);
        free(yout);
        asdf_gwcs_eval_destroy(eval);
        asdf_gwcs_destroy(wcs);
        asdf_close(file);
    }

    globfree(&gl);
    wcs_ref_free(ref);

    if (result == MUNIT_OK && max_sep >= MAX_SEP_ARCSEC)
        return MUNIT_FAIL;

    return result;
}


MU_TEST(test_asdf_gwcs_eval_roman_build21_vs_ast) {
    return run_build21_comparison("roman/build21/ast_wcs_results.csv");
}


MU_TEST(test_asdf_gwcs_eval_roman_build21_vs_gwcs) {
    return run_build21_comparison("roman/build21/gwcs_wcs_results.csv");
}


/* rotate3d
 *
 * This transform didn't appear in any of the reference files so add explicit
 * tests for it
 */

/**
 * Build a two-step WCS wrapping a single transform and evaluate it
 *
 * Both frames are plain 2-D frames holding longitude/latitude in degrees, so
 * the values that come back are the transform's own outputs with no unit
 * conversion applied (`ast_eval_2d` converts from radians only for a
 * SkyFrame output).
 */
static asdf_gwcs_err_t eval_lonlat_transform(
        const asdf_gwcs_transform_t *transform,
        const double *xin, const double *yin,
        double *xout, double *yout, size_t n) {
    const asdf_gwcs_backend_t *backend = asdf_gwcs_backend_get("ast_yaml");

    asdf_gwcs_frame2d_t in_frame = {
        .base = {.type = ASDF_GWCS_FRAME_2D, .name = "native"},
        .axes_names = {"lon", "lat"},
        .axes_order = {0, 1},
        .axis_physical_types = {"pos.eq.ra", "pos.eq.dec"},
    };
    asdf_gwcs_frame2d_t out_frame = {
        .base = {.type = ASDF_GWCS_FRAME_2D, .name = "celestial"},
        .axes_names = {"lon", "lat"},
        .axes_order = {0, 1},
        .axis_physical_types = {"pos.eq.ra", "pos.eq.dec"},
    };
    asdf_gwcs_step_t steps[2] = {
        {.frame = (asdf_gwcs_frame_t *)&in_frame, .transform = transform},
        {.frame = (asdf_gwcs_frame_t *)&out_frame, .transform = NULL},
    };
    asdf_gwcs_t wcs = {.name = "rotate3d_test", .n_steps = 2, .steps = steps};

    asdf_file_t *file = asdf_open(NULL);
    assert_not_null(file);

    asdf_gwcs_err_t err = ASDF_GWCS_OK;
    asdf_gwcs_eval_t *eval = asdf_gwcs_eval_create(file, &wcs, backend, &err);

    if (eval) {
        err = asdf_gwcs_eval_2d(eval, xin, yin, xout, yout, n);
        asdf_gwcs_eval_destroy(eval);
    }

    asdf_close(file);
    return err;
}


/**
 * Under native2celestial, phi and theta are by definition the celestial
 * coordinates of the native pole, so the native pole must map to them.
 */
MU_TEST(test_asdf_gwcs_eval_rotate3d_native_pole) {
    if (!asdf_gwcs_backend_get("ast_yaml"))
        return MUNIT_SKIP;

    static const double phi = 270.0, theta = 66.0, psi = 180.0;
    asdf_gwcs_rotate3d_t rot = {
        .base = {.type = ASDF_GWCS_TRANSFORM_ROTATE3D},
        .phi = phi,
        .theta = theta,
        .psi = psi,
        .direction = "native2celestial",
    };

    double xin = 0.0, yin = 90.0;
    double xout = 0.0, yout = 0.0;

    assert_int(eval_lonlat_transform((const asdf_gwcs_transform_t *)&rot,
        &xin, &yin, &xout, &yout, 1), ==, ASDF_GWCS_OK);
    assert_double(angular_sep_arcsec(xout, yout, phi, theta), <, MAX_SEP_ARCSEC);
    return MUNIT_OK;
}


/**
 * celestial2native inverts native2celestial, so composing the two with the
 * same angles is the identity.
 */
MU_TEST(test_asdf_gwcs_eval_rotate3d_round_trip) {
    if (!asdf_gwcs_backend_get("ast_yaml"))
        return MUNIT_SKIP;

    asdf_gwcs_rotate3d_t native2celestial = {
        .base = {.type = ASDF_GWCS_TRANSFORM_ROTATE3D},
        .phi = 270.0,
        .theta = 66.0,
        .psi = 180.0,
        .direction = "native2celestial",
    };
    asdf_gwcs_rotate3d_t celestial2native = native2celestial;
    celestial2native.direction = "celestial2native";

    asdf_gwcs_transform_t *forward[2] = {
        (asdf_gwcs_transform_t *)&native2celestial,
        (asdf_gwcs_transform_t *)&celestial2native,
    };
    asdf_gwcs_compose_t compose = {
        .base = {.type = ASDF_GWCS_TRANSFORM_COMPOSE},
        .n_forward = 2,
        .forward = forward,
    };

    static const double xin[4] = {0.0, 30.0, 123.4, -45.0};
    static const double yin[4] = {0.0, 10.0, -60.0, 80.0};
    double xout[4], yout[4];

    assert_int(eval_lonlat_transform((const asdf_gwcs_transform_t *)&compose,
        xin, yin, xout, yout, 4), ==, ASDF_GWCS_OK);

    for (size_t idx = 0; idx < 4; idx++)
        assert_double(angular_sep_arcsec(xout[idx], yout[idx], xin[idx], yin[idx]),
            <, MAX_SEP_ARCSEC);

    return MUNIT_OK;
}


/* asdf_gwcs_eval_copy
 *
 * An eval context belongs to the thread that created it; copying is how one
 * WCS gets evaluated from a worker pool.
 */

#define COPY_N_POINTS 64
#define COPY_N_THREADS 8
#define COPY_N_ITERS 500


/** A fixture WCS and its eval context, held open together */
typedef struct {
    asdf_file_t *file;
    asdf_gwcs_t *wcs;
    asdf_gwcs_eval_t *eval;
} copy_fixture_t;


/**
 * Open the simplest GWCS fixture and create an eval context for it
 *
 * Returns false when no evaluation backend is registered, so the caller can
 * skip.
 */
static bool copy_fixture_open(copy_fixture_t *fx) {
    const asdf_gwcs_backend_t *backend = asdf_gwcs_backend_get("ast_yaml");

    if (!backend)
        return false;

    *fx = (copy_fixture_t){0};
    fx->file = asdf_open(get_fixture_file_path("roman_l3_wcs.asdf"), "r");
    assert_not_null(fx->file);

    assert_int(asdf_get_gwcs(fx->file, "wcs", &fx->wcs), ==, ASDF_VALUE_OK);
    assert_not_null(fx->wcs);

    asdf_gwcs_err_t err = ASDF_GWCS_OK;
    fx->eval = asdf_gwcs_eval_create(fx->file, fx->wcs, backend, &err);
    assert_int(err, ==, ASDF_GWCS_OK);
    assert_not_null(fx->eval);

    return true;
}


static void copy_fixture_close(copy_fixture_t *fx) {
    asdf_gwcs_eval_destroy(fx->eval);
    asdf_gwcs_destroy(fx->wcs);
    asdf_close(fx->file);
}


/** Fill the input arrays with a spread of pixel positions, some out of bounds */
static void copy_inputs(double *xin, double *yin) {
    for (size_t idx = 0; idx < COPY_N_POINTS; idx++) {
        xin[idx] = -100.0 + (double)idx * 97.0;
        yin[idx] = 5100.0 - (double)idx * 83.0;
    }
}


/**
 * Evaluate on *eval* and assert the results are bit-identical to the reference
 *
 * Bitwise rather than approximate: a copy is a copy, and anything short of
 * identical means the copied AST objects are not the same transform.
 */
static void assert_eval_matches(
    asdf_gwcs_eval_t *eval, const double *xref, const double *yref) {

    double xin[COPY_N_POINTS], yin[COPY_N_POINTS];
    double xout[COPY_N_POINTS], yout[COPY_N_POINTS];

    copy_inputs(xin, yin);
    assert_int(asdf_gwcs_eval_2d(eval, xin, yin, xout, yout, COPY_N_POINTS),
               ==, ASDF_GWCS_OK);
    assert_memory_equal(sizeof(xout), xout, xref);
    assert_memory_equal(sizeof(yout), yout, yref);
}


/** Reference results for the fixture, evaluated on *eval* itself */
static void copy_reference(asdf_gwcs_eval_t *eval, double *xref, double *yref) {
    double xin[COPY_N_POINTS], yin[COPY_N_POINTS];

    copy_inputs(xin, yin);
    assert_int(asdf_gwcs_eval_2d(eval, xin, yin, xref, yref, COPY_N_POINTS),
               ==, ASDF_GWCS_OK);
}


MU_TEST(test_asdf_gwcs_eval_copy) {
    copy_fixture_t fx;
    double xref[COPY_N_POINTS];
    double yref[COPY_N_POINTS];

    if (!copy_fixture_open(&fx))
        return MUNIT_SKIP;

    copy_reference(fx.eval, xref, yref);

    asdf_gwcs_err_t err = ASDF_GWCS_ERR_OOM;
    asdf_gwcs_eval_t *copy = asdf_gwcs_eval_copy(fx.eval, &err);
    assert_int(err, ==, ASDF_GWCS_OK);
    assert_not_null(copy);
    assert_ptr_not_equal(copy, fx.eval);
    assert_eval_matches(copy, xref, yref);

    /* A copy of a copy is just as good, and the original stays usable */
    asdf_gwcs_eval_t *copy2 = asdf_gwcs_eval_copy(copy, &err);
    assert_int(err, ==, ASDF_GWCS_OK);
    assert_not_null(copy2);
    assert_eval_matches(copy2, xref, yref);
    assert_eval_matches(fx.eval, xref, yref);

    /* Copies are independent of their source in either direction */
    asdf_gwcs_eval_destroy(copy);
    assert_eval_matches(copy2, xref, yref);
    copy_fixture_close(&fx);
    assert_eval_matches(copy2, xref, yref);
    asdf_gwcs_eval_destroy(copy2);

    return MUNIT_OK;
}


MU_TEST(test_asdf_gwcs_eval_copy_invalid) {
    asdf_gwcs_err_t err = ASDF_GWCS_OK;

    assert_null(asdf_gwcs_eval_copy(NULL, &err));
    assert_int(err, ==, ASDF_GWCS_ERR_INVAL);

    /* err_out is optional */
    assert_null(asdf_gwcs_eval_copy(NULL, NULL));
    return MUNIT_OK;
}


#ifdef HAVE_PTHREAD
/* Utilities for the multi-threaded copy tests */

/**
 * Release all worker threads at once
 *
 * pthread_barrier_t is not available on macOS, and overlapping the copies is
 * the entire point of the test, so instead block all threads on a condition
 * variable until ready.
 */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool open;
} gate_t;


static void gate_init(gate_t *gate) {
    pthread_mutex_init(&gate->mutex, NULL);
    pthread_cond_init(&gate->cond, NULL);
    gate->open = false;
}


static void gate_destroy(gate_t *gate) {
    pthread_mutex_destroy(&gate->mutex);
    pthread_cond_destroy(&gate->cond);
}


static void gate_wait(gate_t *gate) {
    pthread_mutex_lock(&gate->mutex);

    while (!gate->open)
        pthread_cond_wait(&gate->cond, &gate->mutex);

    pthread_mutex_unlock(&gate->mutex);
}


static void gate_open(gate_t *gate) {
    pthread_mutex_lock(&gate->mutex);
    gate->open = true;
    pthread_cond_broadcast(&gate->cond);
    pthread_mutex_unlock(&gate->mutex);
}


typedef struct {
    gate_t *gate;
    asdf_gwcs_eval_t *shared;
    const double *xref;
    const double *yref;
    /** evaluations that returned an error or the wrong answer */
    size_t failures;
} worker_t;


/** Entrypoint for worker threads that copy an asdf_gwcs_eval_t context */
static void *copy_worker(void *arg) {
    worker_t *w = arg;
    asdf_gwcs_err_t err = ASDF_GWCS_OK;
    double xin[COPY_N_POINTS];
    double yin[COPY_N_POINTS];
    double xout[COPY_N_POINTS];
    double yout[COPY_N_POINTS];

    copy_inputs(xin, yin);

    /* Copy only once every worker is ready, so the copies really do overlap */
    gate_wait(w->gate);
    asdf_gwcs_eval_t *eval = asdf_gwcs_eval_copy(w->shared, &err);

    if (!eval || err != ASDF_GWCS_OK) {
        w->failures++;
        return NULL;
    }

    for (size_t idx = 0; idx < COPY_N_ITERS; idx++) {
        if (ASDF_GWCS_OK !=
            asdf_gwcs_eval_2d(eval, xin, yin, xout, yout, COPY_N_POINTS)) {
            w->failures++;
            continue;
        }

        if (0 != memcmp(xout, w->xref, sizeof(xout)) ||
            0 != memcmp(yout, w->yref, sizeof(yout)))
            w->failures++;
    }

    asdf_gwcs_eval_destroy(eval);
    return NULL;
}


/** Run COPY_N_THREADS workers against *shared* and assert none failed */
static void run_workers(asdf_gwcs_eval_t *shared, const double *xref,
                        const double *yref) {

    pthread_t threads[COPY_N_THREADS];
    worker_t workers[COPY_N_THREADS];
    gate_t gate;

    gate_init(&gate);

    for (size_t idx = 0; idx < COPY_N_THREADS; idx++) {
        workers[idx] = (worker_t){
            .gate = &gate,
            .shared = shared,
            .xref = xref,
            .yref = yref,
        };
        assert_int(pthread_create(&threads[idx], NULL, copy_worker,
                                  &workers[idx]), ==, 0);
    }

    /* All workers are started, now unblock them and wait for results */
    gate_open(&gate);

    for (size_t idx = 0; idx < COPY_N_THREADS; idx++)
        assert_int(pthread_join(threads[idx], NULL), ==, 0);

    for (size_t idx = 0; idx < COPY_N_THREADS; idx++)
        assert_size(workers[idx].failures, ==, 0);

    gate_destroy(&gate);
}


MU_TEST(test_asdf_gwcs_eval_copy_threaded) {
    copy_fixture_t fx;
    double xref[COPY_N_POINTS], yref[COPY_N_POINTS];

    if (!copy_fixture_open(&fx))
        return MUNIT_SKIP;

    copy_reference(fx.eval, xref, yref);
    run_workers(fx.eval, xref, yref);

    /* The original survives having been borrowed by every worker */
    assert_eval_matches(fx.eval, xref, yref);
    copy_fixture_close(&fx);

    return MUNIT_OK;
}

#else /* !HAVE_PTHREAD */

MU_TEST(test_asdf_gwcs_eval_copy_threaded) {
    return MUNIT_SKIP;
}

#endif /* HAVE_PTHREAD */


MU_TEST_SUITE(
    gwcs_eval,
    MU_RUN_TEST(test_asdf_gwcs_grid2d),
    MU_RUN_TEST(test_asdf_gwcs_backend_get_nonexistent),
    MU_RUN_TEST(test_asdf_gwcs_backend_get_ast_yaml),
    MU_RUN_TEST(test_asdf_gwcs_eval_2d_roman_l2),
    MU_RUN_TEST(test_asdf_gwcs_eval_roman_build21_vs_ast),
    MU_RUN_TEST(test_asdf_gwcs_eval_roman_build21_vs_gwcs),
    MU_RUN_TEST(test_asdf_gwcs_eval_rotate3d_native_pole),
    MU_RUN_TEST(test_asdf_gwcs_eval_rotate3d_round_trip),
    MU_RUN_TEST(test_asdf_gwcs_eval_copy),
    MU_RUN_TEST(test_asdf_gwcs_eval_copy_invalid),
    MU_RUN_TEST(test_asdf_gwcs_eval_copy_threaded)
);


MU_RUN_SUITE(gwcs_eval);
