# Nova Studio 3.0 Preview — Update 19 | Build status

Verified in Linux environment:

- CMake C++17 build: PASS.
- CTest: **9/9 passed** (includes new editor+project patch parser tests and designer templates/snapping tests).
- `python -m unittest discover -s tests -p 'test_studio*.py' -v`: **42 total: 38 passed, 4 skipped** (platform-dependent binary tests).
- Nova self-hosted compiler compiled `examples/studio_next/hello_next.nova`; bytecode executed successfully.
- Trinity EN, UK, PL samples produced identical SHA256 compiled bytecodes.

NOT verified here: Win32/MSVC compile, real designer GUI / minimap paint / AI dialogs, external OpenAI API calls. Windows Actions must confirm build and GUI smoke. Multi-file project AI runs compile-only checks, not the project's entire test suite.

See `NOVA_STUDIO_3_UPDATE_19_UA.md` for controls, privacy constraints and installation.
