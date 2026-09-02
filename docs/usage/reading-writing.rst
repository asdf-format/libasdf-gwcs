.. _reading-writing:

Reading and writing GWCS objects
================================

libasdf-gwcs registers extension types for the GWCS schemas with libasdf.  Once
your program links against both libasdf-gwcs and libasdf (see
:ref:`linking against both libraries <linking>` for the linker and pkg-config
invocations), any ASDF file read through libasdf will
recognize GWCS tags and deserialize them into C structs instead of leaving them
as generic mappings.

Three families of schema are implemented:

* ``tag:stsci.edu:gwcs/`` --- the
  :external+asdf-wcs-schemas:doc:`WCS object <generated/gwcs/wcs-1.1.0>`
  itself, its :external+asdf-wcs-schemas:doc:`steps
  <generated/gwcs/step-1.1.0>`, and its coordinate frames.
* ``tag:stsci.edu:asdf/transform/`` --- the
  :external+asdf-transform-schemas:doc:`transform (model) types <transforms>`.
* ``tag:stsci.edu:asdf/coordinates/frames/`` --- the
  :external+asdf-coordinates-schemas:doc:`astropy reference frames <frames>`
  attached to celestial frames.

.. note::

   Upstream publishes a page per schema, but not always for the newest
   version.  Schema links throughout these pages point at the most recent
   version that is published, which may be older than the version
   libasdf-gwcs reads and writes.


Getting a WCS out of a file
---------------------------

The top-level type is `asdf_gwcs_t`.  If you know where the WCS lives in the
tree, read it by path:

.. code:: c

   asdf_gwcs_t *wcs = NULL;

   if (asdf_get_gwcs(file, "roman/meta/wcs", &wcs) == ASDF_VALUE_OK) {
       /* ... */
       asdf_gwcs_destroy(wcs);
   }

If you don't, search for one.  ``asdf_value_is_gwcs`` has exactly the signature
libasdf's tree traversal expects, so it can be used directly as a predicate:

.. code:: c

   asdf_value_t *root = asdf_get_value(file, "");
   asdf_value_t *found = asdf_value_find(root, asdf_value_is_gwcs);

   if (found) {
       asdf_gwcs_t *wcs = NULL;
       asdf_value_as_gwcs(found, &wcs);
       printf("found a WCS at %s\n", asdf_value_path(found));
   }

   asdf_value_destroy(found);
   asdf_value_destroy(root);

.. note::

   Reading and writing returns libasdf's `asdf_value_err_t` (compare against
   ``ASDF_VALUE_OK``), whereas the evaluation API in :ref:`evaluation` returns
   `asdf_gwcs_err_t` (compare against ``ASDF_GWCS_OK``).  These are two
   distinct enumerations and their values are **not** interchangeable.


The generated API
-----------------

Every extension type is declared with libasdf's ``ASDF_DECLARE_EXTENSION``
macro, which generates a uniform family of eleven exported functions.  For the
top-level WCS (registered as ``gwcs``) these are:

.. code:: c

   asdf_value_err_t asdf_get_gwcs(asdf_file_t *, const char *path, asdf_gwcs_t **out);
   asdf_value_err_t asdf_set_gwcs(asdf_file_t *, const char *path, const asdf_gwcs_t *);
   bool             asdf_is_gwcs(asdf_file_t *, const char *path);
   asdf_value_err_t asdf_value_as_gwcs(asdf_value_t *, asdf_gwcs_t **out);
   bool             asdf_value_is_gwcs(asdf_value_t *);
   asdf_value_t    *asdf_value_of_gwcs(asdf_file_t *, const asdf_gwcs_t *);
   asdf_gwcs_t     *asdf_gwcs_copy(asdf_file_t *, const asdf_gwcs_t *src);
   bool             asdf_gwcs_copy_into(asdf_file_t *, const asdf_gwcs_t *src, asdf_gwcs_t *dst);
   asdf_gwcs_t    **asdf_gwcs_array_copy(asdf_file_t *, const asdf_gwcs_t **src);
   void             asdf_gwcs_deinit(asdf_gwcs_t *);
   void             asdf_gwcs_destroy(asdf_gwcs_t *);

The same pattern holds for every other type in this library, with the
registered name substituted: ``asdf_get_gwcs_step``, ``asdf_get_gwcs_frame2d``,
``asdf_get_gwcs_shift``, and so on.  Only the exceptions are documented
individually in the :ref:`API reference <api>`.

``deinit`` releases the fields an object owns but not the object's own
storage; ``destroy`` does both.  Use ``deinit`` when the struct is embedded in
something you allocated yourself.


Walking the pipeline
--------------------

A WCS is a list of steps.  Each step pairs a coordinate frame with the
transform that maps *out of* that frame into the next step's frame:

.. code:: c

   for (uint32_t idx = 0; idx < wcs->n_steps; idx++) {
       const asdf_gwcs_step_t *step = &wcs->steps[idx];

       printf("%s -> %s\n",
              step->frame->name,
              step->transform ? asdf_gwcs_transform_tag(step->transform)
                              : "(end of pipeline)");
   }

.. important::

   The **last step has no transform**.  Its ``transform`` member is ``NULL``,
   because that step exists only to name the frame the pipeline ends in.  Any
   loop over the steps must handle this.

`asdf_gwcs_transform_tag` returns the full YAML tag of a transform.  For a
transform read from a file this is the tag *as it appeared in that file*,
including its schema version---each transform type is registered for several
versions, so this is not necessarily the same as the version the library would
write.

When the version is beside the point, `asdf_gwcs_transform_type_name` gives
just the schema name: ``affine``, ``compose``, ``fitswcs_imaging``.  Do not
confuse it with the ``name`` member of `asdf_gwcs_transform_t`, which is an
optional label the file's author may have given to one particular transform.
The type name says *what kind* of transform it is and is the same for every
transform of that type; ``name`` is usually ``NULL``.


Frames
------

`asdf_gwcs_frame_t` is a base struct carrying a ``type`` discriminant and a
``name``.  As with transforms, ``name`` is the file author's label for this
particular frame, while `asdf_gwcs_frame_type_name` reports the schema name of
its type---``frame``, ``frame2d`` or ``celestial_frame``.

Downcast on the discriminant to reach the concrete type:

.. code:: c

   switch (frame->type) {
   case ASDF_GWCS_FRAME_2D: {
       const asdf_gwcs_frame2d_t *f2d = (const asdf_gwcs_frame2d_t *)frame;
       printf("axes: %s, %s\n", f2d->axes_names[0], f2d->axes_names[1]);
       break;
   }
   case ASDF_GWCS_FRAME_CELESTIAL: {
       const asdf_gwcs_frame_celestial_t *cel =
           (const asdf_gwcs_frame_celestial_t *)frame;
       /* Which astropy frame this is: "icrs", "fk5", "galactic", ... */
       printf("%s\n", asdf_gwcs_coordinate_frame_type_name(cel->reference_frame));
       break;
   }
   default:
       break;
   }

Most reference frames carry no ``frame_attributes`` and are represented
directly as `asdf_gwcs_baseframe_t`.  The equinox-based ones are the
exception: `asdf_gwcs_fk5_t` adds a required ``equinox``, and
`asdf_gwcs_fk4_t`--shared by
:external+asdf-coordinates-schemas:doc:`FK4 <generated/schemas/frames/fk4-1.0.0>`
and
:external+asdf-coordinates-schemas:doc:`FK4NoETerms <generated/schemas/frames/fk4noeterms-1.0.0>`,
whose schemas are identical--adds an optional ``obstime`` alongside it.  Both
are :external+asdf-standard:doc:`time/time
<generated/stsci.edu/asdf/time/time-1.2.0>` values, read as libasdf's
`asdf_time_t`, and both round-trip:

.. code:: c

   if (cel->reference_frame->type == ASDF_GWCS_COORDINATE_FRAME_FK5) {
       const asdf_gwcs_fk5_t *fk5 = (const asdf_gwcs_fk5_t *)cel->reference_frame;
       printf("equinox %s\n", fk5->equinox->value);
   }

.. note::

   The header is spelled :doc:`frame_celestial.h </api/frame_celestial.h>`
   and the C type
   `asdf_gwcs_frame_celestial_t`, but the corresponding schema tag has the
   words the other way round:
   :external+asdf-wcs-schemas:doc:`gwcs/celestial_frame
   <generated/gwcs/celestial_frame-1.0.0>`.

   This spelling change is just a matter of keeping the API consistent (all
   frame types prefixed with ``asdf_gwcs_frame_``).


Transforms
----------

`asdf_gwcs_transform_t` corresponds to the
:external+asdf-transform-schemas:doc:`base transform schema
<generated/schemas/transform-1.2.0>`, from which every transform type derives,
and is the base of a tagged union.  Concrete transform types embed it via the
``ASDF_GWCS_TRANSFORM_BASE`` macro, which makes the
base fields directly accessible on the derived struct as well as through an
explicit ``.base``:

.. code:: c

   const asdf_gwcs_shift_t *shift = (const asdf_gwcs_shift_t *)transform;

   printf("%u -> %u, offset %g\n",
          shift->n_inputs,        /* same as shift->base.n_inputs */
          shift->n_outputs,
          shift->offset);

Most transform types are registered *generically*: their tags are recognized
and they round-trip through the library unchanged, but they have no dedicated
struct beyond the base, so only the base fields are populated.  The ones that
do have their own struct are listed, with a one-line summary of each, under
:ref:`concrete-transforms`; the generic ones are tabulated there too, under
:ref:`generic-transforms`.

As a historical note on this library's development, the primary initial
motivation was to support a typical WCS used for Roman Space Telescope imaging
observations, hence that particular subset of transforms.  More will be added
in future releases.

Composite transforms
~~~~~~~~~~~~~~~~~~~~

Some transforms are built out of other transforms.
:external+asdf-transform-schemas:doc:`compose <generated/schemas/compose-1.2.0>`
and
:external+asdf-transform-schemas:doc:`concatenate <generated/schemas/concatenate-1.2.0>`
hold an ordered list of them;
:external+asdf-transform-schemas:doc:`divide <generated/schemas/divide-1.2.0>`
holds transforms applied for a numerator and a denominator.  Since each stores its
sub-transforms differently in their associated C structs; thus they are reached
through an iterator rather than through any one struct member:

.. code:: c

   asdf_gwcs_transform_iter_t *iter = asdf_gwcs_transform_iter_init(transform, 0);

   while (asdf_gwcs_transform_iter_next(&iter))
       printf("%s%s\n",
              iter->role ? iter->role : "",
              asdf_gwcs_transform_type_name(iter->value));

The iterator follows the same conventions as libasdf's
``asdf_sequence_iter``: it is freed automatically when iteration is
exhausted, and `asdf_gwcs_transform_iter_destroy` releases it if you break out
early.  ``iter->size`` is the total number of sub-transforms, also available
on its own from `asdf_gwcs_transform_n_children`, which returns ``0`` for a
transform that is not a composite.  Iterating a non-composite is not an error;
it simply yields nothing.

``iter->role`` names the property a sub-transform came from (e.g. ``numerator``
and ``denominator``) and is ``NULL`` when the parent holds an ordered list whose
elements have no individual names, as with ``compose``.

Sub-transforms are owned by their parent.  They live as long as it does and
must not be destroyed individually.

Descending into nested transforms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The second argument to `asdf_gwcs_transform_iter_init` is how many levels
below ``transform`` the walk may descend.  ``0``, as above, visits only its
immediate sub-transforms.  Anything higher visits theirs as well, depth-first
and pre-order, so a transform is always reported before anything nested inside
it, and `ASDF_GWCS_DEPTH_UNLIMITED` walks the whole tree:

.. code:: c

   asdf_gwcs_transform_iter_t *iter = asdf_gwcs_transform_iter_init(
       transform, ASDF_GWCS_DEPTH_UNLIMITED);

   while (asdf_gwcs_transform_iter_next(&iter))
       printf("%*s%s\n", iter->depth * 2, "",
              asdf_gwcs_transform_type_name(iter->value));

``iter->depth`` is ``0`` for an immediate sub-transform, ``1`` for one nested
a level below that, and so on, which is what makes indenting by nesting level
a one-liner.  ``index`` and ``size`` always describe the current transform's
position among its *immediate* siblings, so ``index + 1 == size`` identifies
the last child at any level.

Real pipelines nest deeply---the ``roman_l2_wcs.asdf`` test fixture reaches
nine levels and 80 transforms---so an unlimited walk can produce far more than
expected.


Writing
-------

Writing mirrors reading, via ``asdf_set_gwcs`` and the ``asdf_set_*`` functions
for the individual types.  Writing back a WCS that was read from a file is a
two-liner:

.. code:: c

   asdf_file_t *out = asdf_open(NULL);
   asdf_set_gwcs(out, "wcs", wcs);
   asdf_write_to(out, "out.asdf");
   asdf_close(out);

A value read from a file keeps the tag it was read with, so a straight
read-then-write round trip preserves schema versions rather than silently
upgrading them.


Building a WCS from scratch
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Nothing requires a WCS to have come from a file.  Every type in this library is
a plain struct, so a pipeline can be assembled in memory and written out.

The example below builds the smallest useful WCS: two steps mapping detector
pixels to ICRS sky coordinates through a
:c:struct:`FITS imaging transform <asdf_gwcs_fits_t>`.  Note that none of it is
heap-allocated: ``asdf_set_gwcs`` serializes into the file's tree, so these can
all be ordinary stack values, and there is nothing to destroy afterwards.

.. code:: c
   :test: test-gwcs-write
   :fixture: temp:wcs-from-scratch.asdf

   #include <stdio.h>
   #include <asdf.h>
   #include <asdf/gwcs/gwcs.h>

   int main(int argc, char **argv) {
       const char *path = argc > 1 ? argv[1] : "out.asdf";

       // Step 0's frame: the detector, in pixels.
       asdf_gwcs_frame2d_t detector = {
           .base = {.type = ASDF_GWCS_FRAME_2D, .name = "detector"},
           .axes_names = {"x", "y"},
           .axes_order = {0, 1},
           .unit = {"pixel", "pixel"},
           .axis_physical_types = {"custom:x", "custom:y"},
       };

       // The transform out of the detector frame.  A fitswcs_imaging carries
       // the familiar FITS keywords plus the projection to apply.
       asdf_gwcs_transform_t gnomonic = {.type = ASDF_GWCS_TRANSFORM_GNOMONIC};
       asdf_gwcs_fits_t fits = {
           .base = {.type = ASDF_GWCS_TRANSFORM_FITSWCS_IMAGING},
           .crpix = {1024.5, 1024.5},
           .crval = {5.63, -72.05},
           .cdelt = {-1.0e-5, 1.0e-5},
           .pc = {{1.0, 0.0}, {0.0, 1.0}},
           .projection = &gnomonic,
       };

       // Step 1's frame: the sky.  A celestial frame names the astropy
       // reference frame its coordinates are expressed in.
       asdf_gwcs_baseframe_t icrs = {.type = ASDF_GWCS_COORDINATE_FRAME_ICRS};
       asdf_gwcs_frame_celestial_t sky = {
           .base = {.type = ASDF_GWCS_FRAME_CELESTIAL, .name = "world"},
           .axes_names = {"lon", "lat", NULL},
           .axes_order = {0, 1, 0},
           .unit = {"deg", "deg", NULL},
           .axis_physical_types = {"pos.eq.ra", "pos.eq.dec", NULL},
           .reference_frame = &icrs,
       };

       // The last step has no transform: it only names the frame the
       // pipeline ends in.
       asdf_gwcs_step_t steps[2] = {
           {.frame = (asdf_gwcs_frame_t *)&detector,
            .transform = (const asdf_gwcs_transform_t *)&fits},
           {.frame = (asdf_gwcs_frame_t *)&sky, .transform = NULL},
       };

       uint64_t shape[2] = {2048, 2048};
       asdf_gwcs_t wcs = {
           .name = "example",
           .pixel_ndim = 2,
           .pixel_shape = shape,
           .n_steps = 2,
           .steps = steps,
       };

       asdf_file_t *file = asdf_open(NULL);

       if (asdf_set_gwcs(file, "wcs", &wcs) != ASDF_VALUE_OK) {
           fprintf(stderr, "could not build the WCS\n");
           return 1;
       }

       if (asdf_write_to(file, path) != 0) {
           fprintf(stderr, "could not write %s\n", path);
           return 1;
       }

       asdf_close(file);
       printf("wrote %s\n", path);
       return 0;
   }

The YAML that lands in the file is the same shape the reading examples walk:

.. code:: yaml

   wcs: !<tag:stsci.edu:gwcs/wcs-1.4.0>
     name: example
     pixel_shape:
     - 2048
     - 2048
     steps:
     - !<tag:stsci.edu:gwcs/step-1.3.0>
       frame: !<tag:stsci.edu:gwcs/frame2d-1.2.0>
         name: detector
         axes_names: [
           x,
           y
           ]
         axes_order: [
           0,
           1
           ]
         unit: [
           pixel,
           pixel
           ]
         axis_physical_types: [
           custom:x,
           custom:y
           ]
       transform: !<tag:stsci.edu:gwcs/fitswcs_imaging-1.0.0>
         crpix: !core/ndarray-1.1.0
           data: [
             1024.5,
             1024.5
             ]
           datatype: float64
         # crval, cdelt and pc follow in the same form
         projection: !transform/gnomonic-1.4.0 {}
     - !<tag:stsci.edu:gwcs/step-1.3.0>
       frame: !<tag:stsci.edu:gwcs/celestial_frame-1.2.0>
         name: world
         axes_names: [
           lon,
           lat
           ]
         axes_order: [
           0,
           1
           ]
         unit: [
           deg,
           deg
           ]
         axis_physical_types: [
           pos.eq.ra,
           pos.eq.dec
           ]
         reference_frame: !<tag:astropy.org:astropy/coordinates/frames/icrs-1.3.0>
           frame_attributes: {}
       transform: null

A few things are worth knowing when building a WCS this way:

* Frames and transforms are referenced, not copied, while you assemble the
  pipeline, so everything a `asdf_gwcs_t` points at must outlive the
  ``asdf_set_gwcs`` call.  Stack values in the same scope, as above, satisfy
  that.
* ``ctype`` on `asdf_gwcs_fits_t` is *derived*, not set by you.  It is filled
  in on read, from the terminal frame's ``axis_physical_types``, which is why
  those matter even though they look like documentation: get them wrong and
  the CTYPEs come out wrong or empty.  See :ref:`known-limitations` for why it
  works that way.
* The FITS keyword arrays are written as ``core/ndarray`` values rather than
  bare YAML sequences, but stored *inline*, so a file built this way stays
  YAML-only with no binary blocks.  The other transforms that carry arrays,
  e.g. ``affine`` and ``polynomial``, do the same.  Evaluation forces every
  array inline regardless before handing the tree to AST
  (see :ref:`evaluation`), since a WCS read from someone else's file may well
  use binary blocks.


.. _known-limitations:

Known limitations
-----------------

* Supergalactic and BarycentricMeanEcliptic reference frames are not yet
  registered.
* The ``inverse``, ``fixed``, ``bounds`` and ``input_units_equivalencies``
  members of `asdf_gwcs_transform_t` are laid out for future ABI compatibility
  but are not yet populated.
* `asdf_gwcs_fits_t`'s ``ctype`` members are filled in only when the whole
  containing WCS is read (with ``asdf_get_gwcs``); reading a
  ``fitswcs_imaging`` transform on its own leaves them ``NULL``.

  This one is inherent to the schema rather than an unfinished corner.  A
  CTYPE such as ``RA---TAN`` is two halves from two places: the coordinate
  type (``RA``) is derived from the ``axis_physical_types`` of the WCS's
  *output* frame, whose UCD1+ terms map onto it (``pos.eq.ra`` gives ``RA``,
  ``pos.galactic.lon`` gives ``GLON``), while the projection code (``TAN``)
  comes from the transform's own ``projection``.  A ``fitswcs_imaging`` object
  carries the second half but not the first: the output frame is a sibling
  step of the WCS, not part of the transform, so on its own the transform
  genuinely does not contain enough information to name its own axes.

  This could arguably also be considered a shortcoming in the
  ``fitswcs_imaging`` schema.
