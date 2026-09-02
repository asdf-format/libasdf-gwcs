.. _api-frames:

Coordinate frames
=================

`asdf_gwcs_frame_t` is the base type for all coordinate frames.  It carries a
``type`` discriminant and a ``name``; concrete frame types embed it as their
first member, so a frame pointer can be downcast once its ``type`` has been
checked:

.. code:: c

   if (frame->type == ASDF_GWCS_FRAME_CELESTIAL) {
       const asdf_gwcs_frame_celestial_t *cel =
           (const asdf_gwcs_frame_celestial_t *)frame;
       /* ... */
   }

.. note::

   The header is :doc:`frame_celestial.h </api/frame_celestial.h>` and the
   C type
   `asdf_gwcs_frame_celestial_t`, but the schema tag reverses the words:
   :external+asdf-wcs-schemas:doc:`gwcs/celestial_frame
   <generated/gwcs/celestial_frame-1.0.0>`.

A celestial frame additionally owns a *reference frame*---the astropy
coordinate frame it is expressed in.  These are declared in
``coordinates/baseframe.h``, which registers the
:external+asdf-coordinates-schemas:doc:`astropy frame schemas <frames>` for
ICRS, Galactic, FK5, FK4 and FK4NoETerms.

ICRS and Galactic define no ``frame_attributes`` and are represented directly
as `asdf_gwcs_baseframe_t`.  The equinox-based frames have concrete types of
their own in ``coordinates/fk.h``, carrying the ``equinox`` every one of them
requires and, for the FK4 frames, an optional ``obstime``.

.. toctree::
  :maxdepth: 1

  frame.h
  frame2d.h
  frame_celestial.h
  coordinates/baseframe.h
  coordinates/fk.h
