# Nova Native 1.0 Beta 2 — Windows Acceptance Edition

**Goal:** make Windows functionality measurable instead of assuming Windows compatibility from successful Linux builds.

New work:

- Dedicated `.github/workflows/windows-acceptance.yml` on Windows Server 2022 x64 with MSVC, CMake, tests and downloadable artifact.
- Native `NovaStudio.exe --gui-smoke` exercises actual Win32 controls and Unicode without manual clicks.
- `tests/smoke_win32_click.ps1` launches a Nova GUI application, triggers its real Win32 button and verifies that a Nova function ran.
- Real Windows x64 PE32+ verification, subsystem validation, SHA256 per-file manifest and ZIP-level checksum.
- Relocated portable distribution tests the build outside the source directory under Cyrillic/Polish path names.
- `tools/create_windows_portable.ps1` collects all files needed by a Windows user without Python/C++ toolchain.
- `tools/test_windows_portable.ps1` validates the unpacked release after relocation.
- 5 Linux-runnable automated tests for PE verification and Windows pipeline wiring.

**Not yet verified:** actual compilation or GUI interaction on a Windows host. A successfully executed GitHub Actions workflow is needed for that evidence; this package contains code and automation, **not a claimed Windows test result**.

See `WINDOWS_CI.md` for exact instructions. The GUI test is a first acceptance check, not exhaustive GUI coverage or a Windows 10/11 certification.
