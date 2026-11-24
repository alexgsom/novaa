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
