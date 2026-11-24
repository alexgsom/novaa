# Nova Native 1.0 Beta 1 — release notes

## Implemented this cycle

1. **Native syntax highlighting** using the built-in Windows RichEdit 4.1 component (Windows DLL loaded dynamically), driven by a portable C++ tokenizer that recognizes trilingual keywords, strings, numbers and comments. A plain Edit control remains the fallback.
2. **Find/Replace dialogs** (`Ctrl+F`, `Ctrl+H`, `F3`, `Shift+F3`) with case matching, whole words, previous/next and wrap-around. Replacements are bounded; Replace All currently clears undo history.
3. **Live caret location:** line and column in the editor status panel.
4. **Atomic staged EXE rebuild:** the previous EXE stays intact when the new compilation/packing fails. Rebuilding an existing EXE is now supported after overwrite confirmation.
5. **Windows text and output reliability:** explicitly reject embedded NUL in UTF-8 source, truncate excessive subprocess output with a warning, bound syntax-color processing.
6. **Cross-platform C++ tests:** new `studio/studio_text.hpp`, `tests/studio_text_native.cpp`, Python CI integration and `ctest` target. Windows Studio `--self-test` exercises Ukrainian/Polish text matching too.
7. **Windows CI:** a second independent CMake/MSVC build of Studio + portable text tests before the rest of the Win32 smoke tests.

## Retained features

C++ Nova runtime, self-hosted Nova compiler (bytecode), Win32 `import ui` in the VM, `storage`/`json`/`sqlite`/`http` as previously available, and compilerless packaging of bytecode alongside a prebuilt Windows native runner.

## Honest limitations

- **Not validated on a real Windows desktop yet.** Native text processing and the Nova interpreter/VM/packaging are tested on Linux. The GUI is prepared in C++ but Windows MSVC/MinGW compilation and user interaction remain pending.
- The editor is intentionally limited to one document, no debugger/project explorer; console input is redirected to NUL. No Python is required for end-user runtime or editor, but Python runs the automated tests.
- Syntax colors are best-effort: large documents are not recolored for responsiveness; RichEdit fallback is plain text. Replace All resets undo history.
- Self-hosting means the **Nova compiler** compiles its own bytecode. The virtual machine and Windows system bindings are still implemented in C++.
