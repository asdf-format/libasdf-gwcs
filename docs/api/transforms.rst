.. _api-transforms:

Transforms
==========

`asdf_gwcs_transform_t` implements the
:transform-schema:`base transform schema <transform-1.2.0>` that every
:external+asdf-transform-schemas:doc:`transform type <transforms>` derives
from, and is the base of a tagged union.  Its ``type`` member is an opaque
token identifying the concrete type; `asdf_gwcs_transform_tag` turns that into
the full YAML tag.

Concrete transform types embed the base via the ``ASDF_GWCS_TRANSFORM_BASE``
macro.  That macro expands to an anonymous union, so the base fields are
reachable both directly and through an explicit ``.base``:

.. code:: c

   const asdf_gwcs_shift_t *shift = (const asdf_gwcs_shift_t *)transform;

   shift->n_inputs;        /* both spellings are equivalent */
   shift->base.n_inputs;

.. note::

   ``ASDF_GWCS_TRANSFORM_BASE`` restates `asdf_gwcs_transform_t`'s fields so
   that they are picked up for each concrete type by the documentation
   extractor.  The two must be kept in step when a base field is added.


Tag versions
------------

Each transform type is registered for several versions of its schema.
`asdf_gwcs_transform_tag` reports the version a transform was *read with*, not
the version the library would write, so it is an accurate description of what
was in the file:

.. code:: c

   printf("%s\n", asdf_gwcs_transform_tag(step->transform));
   /* e.g. tag:stsci.edu:asdf/transform/compose-1.3.0 */

For a transform constructed in memory rather than read from a file, it falls
back to the type's preferred (newest registered) tag.


.. _concrete-transforms:

Concrete transforms
-------------------

These types have a dedicated struct carrying their own parameters, so their
type-specific fields are populated on read and written back out:

.. c:autosummary::
   :header: Transform Summary
   :widths: 25 75

   affine <asdf_gwcs_affine_t>
   compose <asdf_gwcs_compose_t>
   concatenate <asdf_gwcs_concatenate_t>
   constant <asdf_gwcs_constant_t>
   divide <asdf_gwcs_divide_t>
   fitswcs_imaging <asdf_gwcs_fits_t>
   identity <asdf_gwcs_identity_t>
   polynomial <asdf_gwcs_polynomial_t>
   remap_axes <asdf_gwcs_remap_axes_t>
   rotate_sequence_3d <asdf_gwcs_rotate_sequence_3d_t>
   scale <asdf_gwcs_scale_t>
   shift <asdf_gwcs_shift_t>
   spherical_cartesian <asdf_gwcs_spherical_cartesian_t>


.. _generic-transforms:

Generic transforms
------------------

The remaining transform types--almost all of them spherical projections-
are registered *generically*.  Their tags are recognized, they deserialize into
a plain `asdf_gwcs_transform_t`, and they round-trip through the library
unchanged, but they have no dedicated struct, so only the base fields are
populated.  Each still gets the full :ref:`generated API <generated-api>` and a
``ASDF_GWCS_TRANSFORM_<NAME>`` type constant.

Where the projection corresponds to a FITS WCS projection code (``CTYPE``),
that code is listed here with its corresponding tag name:

.. list-table::
   :header-rows: 1
   :widths: 45 10 35

   * - Projection
     - CTYPE
     - Newest tag
   * - :transform-schema:`Airy's <airy-1.2.0>`
     - AIR
     - airy-1.4.0
   * - :transform-schema:`Bonne's equal area <bonne_equal_area-1.3.0>`
     - BON
     - bonne_equal_area-1.5.0
   * - :transform-schema:`COBE quadrilateralized spherical cube <cobe_quad_spherical_cube-1.2.0>`
     - CSC
     - cobe_quad_spherical_cube-1.4.0
   * - :transform-schema:`Conic equal area <conic_equal_area-1.3.0>`
     - COE
     - conic_equal_area-1.5.0
   * - :transform-schema:`Conic equidistant <conic_equidistant-1.3.0>`
     - COD
     - conic_equidistant-1.5.0
   * - :transform-schema:`Conic orthomorphic <conic_orthomorphic-1.3.0>`
     - COO
     - conic_orthomorphic-1.5.0
   * - :transform-schema:`Conic perspective <conic_perspective-1.3.0>`
     - COP
     - conic_perspective-1.5.0
   * - :transform-schema:`Cylindrical equal area <cylindrical_equal_area-1.3.0>`
     - CEA
     - cylindrical_equal_area-1.5.0
   * - :transform-schema:`Cylindrical perspective <cylindrical_perspective-1.3.0>`
     - CYP
     - cylindrical_perspective-1.5.0
   * - :transform-schema:`Gnomonic <gnomonic-1.2.0>`
     - TAN
     - gnomonic-1.4.0
   * - :transform-schema:`Hammer-Aitoff <hammer_aitoff-1.2.0>`
     - AIT
     - hammer_aitoff-1.4.0
   * - HEALPix polar (butterfly)
     - XPH
     - healpix_polar-1.4.0
   * - :transform-schema:`Mollweide's <molleweide-1.2.0>`
     - MOL
     - molleweide-1.4.0
   * - :transform-schema:`Parabolic <parabolic-1.2.0>`
     - PAR
     - parabolic-1.4.0
   * - :transform-schema:`Plate carrée <plate_carree-1.2.0>`
     - CAR
     - plate_carree-1.4.0
   * - :transform-schema:`Polyconic <polyconic-1.2.0>`
     - PCO
     - polyconic-1.4.0
   * - :transform-schema:`Quadrilateralized spherical cube <quad_spherical_cube-1.2.0>`
     - QSC
     - quad_spherical_cube-1.4.0
   * - :transform-schema:`Sanson-Flamsteed <sanson_flamsteed-1.2.0>`
     - SFL
     - sanson_flamsteed-1.4.0
   * - :transform-schema:`Slant orthographic <slant_orthographic-1.2.0>`
     - SIN
     - slant_orthographic-1.4.0
   * - :transform-schema:`Slant zenithal perspective <slant_zenithal_perspective-1.2.0>`
     - SZP
     - slant_zenithal_perspective-1.4.0
   * - :transform-schema:`Stereographic <stereographic-1.2.0>`
     - STG
     - stereographic-1.4.0
   * - :transform-schema:`Tangential spherical cube <tangential_spherical_cube-1.2.0>`
     - TSC
     - tangential_spherical_cube-1.4.0
   * - :transform-schema:`Zenithal/azimuthal equal area <zenithal_equal_area-1.2.0>`
     - ZEA
     - zenithal_equal_area-1.4.0
   * - :transform-schema:`Zenithal/azimuthal equidistant <zenithal_equidistant-1.2.0>`
     - ARC
     - zenithal_equidistant-1.4.0
   * - :transform-schema:`Zenithal/azimuthal perspective <zenithal_perspective-1.3.0>`
     - AZP
     - zenithal_perspective-1.5.0

.. note::

   ``molleweide`` is spelled that way deliberately: it matches the upstream
   schema name, which misspells "Mollweide".

   Each projection links to its schema documentation, which upstream publishes
   at a lower version than the newest tag this library accepts, so the two
   versions differ.  ``healpix_polar`` has no published page at all and so is
   not linked.


Header reference
----------------

Each header's page documents its structs, its generated accessors and its
``ASDF_GWCS_TRANSFORM_<NAME>`` type constant in full.  Besides one page per
concrete transform, there are three that are not transform types themselves:

:doc:`transform/transform.h </api/transform/transform.h>`
    The base type shared by every transform, and the registration machinery
    concrete types are declared through.

:doc:`transform/property/bounding_box.h </api/transform/property/bounding_box.h>`
    The ``bounding_box`` property that any transform may carry, giving the
    domain over which it is valid.

:doc:`transform.h </api/transform.h>`
    The umbrella header, which pulls in all of the above.

.. toctree::
  :maxdepth: 1

  transform/transform.h
  transform/affine.h
  transform/compose.h
  transform/concatenate.h
  transform/constant.h
  transform/divide.h
  transform/identity.h
  transform/polynomial.h
  transform/remap_axes.h
  transform/rotate_sequence_3d.h
  transform/scale.h
  transform/shift.h
  transform/spherical_cartesian.h
  fitswcs_imaging.h
  transform/property/bounding_box.h
  transform.h


