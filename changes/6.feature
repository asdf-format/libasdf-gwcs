Added a pluggable evaluation backend interface (``asdf_gwcs_eval_t`` and
friends), and integrated Starlink AST as the first built-in backend, so that
a WCS read from an ASDF file can actually be evaluated.
