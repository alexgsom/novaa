# Nova Studio Next Preview 4 — Update 16/17/18 Build Status

**Supported verification in current build environment:**

- Linux GNU C++17 compile via CMake: PASS.
- CMake/CTest portable native test suites: **7 of 7 PASS**.
- Python tests matching `test_studio*.py`: **37 of 37 PASS**, includes the Trinity bytecode-equivalence test.
- Three Nova Trinity source files compile with the Nova self-hosted compiler into identical `.nbc` files and run with the same output.

**Still required:** Windows MSVC build, real Win32 GUI smoke, Windows packaging, and optional OpenAI API smoke using a user-supplied key. This source ZIP does not contain Windows EXEs. A compile-only AI Proof pass does not establish behavior correctness or passing project tests.

Complete setup instructions: `NOVA_STUDIO_NEXT_16_18_UA.md`.
