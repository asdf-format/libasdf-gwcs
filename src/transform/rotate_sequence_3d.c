#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asdf/extension_util.h>
#include <asdf/log.h>

#include "../util.h"
#include "rotate_sequence_3d.h"
#include "transform.h"


static asdf_value_err_t asdf_gwcs_rotate_sequence_3d_deserialize(
    asdf_value_t *value, UNUSED(const void *userdata), void **out) {
    asdf_gwcs_rotate_sequence_3d_t *rot = NULL;
    asdf_value_err_t err = ASDF_VALUE_ERR_PARSE_FAILURE;
    asdf_mapping_t *map = NULL;
    asdf_sequence_t *angles_seq = NULL;
    double *angles = NULL;
    const char *axes_order = NULL;
    char *axes_order_copy = NULL;
    const char *rotation_type_str = NULL;
    asdf_gwcs_rotation_type_t rotation_type = ASDF_GWCS_ROTATION_TYPE_CARTESIAN;

    if (asdf_value_as_mapping(value, &map) != ASDF_VALUE_OK)
        goto cleanup;

    rot = calloc(1, sizeof(asdf_gwcs_rotate_sequence_3d_t));

    if (!rot) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_gwcs_transform_parse(value, &rot->base);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    err = asdf_get_required_property(map, "angles", ASDF_VALUE_SEQUENCE, NULL, (void *)&angles_seq);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    int n = asdf_sequence_size(angles_seq);

    if (n < 0) {
        err = ASDF_VALUE_ERR_PARSE_FAILURE;
        goto cleanup;
    }

    angles = calloc((size_t)n, sizeof(double));

    if (!angles) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    asdf_sequence_iter_t *iter = asdf_sequence_iter_init(angles_seq);

    while (asdf_sequence_iter_next(&iter)) {
        double v = 0.0;
        err = asdf_value_as_double(iter->value, &v);

        if (ASDF_IS_ERR(err)) {
            asdf_sequence_iter_destroy(iter);
            goto cleanup;
        }

        angles[iter->index] = v;
    }

    err = asdf_get_required_property(
        map, "axes_order", ASDF_VALUE_STRING, NULL, (void *)&axes_order);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    axes_order_copy = strdup(axes_order);

    if (!axes_order_copy) {
        err = ASDF_VALUE_ERR_OOM;
        goto cleanup;
    }

    err = asdf_get_optional_property(
        map, "rotation_type", ASDF_VALUE_STRING, NULL, (void *)&rotation_type_str);


    if (ASDF_IS_OPTIONAL_OK(err) && rotation_type_str != NULL) {
        if (strcmp(rotation_type_str, "spherical") == 0) {
            rotation_type = ASDF_GWCS_ROTATION_TYPE_SPHERICAL;
        } else if (strcmp(rotation_type_str, "cartesian") != 0) {
            err = ASDF_VALUE_ERR_PARSE_FAILURE;
            goto cleanup;
        }
    }

    rot->n_angles = (uint32_t)n;
    rot->angles = angles;
    rot->axes_order = axes_order_copy;
    rot->rotation_type = rotation_type;
    angles = NULL;
    axes_order_copy = NULL;

    asdf_gwcs_transform_arity_set(&rot->base, asdf_value_file(value), 3, 3);

    *out = rot;
    err = ASDF_VALUE_OK;
cleanup:
    free(angles);
    free(axes_order_copy);
    asdf_sequence_destroy(angles_seq);

    if (ASDF_IS_ERR(err))
        asdf_gwcs_rotate_sequence_3d_destroy(rot);

    return err;
}


static asdf_value_t *asdf_gwcs_rotate_sequence_3d_serialize(
    asdf_file_t *file, const void *obj, UNUSED(const void *userdata)) {
    if (UNLIKELY(!file || !obj))
        return NULL;

    const asdf_gwcs_rotate_sequence_3d_t *rot = obj;
    asdf_mapping_t *map = asdf_mapping_create(file);

    if (!map)
        return NULL;

    asdf_value_err_t err = asdf_gwcs_transform_serialize_base(file, &rot->base, map);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    asdf_sequence_t *seq = asdf_sequence_of_double(file, rot->angles, (int)rot->n_angles);

    if (!seq)
        goto cleanup;

    asdf_sequence_set_style(seq, ASDF_YAML_NODE_STYLE_FLOW);
    err = asdf_mapping_set_sequence(map, "angles", seq);

    if (ASDF_IS_ERR(err)) {
        asdf_sequence_destroy(seq);
        goto cleanup;
    }

    err = asdf_mapping_set_string0(map, "axes_order", rot->axes_order);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    const char *rotation_type_str =
        rot->rotation_type == ASDF_GWCS_ROTATION_TYPE_SPHERICAL ? "spherical" : "cartesian";
    err = asdf_mapping_set_string0(map, "rotation_type", rotation_type_str);

    if (ASDF_IS_ERR(err))
        goto cleanup;

    return asdf_value_of_mapping(map);
cleanup:
    asdf_mapping_destroy(map);
    return NULL;
}


static void asdf_gwcs_rotate_sequence_3d_dealloc(void *value) {
    if (!value)
        return;

    asdf_gwcs_rotate_sequence_3d_t *rot = (asdf_gwcs_rotate_sequence_3d_t *)value;
    asdf_gwcs_transform_clean(&rot->base);
    free((double *)rot->angles);
    free((char *)rot->axes_order);
    free(rot);
}


ASDF_REGISTER_EXTENSION(
    gwcs_rotate_sequence_3d,
    ASDF_GWCS_TRANSFORM_TAG_PREFIX "rotate_sequence_3d-1.1.0",
    asdf_gwcs_rotate_sequence_3d_t,
    &libasdf_software,
    asdf_gwcs_rotate_sequence_3d_serialize,
    asdf_gwcs_rotate_sequence_3d_deserialize,
    NULL,
    asdf_gwcs_rotate_sequence_3d_dealloc,
    NULL);
