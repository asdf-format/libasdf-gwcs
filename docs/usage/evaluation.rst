.. _evaluation:

Evaluating a WCS
================

Reading a WCS gives you its structure; *evaluating* it applies the pipeline to
actual coordinates.  This is what libasdf-gwcs's evaluation engine provides.


What "forward" means
--------------------

A :external+asdf-wcs-schemas:doc:`GWCS pipeline <generated/gwcs/wcs-1.1.0>` can
begin and end in any coordinate frame.  In practice it usually starts on the
detector and ends on the sky, so evaluating it maps pixels to world
coordinates, but that is a property of the particular WCS, not of the format.

What libasdf-gwcs guarantees is narrower: it evaluates the **forward**
transformation of the pipeline as stored in the file.

.. note::

   Evaluating a transformation in the *reverse* direction is not yet supported,
   whether by using a stored analytic inverse or by solving numerically for one.
   Only the pipeline's forward direction is available.


The evaluation context
----------------------

Evaluation goes through an `asdf_gwcs_eval_t`, created once per WCS and reused
across as many points as you like:

.. code:: c

   asdf_gwcs_err_t err = ASDF_GWCS_OK;
   asdf_gwcs_eval_t *eval = asdf_gwcs_eval_create(file, wcs, NULL, &err);

   if (!eval) {
       fprintf(stderr, "%s\n", asdf_gwcs_strerror(err));
       return 1;
   }

   double xin[3] = {0.0, 1.0, 2.0};
   double yin[3] = {0.0, 1.0, 2.0};
   double xout[3], yout[3];

   asdf_gwcs_eval_2d(eval, xin, yin, xout, yout, 3);
   asdf_gwcs_eval_destroy(eval);

`asdf_gwcs_eval_2d` works on whole arrays at a time; prefer one call with many
points over many calls with one point.  Passing ``NULL`` as the backend selects
the default.

`asdf_gwcs_eval_destroy` accepts ``NULL``, so it is safe on an error path.

.. note::

   Only a two-dimensional entry point exists at present.  There is no
   N-dimensional equivalent of `asdf_gwcs_eval_2d` yet.


Evaluating over a grid
----------------------

Sampling a regular grid is common enough to have a helper.
`asdf_gwcs_grid2d_fill` expands an `asdf_gwcs_grid2d_t` into coordinate arrays,
and `asdf_gwcs_eval_grid2d` goes straight from the grid description to
evaluated outputs, processing one row at a time so that memory use stays
proportional to the row length rather than the whole grid:

.. code:: c
   :test: test-gwcs-grid
   :fixture: roman_l2_wcs.asdf

   #include <stdio.h>
   #include <stdlib.h>
   #include <asdf.h>
   #include <asdf/gwcs/gwcs.h>

   int main(int argc, char **argv) {
       if (argc < 2) {
           fprintf(stderr, "usage: %s FILE\n", argv[0]);
           return 1;
       }

       const asdf_gwcs_backend_t *backend = asdf_gwcs_backend_get("ast_yaml");

       if (!backend) {
           fprintf(stderr, "built without the ast_yaml backend\n");
           return 0;
       }

       asdf_file_t *file = asdf_open(argv[1], "r");
       asdf_value_t *root = asdf_get_value(file, "");
       asdf_value_t *found = asdf_value_find(root, asdf_value_is_gwcs);

       if (!found) {
           fprintf(stderr, "no GWCS found in %s\n", argv[1]);
           return 1;
       }

       asdf_gwcs_t *wcs = NULL;
       asdf_value_as_gwcs(found, &wcs);

       asdf_gwcs_err_t err = ASDF_GWCS_OK;
       asdf_gwcs_eval_t *eval = asdf_gwcs_eval_create(file, wcs, backend, &err);

       if (!eval) {
           fprintf(stderr, "could not prepare the WCS: %s\n",
                   asdf_gwcs_strerror(err));
           return 1;
       }

       // A 3x3 grid spanning the detector; endpoints are inclusive.
       asdf_gwcs_grid2d_t grid = {
           .x0 = 0.0, .y0 = 0.0,
           .x1 = 4087.0, .y1 = 4087.0,
           .nx = 3, .ny = 3
       };

       // NULL output pointers mean "allocate for me".
       double *ra = NULL;
       double *dec = NULL;

       err = asdf_gwcs_eval_grid2d(eval, &grid, &ra, &dec);

       if (err != ASDF_GWCS_OK) {
           fprintf(stderr, "evaluation failed: %s\n", asdf_gwcs_strerror(err));
           return 1;
       }

       // Results are row-major: index iy * nx + ix.
       for (uint32_t iy = 0; iy < grid.ny; iy++) {
           for (uint32_t ix = 0; ix < grid.nx; ix++) {
               uint32_t idx = iy * grid.nx + ix;
               printf("%12.7f %12.7f", ra[idx], dec[idx]);
           }
           printf("\n");
       }

       free(ra);
       free(dec);
       asdf_gwcs_eval_destroy(eval);
       asdf_value_destroy(found);
       asdf_value_destroy(root);
       asdf_gwcs_destroy(wcs);
       asdf_close(file);
       return 0;
   }

which prints:

.. code:: text

     -90.0130916   65.9742799 -90.1665637   65.9743010 -90.3198486   65.9743569
     -90.0131938   66.0355036 -90.1674743   66.0355096 -90.3215662   66.0355273
     -90.0132976   66.0972133 -90.1683389   66.0972058 -90.3231788   66.0971805

.. note::

   Sky coordinates come back in degrees, with longitude in the range
   ``[-180, 180]``.  Python's GWCS reports the same positions with longitude in
   ``[0, 360)``, so a value of ``-90.013`` here corresponds to ``269.987``
   there.  Add 360 to negative longitudes if you need to match.


Evaluation backends
-------------------

Evaluation is performed by a *backend*, selected by name:

.. code:: c

   const asdf_gwcs_backend_t *backend = asdf_gwcs_backend_get("ast_yaml");

Passing ``NULL`` to `asdf_gwcs_eval_create` selects a default, which is
unambiguous today because ``ast_yaml`` is the only backend built.

.. warning::

   The backend interface (``asdf/gwcs/backend.h``) is **experimental**.  It is
   not yet supported for third-party plugins: the interface is still being
   worked out and will change, so it is deliberately not usable from outside
   the library for now.  Third-party evaluation backends are planned for a
   future release once the design has settled.


The AST backend
---------------

``ast_yaml`` is currently the only backend.  It is built on `Starlink AST`_,
and works in a way worth understanding because it has visible consequences: it
does not walk the parsed `asdf_gwcs_t` structure at all.  Instead it
re-serializes the WCS to YAML in memory---forcing all arrays inline so no
binary blocks are needed---and hands that to AST's YAML channel (``YamlChan``),
which reconstructs it as an ``AstFrameSet``.  Evaluation is then AST's
``astTran2``.

Three things follow from this:

* Anything the serializers drop is invisible to evaluation.  The discarded
  FK4/FK5 ``equinox`` noted in :ref:`reading-writing` is dropped here too.
* A WCS containing a transform this library cannot *serialize* fails when the
  evaluation context is created, not when the file is read.  Reading succeeds;
  ``asdf_gwcs_eval_create`` returns ``NULL`` with
  ``ASDF_GWCS_ERR_TRANSFORM_NOT_SUPPORTED``.
* AST works internally in radians.  When the pipeline's output frame is a
  :external+asdf-wcs-schemas:doc:`celestial frame
  <generated/gwcs/celestial_frame-1.0.0>` the results are converted to degrees
  to match the GWCS convention.
  This conversion is currently keyed on the frame type rather than on real unit
  handling, which is a known rough edge.

See the **AST support** and **Licensing** sections of the README for how AST is
vendored, how to link against an external copy, how to turn it off, and what
its LGPL licensing implies.
