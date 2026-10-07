Added `asdf_gwcs_eval_invert`, which returns a new evaluation context that
evaluates the inverse of an existing one (e.g. world to pixel coordinates).

Declared ``inverse`` transforms in the file are used as given.  If the backend
cannot invert a WCS it returns ``NULL`` with the new error code
`ASDF_GWCS_ERR_NO_INVERSE`.
