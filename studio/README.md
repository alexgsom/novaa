## Nova Studio Next Preview 3 (Update 15)

Нове: кнопка **App Designer** (візуальне компонування, генерація .nova у нову вкладку), поле **Send input** для інтерактивного Run / ask, **Create Nova app (AI)** (ручний дозвіл на відкриття чернетки). Деталі й обмеження: `../NOVA_STUDIO_NEXT_15_UA.md`.

# Nova Studio Next Preview 2 — Windows x64

**Справжні вкладки, автодоповнення, навігація по помилках, AI-перегляд змін і темний трьохпанельний інтерфейс.**

Див. [`../NOVA_STUDIO_NEXT_14_UA.md`](../NOVA_STUDIO_NEXT_14_UA.md).
Повна актуальна інструкція: [`../NOVA_STUDIO_NEXT_13_UA.md`](../NOVA_STUDIO_NEXT_13_UA.md).

---

# Nova Studio 1.0 Beta 1 — Windows x64

Nova Studio is a **native C++17/Win32 editor** for `.nova`. It uses the **Nova-written self-hosted compiler** (`compiler/novac_selfhost.nbc`) plus the Nova C++ bytecode virtual machine. **No Python or Tkinter is needed** to use the application once its Windows binaries have been built or obtained.

## Editing features

- **Syntax colors** for Nova keywords in English, Ukrainian and Polish, numbers, strings and comments. Uses the standard Windows RichEdit component (`Msftedit.dll`); falls back to a plain Edit control if RichEdit is unavailable. Highlighting is throttled and disabled for files over 128 Ki UTF-16 characters to avoid UI stalls.
- **Search** `Ctrl+F`, **Replace** `Ctrl+H`, **Find next** `F3`, **Find previous** `Shift+F3`. Search supports case sensitivity, whole words, wrap-around, and Polish/Ukrainian letters; Replace All has a 4 Mi-character safety limit. Replace All currently resets the editor undo history; this limitation will be addressed later.
- **Toggle line comments** `Ctrl+K` (Edit menu): comments or uncomments the current line or all selected lines, preserving indentation and Windows line endings, with undo support.
- **Cursor status** displays line and column; edit-save is UTF-8 without BOM, and loading also handles UTF-8 with BOM. Invalid UTF-8 and embedded NUL bytes are rejected.
- **New** `Ctrl+N`, **Open** `Ctrl+O`, **Save** `Ctrl+S`, Save As, file drag-and-drop, and opening a `.nova` path supplied as an argument. Unsaved changes trigger confirmation.
- **Toolbar** with Save, Run, Compile, Build EXE and Stop buttons (the same actions as the menus and shortcuts).
- **Run** `F5`, **Compile** `F6`, **Build EXE** `F7`, **Stop** `Shift+F5`, and Build windowed EXE in the Nova menu. Compile/run/build are dispatched to a worker thread. Program stdin is connected to `NUL`; terminal-interactive Nova programs are not supported in Studio yet.
- **Safer EXE rebuild:** a new EXE is packed to a separate staged file in the same directory and atomically replaces an existing destination only on successful packing, after the user confirms the output name in the Windows file dialog. A locked/running destination EXE may block the replacement, leaving the old file intact.

## Build for Windows

Open a **Visual Studio Developer PowerShell** with MSVC or an MSYS2 MinGW-w64 shell with `g++.exe`:

```powershell
.\build_windows.ps1 -Compiler MSVC
.\bin\NovaStudio.exe
```

To run the headless native Studio checks (UTF-8, search, replace and tokenization):

```powershell
$p=Start-Process -FilePath .\bin\NovaStudio.exe -ArgumentList '--self-test' -Wait -PassThru
$p.ExitCode
```

A successful result is `0`. The CMake build includes the real portable C++ search/highlighting test as `nova-studio-text-tests`.

**Distribution layout:** `bin/NovaStudio.exe`, `bin/nova-native.exe`, `bin/nova-windowed.exe`, and `compiler/novac_selfhost.nbc` share one Nova installation root. Only the first Windows-runtime compilation needs MSVC/MinGW; subsequent Nova EXE builds reuse the prebuilt runners. PowerShell CLI equivalents remain available (`nova.ps1 build`).

**Windows status:** source and CI are prepared, but this environment does not have a Windows compiler or a Windows desktop. The executable, interactive menus, dialog boxes and syntax colors have **not** been tested on a real Windows machine. C++ text-engine tests and native Nova compiler/runtime regression tests are run on Linux.

## Security and limits

Nova apps run with the **current user's privileges**, without a sandbox. The Nova package checksum detects accidental corruption, not malicious editing; do not run untrusted `.nova`/EXE files. Output captured by Studio is capped at 1 MiB and shows a truncation notice. Maximum source file size is 4 MiB UTF-8.

Optional Explorer entry **Edit with Nova Studio** is installed only by running `register_nova_files.ps1`, which changes per-user `HKCU` keys, not the `.nova` default association. Undo with `unregister_nova_files.ps1`. For an installed copy:

```powershell
.\install_windows.ps1 -AddToPath -StartMenu
.\uninstall_windows.ps1 -RemoveFromPath -RemoveStartMenu -RemoveExplorerMenu
```


## Beta 2 Windows CI GUI acceptance

`NovaStudio.exe --gui-smoke` creates a real Win32 main window, editor, output and status controls, checks Unicode text input, dispatches a real `WM_COMMAND` for New, validates the layout and exits with status 0 on success. This noninteractive test runs in the Windows Actions job alongside `--self-test`. The independent Nova GUI click acceptance test is `tests/smoke_win32_click.ps1` (see `WINDOWS_CI.md`). These tests do not replace manual review of the full editing workflow on Windows 10/11.

### Nova Studio Next Updates 16–18

- **Trinity:** Edit menu converts recognized Nova keywords between Ukrainian, Polish and English in a new unsaved tab. Comments, quotes and member properties remain unchanged. Current implementation is lexical, not an AST.
- **Living UI:** new designer sources contain a `# NOVA-STUDIO-FORM/1` header; File → Edit saved designer form safely imports *unchanged* generated sources, and reviewed layout changes update the current unsaved document. Hand-written extensions require manual editing.
- **AI Proof:** an explicitly requested AI fix is compiled as a whole candidate source in a temporary folder. It is never executed. Applying requires a second confirmation and an unchanged document snapshot; compile pass does not imply tested/correct software.

See `NOVA_STUDIO_NEXT_16_18_UA.md`.
