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

   The header is ``frame_celestial.h`` and the C type
   `asdf_gwcs_frame_celestial_t`, but the schema tag reverses the words:
   ``gwcs/celestial_frame-1.2.0``.

A celestial frame additionally owns a *reference frame* --- the astropy
coordinate frame it is expressed in.  These are declared in
``coordinates/baseframe.h``, which registers ICRS, Galactic, FK5, FK4 and
FK4NoETerms.

.. warning::

   Reference-frame ``frame_attributes``, notably FK4/FK5 ``equinox``, are
   currently parsed but discarded, pending multi-version tag resolution
   support in libasdf.  They are written back as an empty mapping.

.. toctree::
  :maxdepth: 1

  frame.h
  frame2d.h
  frame_celestial.h
  coordinates/baseframe.h
