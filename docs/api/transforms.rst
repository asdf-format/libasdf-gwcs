.. _api-transforms:

Transforms
==========

`asdf_gwcs_transform_t` implements the
:external+asdf-transform-schemas:doc:`base transform schema
<generated/schemas/transform-1.2.0>` that every
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


Concrete transforms
-------------------

These types have a dedicated struct carrying their own parameters:

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


.. _generic-transforms:

Generic transforms
------------------

The remaining transform types--almost all of them spherical projections-
are registered *generically*.  Their tags are recognized, they deserialize into
a plain `asdf_gwcs_transform_t`, and they round-trip through the library
unchanged, but they have no dedicated struct, so only the base fields are
populated.  Each still gets the full :ref:`generated API <generated-api>` and a
``ASDF_GWCS_TRANSFORM_<NAME>`` type constant.

Where the projection corresponds to a FITS WCS projection code, that code is
listed here:

==========================  =====  ================================
Name                        CTYPE  Newest tag
==========================  =====  ================================
airy                        AIR    airy-1.4.0
bonne_equal_area            BON    bonne_equal_area-1.5.0
cobe_quad_spherical_cube    CSC    cobe_quad_spherical_cube-1.4.0
conic_equal_area            COE    conic_equal_area-1.5.0
conic_equidistant           COD    conic_equidistant-1.5.0
conic_orthomorphic          COO    conic_orthomorphic-1.5.0
conic_perspective           COP    conic_perspective-1.5.0
cylindrical_equal_area      CEA    cylindrical_equal_area-1.5.0
cylindrical_perspective     CYP    cylindrical_perspective-1.5.0
gnomonic                    TAN    gnomonic-1.4.0
hammer_aitoff               AIT    hammer_aitoff-1.4.0
healpix_polar               XPH    healpix_polar-1.4.0
molleweide                  MOL    molleweide-1.4.0
parabolic                   PAR    parabolic-1.4.0
plate_carree                CAR    plate_carree-1.4.0
polyconic                   PCO    polyconic-1.4.0
quad_spherical_cube         QSC    quad_spherical_cube-1.4.0
sanson_flamsteed            SFL    sanson_flamsteed-1.4.0
slant_orthographic          SIN    slant_orthographic-1.4.0
slant_zenithal_perspective  SZP    slant_zenithal_perspective-1.4.0
stereographic               STG    stereographic-1.4.0
tangential_spherical_cube   TSC    tangential_spherical_cube-1.4.0
zenithal_equal_area         ZEA    zenithal_equal_area-1.4.0
zenithal_equidistant        ARC    zenithal_equidistant-1.4.0
zenithal_perspective        AZP    zenithal_perspective-1.5.0
==========================  =====  ================================

.. note::

   ``molleweide`` is spelled that way deliberately: it matches the upstream
   schema name, which misspells "Mollweide".
