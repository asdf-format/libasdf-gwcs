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

   The header is spelled ``frame_celestial.h`` and the C type
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

Most transform types are registered *generically*: their tags are recognized
and they round-trip through the library unchanged, but they have no dedicated
struct beyond the base, so only the base fields are populated.  The types with
their own structs are:

- :external+asdf-transform-schemas:doc:`affine <generated/schemas/affine-1.3.0>`
- :external+asdf-transform-schemas:doc:`compose <generated/schemas/compose-1.2.0>`
- :external+asdf-transform-schemas:doc:`concatenate <generated/schemas/concatenate-1.2.0>`
- :external+asdf-transform-schemas:doc:`constant <generated/schemas/constant-1.4.0>`
- :external+asdf-transform-schemas:doc:`divide <generated/schemas/divide-1.2.0>`
- :external+asdf-transform-schemas:doc:`identity <generated/schemas/identity-1.2.0>`
- :external+asdf-transform-schemas:doc:`polynomial <generated/schemas/polynomial-1.2.0>`
- :external+asdf-transform-schemas:doc:`remap_axes <generated/schemas/remap_axes-1.3.0>`
- :external+asdf-transform-schemas:doc:`rotate_sequence_3d <generated/schemas/rotate_sequence_3d-1.0.0>`
- :external+asdf-transform-schemas:doc:`scale <generated/schemas/scale-1.2.0>`
- :external+asdf-transform-schemas:doc:`shift <generated/schemas/shift-1.2.0>`
- :external+asdf-wcs-schemas:doc:`spherical_cartesian <generated/gwcs/spherical_cartesian-1.1.0>`
- ``fitswcs_imaging``

That is, these are the only transforms currently with explicit implementations
in libasdf-gwcs.  As a historical note on this library's development history,
the primary initial motivation was to support a typical WCS used for Roman
Space Telescope imaging observations, hence this specific subset of transforms.
Though more will be added in future releases.


Writing
-------

Writing mirrors reading, via ``asdf_set_gwcs`` and the ``asdf_set_*`` functions
for the individual types:

.. code:: c

   asdf_file_t *out = asdf_open(NULL);
   asdf_set_gwcs(out, "wcs", wcs);
   asdf_write_to(out, "out.asdf");
   asdf_close(out);

A value read from a file keeps the tag it was read with, so a straight
read-then-write round trip preserves schema versions rather than silently
upgrading them.


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
