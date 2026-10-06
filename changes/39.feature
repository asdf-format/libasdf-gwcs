The ``inverse`` property of transforms is now read into
``asdf_gwcs_transform_t.inverse`` and written back out on serialization.

Previously any declared inverse was silently dropped, so a GWCS read and
re-written (including the copy passed to the AST evaluation backend) lost
it.
