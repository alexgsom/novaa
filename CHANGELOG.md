## 4.2 Aura — 2026-11-24

- Custom dark `WC_TABCONTROLW` renderer, double-buffered; native tab-switch behavior remains.
- Fixed stray `Open` button at (0,0); all Open functionality is available via menu/shortcut.
- Uniform owner-drawn GDI buttons with hover/focus/pressed handling.
- Consistent rail vector icons, spacing and context-aware AI action grid.
- Optional dark theme on Windows native scroll controls.
- Updated Win32 GUI smoke and cross-platform source regression tests.
- No changes to Nova source language, compiler, bytecode VM or App Designer model.
- Actual Windows compilation/visual verification remains pending.

# Nova Studio 4.1 — Visual Redesign

- Removed permanent native menubar; preserved menus inside a new `≡ Menu` dropdown.
- Added two-tier command deck, left activity rail with tooltips, new visual zones and Quiet Slate palette.
- Added collapsible terminal and diagnostics (`Ctrl+J`).
- Expanded real Windows GUI smoke checks for layout and console visibility.
- 10/10 portable C++ tests and 4/4 shell source checks passed on Linux; Windows GUI pending validation.

---

## Nova Studio 4.0.1 — Windows MSVC compilation hotfix (2026-11-24)

- Avoid Windows SDK macro collisions in App Designer control IDs (`ID_DESIGNER_*`).
- Normalize Win32 `RECT`/`LONG` dimensions to `int` before command palette layout `std::min`/`std::max`.
- Windows compilation awaits GitHub Actions re-validation; Linux CTest 10/10 passed.

## Nova Studio 4.0 Hyper Preview — 2026-11-24

- NEW: fuzzy Command Center with workspace files, current-file symbols and actions.
- NEW: Nova Forge — Counter, Notes and Tasks blueprints, no AI/no overwrite.
- NEW: Project X-Ray — offline static findings, clickable source locations.
- NEW: Quality Gate — cancellable, compile-only project batch verification.
- NEW: Time Machine — local, manually created per-file checkpoints and safe restore into editor buffer.
- FIX: remove out-of-scope traceSteps check from standalone embedded-app branch.
- TESTS: portable C++ suite extended to 10 and added to both Linux/Windows CI workflows.
- STATUS: Linux tests passing; Windows UI/MSVC validation pending.

## Nova Studio 3.0 Preview — Update 19

- Kept original restrained dark UI, no cyberpunk redesign.
- New Win32 line gutter and clickable code minimap; current-line highlighting.
- Ctrl+Space now includes local function names; Ctrl+Shift+O jumps to functions; Ctrl+Shift+F searches .nova project files.
- User-consented, bounded project AI context; strict multi-file response format; expanded diff review; compile-only verification in a temporary folder; second approval + disk snapshot checks and best-effort rollback before writing existing files. No auto-run of AI code.
- Designer: 10px grid with snapping, resizing by grip, numeric properties, templates, separate Win32 control preview. Existing Living UI source round-trip preserved.
- New portable C++ tests and Python integration-contract tests; Windows CI test targets/smoke updated. Windows release EXE not verified in current Linux runtime.

## Nova Studio Next Preview 3 / Update 15

- Win32 form designer (`studio_designer15_win.hpp`) з портативною моделлю компонування (`studio_designer15.hpp`), генерацією однієї .nova-програми в нову вкладку лише після підтвердження.
- Interactive Run: обмежений stdin pipe, поточний вивід stdout/stderr, поле Send input (Ctrl+Enter).
- Self-hosted bytecode VM: `ask`, `запитай`, `zapytaj` тепер читають стандартний ввід.
- AI Create app: явно введений опис → чернетка одного файлу Nova, відкривається тільки після підтвердження; без автоматичного запису чи запуску.
- Нові нативні CMake та Python-тести, Windows GUI smoke тест для створення вікна дизайнера, два приклади й інструкції.
- Windows GUI і реальний OpenAI API ще не підтверджені.

## Nova Studio Next Preview 2 — Update 14

- Multi-document tabs with unsaved buffer tracking and Ctrl+Tab/Ctrl+W.
- Nova keyword completion in 3 languages and RichEdit auto-indentation.
- Clickable compiler/runtime diagnostics with line navigation.
- AI proposal preview and manual approval, with original-text verification.
- Portable C++ tests, GUI smoke checks, and CI/packaging integration.

# Update 13 — Nova Studio Next Preview 1

See [STUDIO_NEXT_13_RELEASE.md](STUDIO_NEXT_13_RELEASE.md) for real code changes and test limits.

---

# Nova Native 1.0 Beta 2 — Windows CI Acceptance

- Dedicated GitHub Windows Server 2022 x64 MSVC acceptance workflow, separate from Linux CI.
- Windows portable ZIP built from *real* Windows EXEs, uploaded after all acceptance tests pass.
- New `NovaStudio.exe --gui-smoke` tests real Win32 controls, Unicode editing and menu event.
- New Nova GUI program plus PowerShell Win32 `BM_CLICK` automation validates callback execution end-to-end.
- AMD64 PE32+ format/subsystem verification; SHA256 manifest and ZIP checksum.
- Relocated portable distribution test checks `nova doctor`, Studio, code compilation and GUI callbacks under Unicode paths.
- Five new portable regression tests; 222 total regressions passed on Linux.
- No unverified Windows EXE is bundled or advertised as working.

---

# Nova Native 1.0 Beta 1

- Native syntax highlighting, trilingual keywords, strings, numbers and comments in Nova Studio.
- Native Win32 Find / Replace / Find Next / Find Previous and live line/column status.
- Atomic staged Windows EXE rebuild with rollback on compilation/packing failure.
- Improved UTF-8 validation, capped captured subprocess output with visible truncation status.
- Added real cross-platform C++ tests for Studio text algorithms and Windows CI CMake build.

## 1.0 Alpha — Nova Studio / Windows-first

- Нативний Win32/C++17 редактор `.nova`: відкриття, UTF-8, збереження, drag-and-drop, запуск, компіляція, EXE-збирання, панель помилок, stop.
- Фоновий компілятор Nova та VM без Python, інтегровані в редактор.
- Windows MSVC/MinGW і CMake збірка NovaStudio.exe.
- Необов'язкова інтеграція в Explorer; встановлення без прав адміністратора.
- Виправлено некоректні рядки у Windows CI; додано самоперевірку Nova Studio та нові тести.

# Nova Native 0.9 — Windows GUI Bytecode

- Підтримка `import ui` у self-hosted `novac.nova` і нативній байткодній VM.
- Прив’язка подій Win32 `ui.button`/`ui.menu` до функцій байткоду без Python.
- Передавання функції як значення (`greet`) або за назвою (`"greet"`).
- Спрощені методи `field.get()`/`field.set()` та `ui.get`/`ui.set`.
- Спільний бюджет кроків VM для основної програми і callback.
- Команда `nova.ps1 build ... -Windowed` тепер призначена для реальних GUI-програм з `import ui`.
- Чотири приклади Win32 і нові тести компіляції/пакування.
- Reproducible self-host: stage-1 == stage-2, побайтово.
- Windows workflow: компіляція трьох GUI `.exe`, Win32 noninteractive smoke.
- Win32 інтерактивні події ще потребують перевірки на Windows; локально перевірено Linux компіляцію і пайплайн.

---

# Nova Native 0.8 — Portable Windows / self-hosted compiler

- Створено `src/nova_package.hpp`: C++17 пакування готового runtime з байткодом Nova в один виконуваний файл **без повторної компіляції C++**.
- CLI `nova-native pack`, `inspect`, `unpack` з перевіркою довжини/контрольної суми та обмеженням 4 MiB.
- Відновлення вбудованого `.nbc` побайтово; захист від заміни вхідного/вихідного файла і вже запакованого runtime.
- Перевірка PE32+ x64/підсистеми Windows, а також відхилення підроблених PE заголовків і структурно неправильних інструкцій байткоду.
- `nova.ps1 build|compile|run|inspect|unpack|doctor` для Windows; `nova.cmd` маршрутизує команди; `nova.sh` для Linux.
- `doctor` без Python створює тестову програму self-hosted компілятором, виконує її через Nova VM, пакує та перевіряє результат.
- `nova.sh compile` тепер не перезаписує існуючі байткод-файли без `--force`; аналогічний захист є в Windows-скрипті.
- `build_windows.ps1`/`build_windows.bat` створюють консольний та безконсольний Windows runtime (для встановлення користувачем потрібні попередньо зібрані бінарники).
- Windows CI створює Windows portable distribution і тестує команди PowerShell; результат CI **не перевірено локально**.
- Новий приклад `examples/portable/nova_tasks_cli.nova`: робота із `system.args`, файлами та JSON у самодостатній Nova-програмі.
- Додаткові автоматичні тести CLI, захисту пакета, Unicode, збереження даних і самокомпіляції.

**Обмеження:** готового перевіреного Windows EXE в локальній збірці немає; `-Windowed` приховує консоль, але Nova bytecode compiler **ще не компілює `import ui`**; `.exe` створюється на основі готового PE x64 runtime, а не з машинного коду, згенерованого Nova.

**Перевірки:** 192/192 на Linux; CMake/GCC/Clang зібрали C++ рушій. Три Linux x64 демонстраційні виконувані файли містяться в `dist/`.

## Nova Studio Next Preview 4 — Updates 16 + 17 + 18

- 16 Trinity: trilingual keyword view (new unsaved tab) with literal/comment protection; three sample programs compile into identical bytecode.
- 17 Living UI: exact metadata-backed designer roundtrip for generated forms; rejects hand-edited code, applies on approval.
- 18 AI Proof: non-executing isolated-file compiler check for AI proposals, snapshot validation, explicit approval, fail-closed semantics.
- Three new portable native test suites plus end-to-end Trinity Python regression; CI targets and portable Windows package updated.
