#include <stdlib.h>

#include <asdf.h>
#include <asdf/gwcs/fitswcs_imaging.h>
#include <asdf/gwcs/gwcs.h>
#include <asdf/gwcs/transform.h>
#include <asdf/gwcs/transforms/compose.h>
#include <asdf/gwcs/transforms/concatenate.h>
#include <asdf/gwcs/transforms/polynomial.h>
#include <asdf/gwcs/transforms/remap_axes.h>
#include <asdf/gwcs/transforms/rotate_sequence_3d.h>
#include <asdf/gwcs/transforms/shift.h>

#include "munit.h"
#include "util.h"


/**
 * Assert that a deserialized asdf_gwcs_fits_t matches the expected values used
 * in the write tests (crpix/crval/cdelt/pc/projection).
 */
static void check_fits_values(const asdf_gwcs_fits_t *fits_out) {
    double crpix[2] = {12099.5, -88700.5};
    double crval[2] = {270.0, 64.60237301};
    double cdelt[2] = {1.52777778e-05, 1.52777778e-05};
    double pc[2][2] = {{1.0, 0.0}, {-0.0, 1.0}};

    for (int idx = 0; idx < 2; idx++) {
        assert_double_equal(fits_out->crpix[idx], crpix[idx], 5);
        assert_double_equal(fits_out->crval[idx], crval[idx], 5);
        assert_double_equal(fits_out->cdelt[idx], cdelt[idx], 5);
        for (int jdx = 0; jdx < 2; jdx++) {
            assert_double_equal(fits_out->pc[idx][jdx], pc[idx][jdx], 5);
        }
    }

    assert_int(fits_out->projection.type, ==, ASDF_GWCS_TRANSFORM_GNOMONIC);
}


MU_TEST(test_asdf_set_gwcs_fits) {
    const char *path = get_temp_file_path(fixture->tempfile_prefix, ".asdf");
    asdf_file_t *file = asdf_open(NULL);
    assert_not_null(file);

    asdf_gwcs_fits_t fits = {
        .base = {.type = ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING},
        .crpix = {12099.5, -88700.5},
        .crval = {270.0, 64.60237301},
        .cdelt = {1.52777778e-05, 1.52777778e-05},
        .pc = {{1.0, 0.0}, {-0.0, 1.0}},
        .projection = {.type = ASDF_GWCS_TRANSFORM_GNOMONIC},
    };

    assert_int(asdf_set_gwcs_fits(file, "transform", &fits), ==, ASDF_VALUE_OK);
    assert_int(asdf_write_to(file, path), ==, 0);
    asdf_close(file);

    // Re-open and validate the round-tripped data
    file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_fits_t *fits_out = NULL;
    asdf_value_err_t err = asdf_get_gwcs_fits(file, "transform", &fits_out);
    assert_int(err, ==, ASDF_VALUE_OK);
    assert_not_null(fits_out);

    asdf_gwcs_transform_t *transform = (asdf_gwcs_transform_t *)fits_out;
    assert_int(transform->type, ==, ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING);
    assert_null(transform->name);
    assert_null(transform->bounding_box);

    check_fits_values(fits_out);

    // ctype is NULL when reading fits without the full gwcs context
    assert_null(fits_out->ctype[0]);
    assert_null(fits_out->ctype[1]);

    asdf_gwcs_fits_destroy(fits_out);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_set_gwcs) {
    const char *path = get_temp_file_path(fixture->tempfile_prefix, ".asdf");
    asdf_file_t *file = asdf_open(NULL);
    assert_not_null(file);

    asdf_gwcs_frame2d_t detector_frame = {
        .base = {.type = ASDF_GWCS_FRAME_2D, .name = "detector"},
        .axes_names = {"x", "y"},
        .axes_order = {0, 1},
        .axis_physical_types = {"custom:x", "custom:y"},
    };

    asdf_gwcs_fits_t fits = {
        .base = {.type = ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING},
        .crpix = {12099.5, -88700.5},
        .crval = {270.0, 64.60237301},
        .cdelt = {1.52777778e-05, 1.52777778e-05},
        .pc = {{1.0, 0.0}, {-0.0, 1.0}},
        .projection = {.type = ASDF_GWCS_TRANSFORM_GNOMONIC},
    };

    asdf_gwcs_frame_celestial_t icrs_frame = {
        .base = {.type = ASDF_GWCS_FRAME_CELESTIAL, .name = "icrs"},
        .axes_names = {"lon", "lat", NULL},
        .axes_order = {0, 1, 0},
        .axis_physical_types = {"pos.eq.ra", "pos.eq.dec", NULL},
    };

    asdf_gwcs_step_t steps[2] = {
        {.frame = (asdf_gwcs_frame_t *)&detector_frame,
         .transform = (const asdf_gwcs_transform_t *)&fits},
        {.frame = (asdf_gwcs_frame_t *)&icrs_frame, .transform = NULL},
    };

    asdf_gwcs_t gwcs = {
        .name = "test_wcs",
        .n_steps = 2,
        .steps = steps,
    };

    assert_int(asdf_set_gwcs(file, "wcs", &gwcs), ==, ASDF_VALUE_OK);
    assert_int(asdf_write_to(file, path), ==, 0);
    asdf_close(file);

    // Re-open and validate the round-tripped data
    file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_t *gwcs_out = NULL;
    asdf_value_err_t err = asdf_get_gwcs(file, "wcs", &gwcs_out);
    assert_int(err, ==, ASDF_VALUE_OK);
    assert_not_null(gwcs_out);

    assert_string_equal(gwcs_out->name, "test_wcs");
    assert_int(gwcs_out->n_steps, ==, 2);
    assert_not_null(gwcs_out->steps);

    const asdf_gwcs_step_t *step = &gwcs_out->steps[0];
    assert_not_null(step->frame);
    assert_int(step->frame->type, ==, ASDF_GWCS_FRAME_2D);
    assert_string_equal(step->frame->name, "detector");
    asdf_gwcs_frame2d_t *frame2d_out = (asdf_gwcs_frame2d_t *)step->frame;
    assert_string_equal(frame2d_out->axes_names[0], "x");
    assert_string_equal(frame2d_out->axes_names[1], "y");
    assert_string_equal(frame2d_out->axis_physical_types[0], "custom:x");
    assert_string_equal(frame2d_out->axis_physical_types[1], "custom:y");
    assert_int(frame2d_out->axes_order[0], ==, 0);
    assert_int(frame2d_out->axes_order[1], ==, 1);
    assert_not_null(step->transform);
    assert_int(step->transform->type, ==, ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING);

    asdf_gwcs_fits_t *fits_out = (asdf_gwcs_fits_t *)step->transform;
    check_fits_values(fits_out);

    // ctype should be filled in since step[1] has a celestial frame with
    // recognized axis_physical_types
    assert_string_equal(fits_out->ctype[0], "RA---TAN");
    assert_string_equal(fits_out->ctype[1], "DEC--TAN");

    step = &gwcs_out->steps[1];
    assert_not_null(step->frame);
    assert_int(step->frame->type, ==, ASDF_GWCS_FRAME_CELESTIAL);
    assert_string_equal(step->frame->name, "icrs");
    asdf_gwcs_frame_celestial_t *frame_celestial_out = (asdf_gwcs_frame_celestial_t *)step->frame;
    assert_string_equal(frame_celestial_out->axes_names[0], "lon");
    assert_string_equal(frame_celestial_out->axes_names[1], "lat");
    assert_string_equal(frame_celestial_out->axis_physical_types[0], "pos.eq.ra");
    assert_string_equal(frame_celestial_out->axis_physical_types[1], "pos.eq.dec");
    assert_int(frame_celestial_out->axes_order[0], ==, 0);
    assert_int(frame_celestial_out->axes_order[1], ==, 1);
    assert_null(step->transform);

    asdf_gwcs_destroy(gwcs_out);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_get_gwcs_fits) {
    const char *path = get_fixture_file_path("roman_l3_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_fits_t *fits = NULL;
    asdf_value_err_t err = asdf_get_gwcs_fits(file, "wcs/steps/0/transform", &fits);
    assert_int(err, ==, ASDF_VALUE_OK);
    assert_not_null(fits);

    // Cast to a generic transform and check the transform properties
    asdf_gwcs_transform_t *transform = (asdf_gwcs_transform_t *)fits;
    // Actually in this test case the transform is not given a name
    assert_int(transform->type, ==, ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING);
    assert_null(transform->name);
    assert_not_null(transform->bounding_box);
    const asdf_gwcs_bounding_box_t *bb = transform->bounding_box;
    assert_string_equal(bb->intervals[0].input_name, "x");
    assert_double_equal(bb->intervals[0].bounds[0], -0.5, 9);
    assert_double_equal(bb->intervals[0].bounds[1], 4999.5, 9);
    assert_string_equal(bb->intervals[1].input_name, "y");
    assert_double_equal(bb->intervals[1].bounds[0], -0.5, 9);
    assert_double_equal(bb->intervals[1].bounds[1], 4999.5, 9);
    //TODO
    //assert_int(bb->order, ==, ASDF_ARRAY_STORAGE_ORDER_F);

    // Now check the FITS-specific properties
    double crpix[2] = {12099.5, -88700.5};
    double crval[2] = {270.0, 64.60237301};
    double cdelt[2] = {1.52777778e-05, 1.52777778e-05};
    double pc[2][2] = {{1.0, 0.0}, {-0.0, 1.0}};

    for (int idx = 0; idx < 2; idx++) {
        assert_double_equal(fits->crpix[idx], crpix[idx], 5);
        assert_double_equal(fits->crval[idx], crval[idx], 5);
        assert_double_equal(fits->cdelt[idx], cdelt[idx], 5);
        for (int jdx = 0; jdx < 2; jdx++) {
            assert_double_equal(fits->pc[idx][jdx], pc[idx][jdx], 5);
        }
    }

    assert_int(fits->projection.type, ==, ASDF_GWCS_TRANSFORM_GNOMONIC);
    asdf_gwcs_fits_destroy(fits);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_get_gwcs) {
    const char *path = get_fixture_file_path("roman_l3_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_t *gwcs = NULL;
    asdf_value_err_t err = asdf_get_gwcs(file, "wcs", &gwcs);
    assert_int(err, ==, ASDF_VALUE_OK);
    assert_not_null(gwcs);

    assert_string_equal(gwcs->name, "270p65x48y69");
    assert_int(gwcs->n_steps, ==, 2);
    assert_not_null(gwcs->steps);

    const asdf_gwcs_step_t *step = &gwcs->steps[0];
    assert_not_null(step->frame);
    assert_int(step->frame->type, ==, ASDF_GWCS_FRAME_2D);
    assert_string_equal(step->frame->name, "detector");
    asdf_gwcs_frame2d_t *frame2d = (asdf_gwcs_frame2d_t *)step->frame;
    assert_string_equal(frame2d->axes_names[0], "x");
    assert_string_equal(frame2d->axes_names[1], "y");
    assert_string_equal(frame2d->axis_physical_types[0], "custom:x");
    assert_string_equal(frame2d->axis_physical_types[1], "custom:y");
    assert_int(frame2d->axes_order[0], ==, 0);
    assert_int(frame2d->axes_order[1], ==, 1);
    assert_not_null(step->transform);
    assert_int(step->transform->type, ==, ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING);

    // Check that the FITS CTYPEn keywords were initialized successfully
    asdf_gwcs_fits_t *fits = (asdf_gwcs_fits_t *)step->transform;
    assert_string_equal(fits->ctype[0], "RA---TAN");
    assert_string_equal(fits->ctype[1], "DEC--TAN");

    step = &gwcs->steps[1];
    assert_not_null(step->frame);
    assert_int(step->frame->type, ==, ASDF_GWCS_FRAME_CELESTIAL);
    assert_string_equal(step->frame->name, "icrs");
    asdf_gwcs_frame_celestial_t *frame_celestial = (asdf_gwcs_frame_celestial_t *)step->frame;
    assert_string_equal(frame_celestial->axes_names[0], "lon");
    assert_string_equal(frame_celestial->axes_names[1], "lat");
    assert_string_equal(frame_celestial->axis_physical_types[0], "pos.eq.ra");
    assert_string_equal(frame_celestial->axis_physical_types[1], "pos.eq.dec");
    assert_null(frame_celestial->axes_names[2]);
    assert_int(frame_celestial->axes_order[0], ==, 0);
    assert_int(frame_celestial->axes_order[1], ==, 1);
    assert_null(step->transform);

    asdf_gwcs_destroy(gwcs);
    asdf_close(file);
    return MUNIT_OK;
}


/** shift transform tests */

static void check_shift_values(const asdf_gwcs_shift_t *shift, double expected_offset) {
    assert_not_null(shift);
    assert_int(((const asdf_gwcs_transform_t *)shift)->type, ==, ASDF_GWCS_TRANSFORM_SHIFT);
    assert_double_equal(shift->offset, expected_offset, 10);
    assert_uint32(shift->base.n_inputs, ==, 1);
    assert_uint32(shift->base.n_outputs, ==, 1);
}


MU_TEST(test_asdf_set_gwcs_shift) {
    const char *path = get_temp_file_path(fixture->tempfile_prefix, ".asdf");
    asdf_file_t *file = asdf_open(NULL);
    assert_not_null(file);

    asdf_gwcs_shift_t shift = {
        .base = {.type = ASDF_GWCS_TRANSFORM_SHIFT},
        .offset = 42.5,
    };

    assert_int(asdf_set_gwcs_shift(file, "transform", &shift), ==, ASDF_VALUE_OK);
    assert_int(asdf_write_to(file, path), ==, 0);
    asdf_close(file);

    file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_shift_t *shift_out = NULL;
    assert_int(asdf_get_gwcs_shift(file, "transform", &shift_out), ==, ASDF_VALUE_OK);
    check_shift_values(shift_out, 42.5);

    asdf_gwcs_shift_destroy(shift_out);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_get_gwcs_shift_from_fixture) {
    const char *path = get_fixture_file_path("roman_l2_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_shift_t *shift = NULL;
    assert_int(asdf_get_gwcs_shift(file,
        "roman/meta/wcs/steps/0/transform/forward/0/forward/0/forward/0/forward/0",
        &shift), ==, ASDF_VALUE_OK);
    check_shift_values(shift, 1.0);

    asdf_gwcs_shift_destroy(shift);
    asdf_close(file);
    return MUNIT_OK;
}


/* remap_axes */

static void check_remap_axes_values(
    const asdf_gwcs_remap_axes_t *remap, uint32_t n, const uint32_t *expected) {
    assert_not_null(remap);
    assert_int(((const asdf_gwcs_transform_t *)remap)->type, ==, ASDF_GWCS_TRANSFORM_REMAP_AXES);
    assert_uint32(remap->base.n_outputs, ==, n);
    assert_not_null(remap->mapping);
    uint32_t max_val = 0;
    for (uint32_t idx = 0; idx < n; idx++) {
        assert_uint32(remap->mapping[idx], ==, expected[idx]);
        if (expected[idx] > max_val)
            max_val = expected[idx];
    }
    assert_uint32(remap->base.n_inputs, ==, max_val + 1);
}


MU_TEST(test_asdf_set_gwcs_remap_axes) {
    const char *path = get_temp_file_path(fixture->tempfile_prefix, ".asdf");
    asdf_file_t *file = asdf_open(NULL);
    assert_not_null(file);

    uint32_t mapping[] = {1, 0, 2};
    asdf_gwcs_remap_axes_t remap = {
        .base = {.type = ASDF_GWCS_TRANSFORM_REMAP_AXES, .n_outputs = 3},
        .mapping = mapping,
    };

    assert_int(asdf_set_gwcs_remap_axes(file, "transform", &remap), ==, ASDF_VALUE_OK);
    assert_int(asdf_write_to(file, path), ==, 0);
    asdf_close(file);

    file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_remap_axes_t *remap_out = NULL;
    assert_int(asdf_get_gwcs_remap_axes(file, "transform", &remap_out), ==, ASDF_VALUE_OK);
    check_remap_axes_values(remap_out, 3, mapping);

    asdf_gwcs_remap_axes_destroy(remap_out);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_get_gwcs_remap_axes_from_fixture) {
    const char *path = get_fixture_file_path("roman_l2_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_remap_axes_t *remap = NULL;
    assert_int(asdf_get_gwcs_remap_axes(file,
        "roman/meta/wcs/steps/0/transform/forward/0/forward/1/forward/0/forward/0",
        &remap), ==, ASDF_VALUE_OK);

    uint32_t expected[] = {0, 1, 0, 1};
    check_remap_axes_values(remap, 4, expected);

    asdf_gwcs_remap_axes_destroy(remap);
    asdf_close(file);
    return MUNIT_OK;
}


/* polynomial */

static void check_polynomial_shape(
    const asdf_gwcs_polynomial_t *poly, uint32_t ndim, uint32_t degree, uint32_t n_coeffs) {
    assert_not_null(poly);
    assert_int(((const asdf_gwcs_transform_t *)poly)->type, ==, ASDF_GWCS_TRANSFORM_POLYNOMIAL);
    assert_uint32(poly->ndim, ==, ndim);
    assert_uint32(poly->degree, ==, degree);
    assert_uint32(poly->n_coeffs, ==, n_coeffs);
    assert_not_null(poly->coefficients);
    assert_uint32(poly->base.n_inputs, ==, ndim);
    assert_uint32(poly->base.n_outputs, ==, 1);
}


MU_TEST(test_asdf_set_gwcs_polynomial) {
    const char *path = get_temp_file_path(fixture->tempfile_prefix, ".asdf");
    asdf_file_t *file = asdf_open(NULL);
    assert_not_null(file);

    double coeffs[4] = {1.0, 2.0, 3.0, 4.0};
    asdf_gwcs_polynomial_t poly = {
        .base = {.type = ASDF_GWCS_TRANSFORM_POLYNOMIAL},
        .ndim = 2,
        .degree = 1,
        .n_coeffs = 4,
        .coefficients = coeffs,
    };

    assert_int(asdf_set_gwcs_polynomial(file, "transform", &poly), ==, ASDF_VALUE_OK);
    assert_int(asdf_write_to(file, path), ==, 0);
    asdf_close(file);

    file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_polynomial_t *poly_out = NULL;
    assert_int(asdf_get_gwcs_polynomial(file, "transform", &poly_out), ==, ASDF_VALUE_OK);
    check_polynomial_shape(poly_out, 2, 1, 4);
    for (int idx = 0; idx < 4; idx++)
        assert_double_equal(poly_out->coefficients[idx], coeffs[idx], 10);

    asdf_gwcs_polynomial_destroy(poly_out);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_get_gwcs_polynomial_from_fixture) {
    const char *path = get_fixture_file_path("roman_l2_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    /* First polynomial in the first compose|concatenate block: shape [6,6] */
    asdf_gwcs_polynomial_t *poly = NULL;
    assert_int(asdf_get_gwcs_polynomial(file,
        "roman/meta/wcs/steps/0/transform/forward/0/forward/1/forward/0/forward/1/forward/0",
        &poly), ==, ASDF_VALUE_OK);
    check_polynomial_shape(poly, 2, 5, 36);

    asdf_gwcs_polynomial_destroy(poly);
    asdf_close(file);
    return MUNIT_OK;
}


/* rotate_sequence_3d */

static void check_rotate_sequence_3d_values(
    const asdf_gwcs_rotate_sequence_3d_t *rot,
    uint32_t n_angles, const double *expected_angles,
    const char *expected_axes_order) {
    assert_not_null(rot);
    assert_int(((const asdf_gwcs_transform_t *)rot)->type, ==,
        ASDF_GWCS_TRANSFORM_ROTATE_SEQUENCE_3D);
    assert_uint32(rot->n_angles, ==, n_angles);
    assert_not_null(rot->angles);
    assert_string_equal(rot->axes_order, expected_axes_order);
    for (uint32_t idx = 0; idx < n_angles; idx++)
        assert_double_equal(rot->angles[idx], expected_angles[idx], 10);
    assert_uint32(rot->base.n_inputs, ==, 3);
    assert_uint32(rot->base.n_outputs, ==, 3);
}


MU_TEST(test_asdf_set_gwcs_rotate_sequence_3d) {
    const char *path = get_temp_file_path(fixture->tempfile_prefix, ".asdf");
    asdf_file_t *file = asdf_open(NULL);
    assert_not_null(file);

    double angles[3] = {10.0, 20.0, 30.0};
    asdf_gwcs_rotate_sequence_3d_t rot = {
        .base = {.type = ASDF_GWCS_TRANSFORM_ROTATE_SEQUENCE_3D},
        .n_angles = 3,
        .angles = angles,
        .axes_order = "xyz",
    };

    assert_int(asdf_set_gwcs_rotate_sequence_3d(file, "transform", &rot), ==, ASDF_VALUE_OK);
    assert_int(asdf_write_to(file, path), ==, 0);
    asdf_close(file);

    file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_rotate_sequence_3d_t *rot_out = NULL;
    assert_int(asdf_get_gwcs_rotate_sequence_3d(file, "transform", &rot_out), ==, ASDF_VALUE_OK);
    check_rotate_sequence_3d_values(rot_out, 3, angles, "xyz");

    asdf_gwcs_rotate_sequence_3d_destroy(rot_out);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_get_gwcs_rotate_sequence_3d_from_fixture) {
    const char *path = get_fixture_file_path("roman_l2_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_rotate_sequence_3d_t *rot = NULL;
    assert_int(asdf_get_gwcs_rotate_sequence_3d(file,
        "roman/meta/wcs/steps/2/transform"
        "/forward/0/forward/0/forward/0/forward/0/forward/0/forward/0/forward/1",
        &rot), ==, ASDF_VALUE_OK);
    double expected_angles[3] = {
        0.3647080959023555, 0.28910704796541764, 59.846679364670365
    };
    check_rotate_sequence_3d_values(rot, 3, expected_angles, "zyx");

    asdf_gwcs_rotate_sequence_3d_destroy(rot);
    asdf_close(file);
    return MUNIT_OK;
}


/* compose */

MU_TEST(test_asdf_set_gwcs_compose) {
    const char *path = get_temp_file_path(fixture->tempfile_prefix, ".asdf");
    asdf_file_t *file = asdf_open(NULL);
    assert_not_null(file);

    double offset0 = 1.5, offset1 = 2.5;
    asdf_gwcs_shift_t s0 = {
        .base = {.type = ASDF_GWCS_TRANSFORM_SHIFT},
        .offset = offset0,
    };
    asdf_gwcs_shift_t s1 = {
        .base = {.type = ASDF_GWCS_TRANSFORM_SHIFT},
        .offset = offset1,
    };
    asdf_gwcs_transform_t *fwd[2] = {
        (asdf_gwcs_transform_t *)&s0,
        (asdf_gwcs_transform_t *)&s1,
    };
    asdf_gwcs_compose_t compose = {
        .base = {.type = ASDF_GWCS_TRANSFORM_COMPOSE},
        .n_forward = 2,
        .forward = fwd,
    };

    assert_int(asdf_set_gwcs_compose(file, "transform", &compose), ==, ASDF_VALUE_OK);
    assert_int(asdf_write_to(file, path), ==, 0);
    asdf_close(file);

    file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_compose_t *compose_out = NULL;
    assert_int(asdf_get_gwcs_compose(file, "transform", &compose_out), ==, ASDF_VALUE_OK);
    assert_not_null(compose_out);
    assert_int(((const asdf_gwcs_transform_t *)compose_out)->type, ==,
        ASDF_GWCS_TRANSFORM_COMPOSE);
    assert_uint32(compose_out->n_forward, ==, 2);
    assert_not_null(compose_out->forward[0]);
    assert_int(compose_out->forward[0]->type, ==, ASDF_GWCS_TRANSFORM_SHIFT);
    assert_double_equal(((asdf_gwcs_shift_t *)compose_out->forward[0])->offset, offset0, 10);
    assert_not_null(compose_out->forward[1]);
    assert_int(compose_out->forward[1]->type, ==, ASDF_GWCS_TRANSFORM_SHIFT);
    assert_double_equal(((asdf_gwcs_shift_t *)compose_out->forward[1])->offset, offset1, 10);
    assert_uint32(compose_out->base.n_inputs, ==, 1);
    assert_uint32(compose_out->base.n_outputs, ==, 1);

    asdf_gwcs_compose_destroy(compose_out);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_get_gwcs_compose_from_fixture) {
    const char *path = get_fixture_file_path("roman_l2_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    /* Compose whose forward[1] is a rotate_sequence_3d */
    asdf_gwcs_compose_t *compose = NULL;
    assert_int(asdf_get_gwcs_compose(file,
        "roman/meta/wcs/steps/2/transform"
        "/forward/0/forward/0/forward/0/forward/0/forward/0/forward/0",
        &compose), ==, ASDF_VALUE_OK);
    assert_not_null(compose);
    assert_int(((const asdf_gwcs_transform_t *)compose)->type, ==,
        ASDF_GWCS_TRANSFORM_COMPOSE);
    assert_uint32(compose->n_forward, ==, 2);
    assert_not_null(compose->forward[1]);
    assert_int(compose->forward[1]->type, ==, ASDF_GWCS_TRANSFORM_ROTATE_SEQUENCE_3D);

    asdf_gwcs_compose_destroy(compose);
    asdf_close(file);
    return MUNIT_OK;
}


/* concatenate */

MU_TEST(test_asdf_set_gwcs_concatenate) {
    const char *path = get_temp_file_path(fixture->tempfile_prefix, ".asdf");
    asdf_file_t *file = asdf_open(NULL);
    assert_not_null(file);

    double offset0 = 10.0, offset1 = 20.0, offset2 = 30.0;
    asdf_gwcs_shift_t s0 = {
        .base = {.type = ASDF_GWCS_TRANSFORM_SHIFT},
        .offset = offset0,
    };
    asdf_gwcs_shift_t s1 = {
        .base = {.type = ASDF_GWCS_TRANSFORM_SHIFT},
        .offset = offset1,
    };
    asdf_gwcs_shift_t s2 = {
        .base = {.type = ASDF_GWCS_TRANSFORM_SHIFT},
        .offset = offset2,
    };
    asdf_gwcs_transform_t *fwd[3] = {
        (asdf_gwcs_transform_t *)&s0,
        (asdf_gwcs_transform_t *)&s1,
        (asdf_gwcs_transform_t *)&s2,
    };
    asdf_gwcs_concatenate_t concat = {
        .base = {.type = ASDF_GWCS_TRANSFORM_CONCATENATE},
        .n_forward = 3,
        .forward = fwd,
    };

    assert_int(asdf_set_gwcs_concatenate(file, "transform", &concat), ==, ASDF_VALUE_OK);
    assert_int(asdf_write_to(file, path), ==, 0);
    asdf_close(file);

    file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_concatenate_t *concat_out = NULL;
    assert_int(asdf_get_gwcs_concatenate(file, "transform", &concat_out), ==, ASDF_VALUE_OK);
    assert_not_null(concat_out);
    assert_int(((const asdf_gwcs_transform_t *)concat_out)->type, ==,
        ASDF_GWCS_TRANSFORM_CONCATENATE);
    assert_uint32(concat_out->n_forward, ==, 3);
    for (uint32_t idx = 0; idx < 3; idx++) {
        assert_not_null(concat_out->forward[idx]);
        assert_int(concat_out->forward[idx]->type, ==, ASDF_GWCS_TRANSFORM_SHIFT);
    }
    assert_double_equal(((asdf_gwcs_shift_t *)concat_out->forward[0])->offset, offset0, 10);
    assert_double_equal(((asdf_gwcs_shift_t *)concat_out->forward[1])->offset, offset1, 10);
    assert_double_equal(((asdf_gwcs_shift_t *)concat_out->forward[2])->offset, offset2, 10);
    assert_uint32(concat_out->base.n_inputs, ==, 3);
    assert_uint32(concat_out->base.n_outputs, ==, 3);

    asdf_gwcs_concatenate_destroy(concat_out);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_get_gwcs_concatenate_from_fixture) {
    const char *path = get_fixture_file_path("roman_l2_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    /* Concatenate of two shifts in step 0 */
    asdf_gwcs_concatenate_t *concat = NULL;
    assert_int(asdf_get_gwcs_concatenate(file,
        "roman/meta/wcs/steps/0/transform/forward/1",
        &concat), ==, ASDF_VALUE_OK);
    assert_not_null(concat);
    assert_int(((const asdf_gwcs_transform_t *)concat)->type, ==,
        ASDF_GWCS_TRANSFORM_CONCATENATE);
    assert_uint32(concat->n_forward, ==, 2);
    assert_not_null(concat->forward[0]);
    assert_int(concat->forward[0]->type, ==, ASDF_GWCS_TRANSFORM_SHIFT);
    assert_double_equal(((asdf_gwcs_shift_t *)concat->forward[0])->offset,
        1312.9491452484797, 10);
    assert_not_null(concat->forward[1]);
    assert_int(concat->forward[1]->type, ==, ASDF_GWCS_TRANSFORM_SHIFT);
    assert_double_equal(((asdf_gwcs_shift_t *)concat->forward[1])->offset,
        -1040.7853726755036, 10);
    assert_uint32(concat->base.n_inputs, ==, 2);
    assert_uint32(concat->base.n_outputs, ==, 2);

    asdf_gwcs_concatenate_destroy(concat);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST(test_asdf_get_roman_l2_gwcs) {
    const char *path = get_fixture_file_path("roman_l2_wcs.asdf");
    asdf_file_t *file = asdf_open(path, "r");
    assert_not_null(file);

    asdf_gwcs_t *gwcs = NULL;
    assert_int(asdf_get_gwcs(file, "roman/meta/wcs", &gwcs), ==, ASDF_VALUE_OK);
    assert_not_null(gwcs);
    assert_uint32(gwcs->n_steps, ==, 5);

    static const char *expected_names[] = {
        "detector", "v2v3", "v2v3vacorr", "v2v3corr", "world"
    };
    static const asdf_gwcs_frame_type_t expected_frame_types[] = {
        ASDF_GWCS_FRAME_2D,
        ASDF_GWCS_FRAME_2D,
        ASDF_GWCS_FRAME_2D,
        ASDF_GWCS_FRAME_2D,
        ASDF_GWCS_FRAME_CELESTIAL,
    };

    for (uint32_t idx = 0; idx < gwcs->n_steps; idx++) {
        const asdf_gwcs_step_t *step = &gwcs->steps[idx];
        assert_not_null(step->frame);
        assert_int(step->frame->type, ==, expected_frame_types[idx]);
        assert_string_equal(step->frame->name, expected_names[idx]);
    }

    for (uint32_t idx = 0; idx < gwcs->n_steps - 1; idx++) {
        assert_not_null(gwcs->steps[idx].transform);
        assert_int(gwcs->steps[idx].transform->type, ==, ASDF_GWCS_TRANSFORM_COMPOSE);
    }
    assert_null(gwcs->steps[gwcs->n_steps - 1].transform);

    asdf_gwcs_destroy(gwcs);
    asdf_close(file);
    return MUNIT_OK;
}


MU_TEST_SUITE(
    gwcs,
    MU_RUN_TEST(test_asdf_get_gwcs_fits),
    MU_RUN_TEST(test_asdf_get_gwcs),
    MU_RUN_TEST(test_asdf_set_gwcs_fits),
    MU_RUN_TEST(test_asdf_set_gwcs),
    MU_RUN_TEST(test_asdf_set_gwcs_shift),
    MU_RUN_TEST(test_asdf_get_gwcs_shift_from_fixture),
    MU_RUN_TEST(test_asdf_set_gwcs_remap_axes),
    MU_RUN_TEST(test_asdf_get_gwcs_remap_axes_from_fixture),
    MU_RUN_TEST(test_asdf_set_gwcs_polynomial),
    MU_RUN_TEST(test_asdf_get_gwcs_polynomial_from_fixture),
    MU_RUN_TEST(test_asdf_set_gwcs_rotate_sequence_3d),
    MU_RUN_TEST(test_asdf_get_gwcs_rotate_sequence_3d_from_fixture),
    MU_RUN_TEST(test_asdf_set_gwcs_compose),
    MU_RUN_TEST(test_asdf_get_gwcs_compose_from_fixture),
    MU_RUN_TEST(test_asdf_set_gwcs_concatenate),
    MU_RUN_TEST(test_asdf_get_gwcs_concatenate_from_fixture),
    MU_RUN_TEST(test_asdf_get_roman_l2_gwcs)
);


MU_RUN_SUITE(gwcs);
