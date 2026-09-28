Added `asdf_gwcs_eval_copy`, which makes an independent copy of an evaluation
context.

This is handled in a thread-safe manner with the AST backend, making it
possible for user code to copy an evaluation context safely between threads.
This is necessary at least for the AST backend since otherwise the context
can only be used from the thread which created it.
