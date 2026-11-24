# Nova Windows regression fix 11

Files to copy into your existing novaa repository at the same paths:
- `src/nova_native.cpp` — reject root-relative import paths on Windows
- `tests/test_studio10.py` — explicitly decode UTF-8 program output
- `tests/test_studio_beta1.py` — recognize both native and escaped Unicode character literals

Keep the rest of the repository unchanged. Commit and push via GitHub Desktop.
