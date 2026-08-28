.. _api:

API documentation
=================

The API reference is organized according to the header files each documented
member comes from, grouped by topic.

Everything is reachable through the single umbrella header:

.. code:: c

   #include <asdf/gwcs/gwcs.h>


.. _generated-api:

The generated extension API
---------------------------

Most of libasdf-gwcs's exported symbols are not written out by hand.  Each
extension type is declared with libasdf's ``ASDF_DECLARE_EXTENSION(name, type)``
macro, which generates a uniform family of **eleven** exported functions for it:

.. code:: c

   asdf_value_err_t asdf_get_<name>(asdf_file_t *, const char *path, <type> **out);
   asdf_value_err_t asdf_set_<name>(asdf_file_t *, const char *path, const <type> *);
   bool             asdf_is_<name>(asdf_file_t *, const char *path);
   asdf_value_err_t asdf_value_as_<name>(asdf_value_t *, <type> **out);
   bool             asdf_value_is_<name>(asdf_value_t *);
   asdf_value_t    *asdf_value_of_<name>(asdf_file_t *, const <type> *);
   <type>          *asdf_<name>_copy(asdf_file_t *, const <type> *src);
   bool             asdf_<name>_copy_into(asdf_file_t *, const <type> *src, <type> *dst);
   <type>         **asdf_<name>_array_copy(asdf_file_t *, const <type> **src);
   void             asdf_<name>_deinit(<type> *);
   void             asdf_<name>_destroy(<type> *);

Because these are produced by the preprocessor they do not appear in the pages
below, which show only the declarations as written in the headers.  Read this
section as applying to every registered type: the WCS itself (``gwcs``), steps
(``gwcs_step``), each frame type, and each transform type.

Two conventions are worth restating:

* ``deinit`` releases what an object owns but not the object's own storage;
  ``destroy`` does both.  Use ``deinit`` for a struct embedded in storage you
  allocated yourself.
* ``asdf_value_of_<name>`` and ``asdf_set_<name>`` write the type's *preferred*
  tag, which is the first one it was registered under, unless the object was
  read from a file, in which case the tag it was read with is preserved.

Errors come from two distinct enumerations, and they are not interchangeable:
reading and writing return libasdf's ``asdf_value_err_t`` (success is
``ASDF_VALUE_OK``), while evaluation returns `asdf_gwcs_err_t` (success is
``ASDF_GWCS_OK``).


Reference
---------

.. toctree::
  :maxdepth: 2

  core
  wcs
  frames
  transforms
  evaluation
