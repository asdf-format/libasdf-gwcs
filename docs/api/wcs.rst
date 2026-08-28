.. _api-wcs:

The WCS object
==============

`asdf_gwcs_t` is the top-level type: a named pipeline of steps.  Each
`asdf_gwcs_step_t` pairs a coordinate frame with the transform mapping out of
that frame into the next step's frame.

.. important::

   The **final step has no transform** --- its ``transform`` member is
   ``NULL``, because that step exists only to name the frame the pipeline ends
   in.  Any loop over ``steps`` must account for this.

.. toctree::
  :maxdepth: 1

  wcs.h
  step.h
