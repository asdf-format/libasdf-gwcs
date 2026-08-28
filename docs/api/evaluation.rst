.. _api-evaluation:

Evaluation
==========

Evaluating a WCS applies its pipeline to actual coordinates.  See
:ref:`evaluation` for full details.

The lifecycle is: create an `asdf_gwcs_eval_t` from a WCS once, evaluate as
many points through it as you like, then destroy it.

.. code:: c

   asdf_gwcs_err_t err = ASDF_GWCS_OK;
   asdf_gwcs_eval_t *eval = asdf_gwcs_eval_create(file, wcs, NULL, &err);
   asdf_gwcs_eval_2d(eval, xin, yin, xout, yout, n);
   asdf_gwcs_eval_destroy(eval);

Only the pipeline's **forward** transformation is evaluated; there is no
reverse evaluation, and no N-dimensional counterpart, yet, to
`asdf_gwcs_eval_2d`.

``grid.h`` adds helpers for the common case of sampling a regular 2-D grid
without materializing the input coordinates yourself.

.. warning::

   The backend interface in ``backend.h`` is **experimental** and is not yet
   supported for third-party plugins.  It will change.  Support for
   third-party evaluation backends is planned once the design has settled.

.. toctree::
  :maxdepth: 1

  eval.h
  grid.h
  backend.h
