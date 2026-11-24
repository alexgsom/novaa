# Nova Windows regression fixes — 11

This release candidate builds upon Fix 10. The Windows CI regression log had 222 tests,
3 failures and 33 skips. This patch addresses those three failures:

1. `src/nova_native.cpp`: reject Windows rooted-but-not-absolute import paths
   (e.g. `/tmp/secret.nova`), before joining to the script directory.
2. `tests/test_studio10.py`: decode compiled Nova UTF-8 output as UTF-8 on Windows,
   not the host's cp1252 locale.
3. `tests/test_studio_beta1.py`: allow C++ Unicode UCN escape spellings for
   Polish Ł and Ż in the header source. The native text tests still test behavior.

Windows MSVC CI must be rerun to confirm these changes on the hosted runner.
