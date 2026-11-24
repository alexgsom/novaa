# Nova Studio 4.2 — перевірений статус

## PASS (Linux)
- 10/10 C++ CTest (Release)
- 52 Python source integration tests: 49 passed, 3 skipped
- HTML preview rendered with Chromium; `design/AURA_PREVIEW.png` is a screenshot of the concept, not the native Win32 application.

## NEEDS WINDOWS VERIFICATION
- MSVC / CMake Windows compile of `studio/nova_studio.cpp`
- Win32 `NovaStudio.exe --self-test` and `--gui-smoke`
- Actual visual controls on Windows, 100%/125%/150% scaling, tab rendering, sidebar reflow

This package includes source and a patch, but **no tested Windows EXE**.
