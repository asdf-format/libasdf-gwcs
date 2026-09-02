/**
 * Shared frame_attributes handling for the equinox-based coordinate frames
 *
 * Should be possible to extend later if adding other frames that have
 * different frame attributes, though currently this code is geared towards
 * explicit support of the FK frames that do.
 *
 * FK4, FK4NoETerms and FK5 differ only in whether obstime is defined, so both
 * halves of the round trip take it as an optional out/in parameter.
 */
#pragma once

#include <asdf/core/time.h>
#include <asdf/file.h>
#include <asdf/value.h>


/**
 * Read frame_attributes into the given equinox and (optional) obstime
 *
 * ``obstime`` may be NULL for a schema that does not define one, in which case
 * the property is not looked for.  ``equinox`` is required: its absence is a
 * parse failure.  Both are owned by the caller on success.
 */
ASDF_LOCAL asdf_value_err_t asdf_gwcs_frame_attributes_parse(
    asdf_value_t *value, asdf_time_t **equinox, asdf_time_t **obstime);


/**
 * Build the mapping a frame with frame_attributes serializes to
 *
 * ``obstime`` may be NULL, in which case it is omitted; ``equinox`` is written
 * whenever it is non-NULL.
 */
ASDF_LOCAL asdf_value_t *asdf_gwcs_frame_attributes_serialize(
    asdf_file_t *file, const asdf_time_t *equinox, const asdf_time_t *obstime);
