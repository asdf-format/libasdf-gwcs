#include <asdf.h>
#include <asdf/gwcs/fitswcs_imaging.h>
#include <asdf/gwcs/gwcs.h>
#include <asdf/gwcs/transform.h>

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
    const char *path = get_fixture_file_path("roman_wcs.asdf");
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
    const char *path = get_fixture_file_path("roman_wcs.asdf");
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


MU_TEST_SUITE(
    gwcs,
    MU_RUN_TEST(test_asdf_get_gwcs_fits),
    MU_RUN_TEST(test_asdf_get_gwcs),
    MU_RUN_TEST(test_asdf_set_gwcs_fits),
    MU_RUN_TEST(test_asdf_set_gwcs)
);


MU_RUN_SUITE(gwcs);
