# Nova Native 1.0 Beta 2 — Build and test status

## Confirmed in this Linux development environment

- **222 / 222 Python-driven automated regression tests passed**, including five new Windows acceptance/PE verifier tests and the existing Nova native runtime and Studio text-engine checks. Python is a test runner, not a dependency of the native Nova runtime.
- **CMake native Studio text tests pass with both GCC and Clang** on Linux.
- **The Nova compiler successfully compiled its own source to bytecode that is byte-for-byte identical to the bundled compiler bytecode.**
- The Nova-authored compiler accepted and compiled the new `tests/windows_gui_click.nova` example.
- GitHub Actions YAML parses and defines separate Linux and Windows workflows.

## Not yet confirmed

- Windows MSVC or CMake Windows compilations, Nova Studio Win32 behavior or Win32 button acceptance have **not been executed** here. They are configured in `.github/workflows/windows-acceptance.yml` and must be run on GitHub-hosted Windows to confirm.
- No ready-to-use Windows EXE is included in this source archive. **A Windows ZIP is created by CI only after the actual Windows tests pass.**
- Windows 10/11 desktop functionality still requires user/manual testing even after a successful Windows Server 2022 GitHub runner.

## How to complete Windows validation

1. Commit the unpacked project, preserving `.github/workflows/windows-acceptance.yml`.
2. In GitHub Actions, select `Nova Windows x64 Acceptance` and choose `Run workflow`.
3. Inspect Windows job logs. Only after a green check, download `nova-windows-x64` (the tested Windows ZIP and SHA256).

See `WINDOWS_CI.md` for complete instructions and troubleshooting.
