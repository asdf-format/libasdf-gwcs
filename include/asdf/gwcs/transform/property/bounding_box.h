/**
 * Partial implementation of version 1.2.0 of the
 * :transform-schema:`transform/property/bounding_box
 * <property/bounding_box-1.0.0>` schema
 */

//

#ifndef ASDF_GWCS_TRANSFORM_PROPERTY_BOUNDING_BOX_H
#define ASDF_GWCS_TRANSFORM_PROPERTY_BOUNDING_BOX_H

#include <stdint.h>

#include <asdf/extension.h>
#include <asdf/util.h>

ASDF_BEGIN_DECLS

#define ASDF_GWCS_BOUNDING_BOX_TAG ASDF_GWCS_TRANSFORM_TAG_PREFIX "property/bounding_box-1.2.0"
/**
 * Enum values for array storage order
 *
 * This probably belongs in a different header but the bounding_box is the first schema
 * where I've seen it used so it is declared here for now.
 */
typedef enum {
    /** C order */
    ASDF_ARRAY_STORAGE_ORDER_C = 'C',
    /** FORTRAN order */
    ASDF_ARRAY_STORAGE_ORDER_F = 'F'
} asdf_array_storage_order_t;


/**
 * Pairs an axis name with an interval
 *
 * .. todo::
 *
 *   Currently only numeric intervals are supported, not quantities.
 */
/**
 * The valid interval of a single input to a transform
 */
typedef struct {
    /** Name of the input this interval constrains (may be ``NULL``) */
    const char *input_name;

    /** Inclusive lower and upper bounds */
    double bounds[2];
} asdf_gwcs_interval_t;


/**
 * The domain over which a transform is defined
 *
 * A bounding box constrains each of a transform's inputs to an interval.
 */
typedef struct {
    /** Number of entries in `intervals` */
    uint32_t n_intervals;

    /** Array of `n_intervals` per-input intervals */
    const asdf_gwcs_interval_t *intervals;
    /** Null-terminated (or NULL pointer when missing) array of ignored inputs */
    const char **ignore;
    /** Input ordering (C or FORTRAN) */
    asdf_array_storage_order_t order;
} asdf_gwcs_bounding_box_t;


ASDF_DECLARE_EXTENSION(gwcs_bounding_box, asdf_gwcs_bounding_box_t);


ASDF_END_DECLS
#endif /* ASDF_GWCS_TRANSFORM_PROPERTY_BOUNDING_BOX_H */
