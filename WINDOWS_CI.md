> **Update 14:** CMake та обидва workflows запускають `nova-studio-next14-tests`, а `NovaStudio.exe --gui-smoke` перевіряє існування та перемикання вкладок. Новий Windows Artifact потребує успішного зеленого запуску.

# Nova Native 1.0 Beta 2 — Windows CI & acceptance guide

This release includes a **real Windows CI workflow** in `.github/workflows/windows-acceptance.yml`.
It has **not been executed yet in this development environment**. It runs only after this
project is committed to a GitHub repository with Actions enabled.

## First Windows build: 5 steps

1. Create a GitHub repository (private is fine). Upload **the contents of this project folder**, keeping `.github/workflows/windows-acceptance.yml` at the repository root. Do not upload only the ZIP to GitHub as a single file; GitHub Actions needs the unpacked source tree.
2. Open the repository's **Actions** tab, enable Actions if prompted, and select **Nova Windows x64 Acceptance**.
3. Select **Run workflow** on the default branch, then open the new workflow run. Pushes and pull requests can also trigger the checks.
4. Wait for the `windows-x64` job result. A green check means the *whole pipeline*, including native Win32 button events, passed on a Microsoft-hosted Windows Server 2022 x64 runner. A red X means something still needs fixing; read the failed step logs. **Do not treat workflow configuration or a skipped run as Windows verification.**
5. On a green run, download **`nova-windows-x64`** from the run's **Artifacts** section. Extract the artifact to obtain `Nova-Windows-x64-Beta2.zip` and the `.sha256` file. Extract this ZIP and use the `Nova` folder inside it. No Python or compiler is required to **run** the bundled Nova binaries or build another program from `.nova` with the prebuilt toolchain.

Artifacts are retained for 21 days by workflow configuration (subject to GitHub settings). GitHub Actions availability, usage and billing are subject to your GitHub plan and repository settings.

## What Windows CI verifies

- MSVC compiles `nova-native.exe`, `nova-windowed.exe`, and `NovaStudio.exe`.
- PE inspector confirms **Windows AMD64 PE32+**, console vs GUI subsystems (not Linux ELF masquerading as EXE).
- CMake/MSVC also builds and runs the independent native Studio text tests.
- `NovaStudio.exe --self-test` checks UTF-8/search/replace/tokenization; `--gui-smoke` creates real Win32 editor controls, writes Unicode, sends `WM_COMMAND`, tests layout and closes.
- Self-hosting: stage-0 Nova compiler compiles `novac.nova` to A; compiled A compiles the same source to B; their SHA256 must match; bundled compiler EXE must produce the same test bytecode.
- `nova doctor`, compilerless `nova.ps1 build`, integrity, Unicode paths, single-file programs, and bundled nested `.nova` modules.
- Real GUI event: Windows launches a **windowed Nova EXE**, locates its native `BUTTON`, sends `BM_CLICK` through `user32.dll`, and verifies a Nova callback saved a UTF-8 marker before closing the window. This is a genuine event test, **not** a simulated interpreter stub.
- Full Python-driven regression test suite (**Python is only used by CI to test Nova; the Nova binaries do not embed Python**).
- The prepared distribution is copied to a **different folder with Ukrainian/Polish characters** and tested *again* (Nova Studio, doctor, build, console and GUI callback).
- Only after all checks pass does CI produce the ZIP and SHA256 checksum. On test failure the release artifact is not published; CI test logs identify the cause.

## Quick manual use of downloaded Windows ZIP

In a Windows PowerShell terminal opened in the extracted `Nova` folder:

```powershell
.\nova.ps1 doctor
.\bin\NovaStudio.exe
.\nova.ps1 build -Source hello.nova -Output Hello.exe
.\Hello.exe
```

Create `hello.nova` containing `покажи "Привіт із Nova!"` first, or use your own `.nova` file.
For Windows 10/11, manually confirm that the editor opens, menus work, files save in UTF-8, and a compiled GUI application responds to clicks. GitHub-hosted **Windows Server 2022 is not an identical replacement for Windows 10/11 desktop validation**.

## Troubleshooting

- **C++ compilation error**: open the `Build real Windows x64...` step; logs contain MSVC diagnostics. Do not distribute failed artifacts.
- **Nova Studio `--gui-smoke` fails**: inspect error code 21–26 and the Win32 runner desktop availability.
- **GUI click test fails**: check whether the CI runner exposes a working desktop/window station and whether the callback generated `nova_ci_click.txt`. The test intentionally fails rather than skipping this critical case.
- **Portable layout test fails**: verify that `bin/nova-native.exe`, `bin/nova-windowed.exe`, `bin/NovaStudio.exe` and `compiler/novac_selfhost.nbc` retain their relative paths.
- **`sqlite3.dll`**: native SQLite apps may still need this DLL installed beside the EXE. It is not shipped in the portable package.
- **Unknown publisher / SmartScreen warning**: the created EXEs are unsigned developer builds, not production-certified Windows installers. Do not bypass security warnings for files you do not trust.

## Current verified status

Local Linux environment: source checkout and C++ portable tests available; Windows GUI/CMake/MSVC cannot be executed here. **The Windows CI run status is not known until the workflow is actually triggered on GitHub.**
