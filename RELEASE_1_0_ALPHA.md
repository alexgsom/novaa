# Nova Native 1.0 Alpha — Windows Studio edition

**Preview; not a stable Windows release.** The independent Nova runtime, bytecode VM and Nova-written self-hosted compiler are retained from 0.9. This release adds a native Windows editor and fixes release tooling.

## New in this cycle

1. **Nova Studio** (`studio/nova_studio.cpp`): native Win32/C++17 source editor (no Tkinter/Python). Supports UTF-8, optional BOM on read, Cyrillic/Polish paths, new/open/save/save-as, drag-and-drop and file argument opening, edit confirmation, F5 Run, F6 Compile, F7 EXE Build, windowed EXE builds, output/error panel, and Stop. Running Nova programs is performed in a worker thread; `Nova Studio` dispatches compile/run work to a separate thread to avoid blocking the editor UI (Windows behavior not yet manually verified).
2. **Self-hosted compiler integration:** Nova Studio launches the existing Nova-authored bytecode compiler and C++ VM directly; it does not shell out to Python or require PowerShell for routine compilation. Windows prebuilt runners are still a prerequisite.
3. **Windows scripts:** `build_windows.ps1` now builds `NovaStudio.exe` with MSVC or MinGW-w64; CMake adds `NovaStudio` on Windows. Per-user `install_windows.ps1` copies Studio when present.
4. **Explorer integration (opt-in):** `register_nova_files.ps1` and matching uninstall script add or remove an “Edit with Nova Studio” context menu without changing the default association.
5. **Windows CI repair:** removed stray executable-path lines from the 0.8 smoke step, added a headless native Studio `--self-test` stage, and included Studio and documentation in the Windows portable CI artifact.
6. **Regression coverage:** additional tests for Unicode-named source files compiled and executed by Nova's self-hosted compiler, Windows distribution wiring, CI smoke script sanity and editor integration checks.

## Architecture and limitations

```
.nova source --[Nova-written novac.nova / novac_selfhost.nbc]--> Nova .nbc bytecode
       |                                                |
    Nova Studio                               Native C++ Nova VM
                                                        |
                                                  packaged .exe
```

- **Self-hosting:** the Nova compiler can recompile itself to byte-for-byte identical bytecode; the executable VM remains written in C++.
- **Python:** not required to compile or run a Nova program, or to use Nova Studio; Python is used **only in the test harness**.
- **Windows build:** the development environment is Linux; Windows MSVC/MinGW and GUI behavior are **not verified here**. No tested `NovaStudio.exe` is included in the source release; use the scripts or CI artifact when its run completes successfully.
- **Editor scope:** no syntax highlighting, debugger, stdin console interaction or UI design tool yet. Some language constructs from Nova 1.2 are not available in the self-hosted bytecode compiler.
- **Security:** Nova apps run with the current user's permissions. The package integrity FNV hash checks corruption only; it is not a signature. Do not run unknown code.

See `studio/README.md` and `INSTALL_WINDOWS.md` for instructions.
