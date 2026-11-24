# Nova Studio 4.1 — Visual Redesign (Windows preview)

**NEW IN 4.1:** The application shell is replaced in the real Win32 Studio source: custom two-tier command deck, permanent activity rail, Explorer/Source/AI panels, Quiet Slate dark surfaces and collapsible console (`Ctrl+J`). The old permanent native Windows menu bar is removed; full menus are available in `≡ Menu`.

See [NOVA_STUDIO_4_1_VISUAL_UA.md](NOVA_STUDIO_4_1_VISUAL_UA.md) for detailed Ukrainian notes, verification status and installation instructions. `design/CONCEPT_PREVIEW.html` is an **illustrative mockup, not an actual Windows screenshot**.

**Verification boundary:** 10/10 portable C++ tests and 4/4 shell-integration checks pass on Linux. **Windows 4.1 executable and visual behavior have not been tested here.** Build and GUI smoke test through GitHub Actions before release.

---

**NOVA STUDIO 4.0 HYPER PREVIEW:** new Command Center (`Ctrl+Shift+P`), Nova Forge (three local application blueprints), offline Project X-Ray, compile-only project Quality Gate, and per-file Time Machine checkpoints with safe restore. Includes a fix for standalone compilation after Update 24. **Windows build/UI not yet verified.** Read [Ukrainian release guide](NOVA_STUDIO_4_HYPER_UA.md).

**Nova Studio 3.0 Preview — Update 19:** preserved restrained dark theme (no cyberpunk redesign), with minimap, line numbers, current-line highlight, smarter completion, function navigation (Ctrl+Shift+O), project search (Ctrl+Shift+F), bounded project-aware AI multi-file proposal + compile-only verification, and App Designer snapping/grid/properties/templates/native preview. Read [Ukrainian guide](NOVA_STUDIO_3_UPDATE_19_UA.md). Windows MSVC/GUI verification still required.

**Nova Studio Next Preview 4 — Combined Updates 16/17/18**: Trinity trilingual source views, Living UI form round-trip, and AI Proof compile-first review. Read [the Ukrainian guide](NOVA_STUDIO_NEXT_16_18_UA.md).

> **Nova Studio Next Preview 3 (Update 15):** візуальний конструктор вікон, інтерактивний Run-термінал, підтримка `ask()` у байткоді та AI-генерація чернетки Nova-застосунку. Інструкція: [NOVA_STUDIO_NEXT_15_UA.md](NOVA_STUDIO_NEXT_15_UA.md).

> **Nova Studio Next Preview 2 (Update 14):** вкладки, Ctrl+Space, авто-відступи, Problems і AI Preview/Apply. Інструкція: [NOVA_STUDIO_NEXT_14_UA.md](NOVA_STUDIO_NEXT_14_UA.md).

# NOVA STUDIO NEXT — PREVIEW 1 / UPDATE 13

Нова темна IDE з навігатором файлів і необов'язковим OpenAI AI Assistant.
**Інструкції та обмеження:** [NOVA_STUDIO_NEXT_13_UA.md](NOVA_STUDIO_NEXT_13_UA.md).

---

# Nova Native 1.0 Beta 2 — Windows CI Acceptance Edition

**NEW:** This project now includes Windows x64 GitHub Actions CI that builds real `.exe` files using MSVC, exercises native Win32 event callbacks and produces a portable Windows ZIP on successful checks. It has **not yet been executed on GitHub/Windows**. See [WINDOWS_CI.md](WINDOWS_CI.md) for exact setup and run steps, and [RELEASE_BETA_2.md](RELEASE_BETA_2.md) for changes.

---

# Nova Native 1.0 Beta 1 — Windows Studio

**Нові можливості редактора:** підсвічування синтаксису `.nova` (українська, польська й англійська), пошук `Ctrl+F`, заміна `Ctrl+H`, наступний збіг `F3`, попередній `Shift+F3`, індикатор рядка/стовпця. Підсвічування використовує системний Windows RichEdit, а пошук і кольори мають спільне нативне C++-ядро з реальними тестами на Linux. Нове складання EXE в Nova Studio проходить через тимчасовий файл: попередня версія лишається цілою, якщо нова збірка не вдалася.

**Збірка Windows:** `./build_windows.ps1 -Compiler MSVC` → `./bin/NovaStudio.exe`. Додано перевірки Win32-редактора через MSVC/CMake у GitHub Actions та `NovaStudio.exe --self-test`. **Фактичне складання й запуск Nova Studio під Windows у поточному Linux-середовищі не підтверджені.** Архів має вихідний код і бінарники лише для Linux x86-64; Windows EXE необхідно зібрати або отримати з успішної CI-збірки.

Інструкції: [`studio/README.md`](studio/README.md), [`INSTALL_WINDOWS.md`](INSTALL_WINDOWS.md), [`RELEASE_1_0_BETA_1.md`](RELEASE_1_0_BETA_1.md). Попередній функціонал інтерпретатора, компілятора й Windows UI збережено.

---

# Nova Native 1.0 Alpha — Windows Studio

**Новинка:** нативний редактор **Nova Studio** на C++/Win32 без Python. Відкривайте `.nova` (у тому числі drag-and-drop), запускайте програму **F5**, компілюйте **F6**, збирайте `.exe` **F7**, переглядайте помилки в нижній панелі; команди виконуються у фоновому потоці. Доступне необов’язкове меню Провідника «Edit with Nova Studio» без зміни асоціації файлів.

Після отримання/збирання Windows-бінарників: `./build_windows.ps1 -Compiler MSVC`, потім `./bin/NovaStudio.exe`. Для детального опису див. [`studio/README.md`](studio/README.md), [релізні зміни](RELEASE_1_0_ALPHA.md) та [`INSTALL_WINDOWS.md`](INSTALL_WINDOWS.md).

**Статус:** Windows `NovaStudio.exe` у цьому Linux-середовищі не збирався і не запускався. Windows CI підготовлений, але не підтверджений; цей реліз є **Alpha**, а не готовим стабільним продуктом. Наявні бінарники `bin/nova-native` у ZIP призначені тільки для Linux x86-64. Нижче збережено документацію Nova 0.9.

---

# Nova Native 0.9 — Self-hosted Windows GUI

**Nova 0.9** — експериментальна самостійна мова програмування з власним C++17 runtime, віртуальною машиною та компілятором `compiler/novac.nova`, **написаним самою Nova**. Виконання коду, компіляція `.nova → .nbc` і пакування в готовий виконуваний файл не використовують Python.

## Нове в 0.8

- **Пакувальник C++ без повторної компіляції:** `nova-native pack code.nbc MyApp.exe --runtime nova-native.exe`. Він приєднує байткод до *вже зібраного* нативного рушія, а не викликає Visual Studio для кожного застосунку.
- **Нова команда Windows:** `nova.cmd build main.nova MyApp.exe` або `./nova.ps1 build -Source main.nova -Output MyApp.exe`. Також доступні `compile`, `run`, `inspect`, `unpack` та `doctor`.
- **Два попередньо зібрані рушії Windows, коли CI буде виконано:** `nova-native.exe` (консольний) і `nova-windowed.exe` (без консолі, підтримує байткодний Win32 UI).
- **Linux:** `sh nova.sh build input.nova output`, `sh nova.sh doctor`, `sh nova.sh compile ...`.
- **Команди `inspect/unpack`:** перевірка контрольної суми упакованого байткоду і його повне відновлення.
- **Робота з Unicode:** кирилиця та польські літери у файлах, даних і шляхах.
- **Безпечніша перевірка:** пакувальник відкидає неправильну структуру інструкцій, не приймає підроблений заголовок PE, не перезаписує файли без `--force`/`-Force`.
- **Практична програма:** `examples/portable/nova_tasks_cli.nova` — невеликий застосунок, який зберігає список справ у JSON через нативні `storage` + `system`, без Python.
- **Самокомпіляція збережена:** скомпільований компілятор Nova може знову скомпілювати `novac.nova` і отримати побайтово однаковий результат (перевірка на Linux).

## WINDOWS 10/11 x64 — підготований сценарій

**Перший запуск саме на Windows поки не перевірено.** В архіві є Linux x86-64 виконувані файли та вихідний код; **готових підтверджених Windows EXE тут немає**. Windows-рушій можна отримати зі збірки GitHub Actions (`nova-08-windows-portable-no-compiler-required`, після успішного запуску CI) або одноразово зібрати на Windows компілятором MSVC/MinGW-w64:

```powershell
.\build_windows.ps1 -Compiler MSVC
```

Після цього для створення кожного наступного застосунку **C++-компілятор більше не потрібен**:

```powershell
.\nova.ps1 doctor
.\nova.ps1 build -Source examples\bootstrap\trilingual.nova -Output bin\HelloNova.exe
.\bin\HelloNova.exe
.\nova.ps1 inspect -Source bin\HelloNova.exe
```

Або коротко через `nova.cmd` (передаючи позиційні аргументи):

```cmd
nova.cmd doctor
nova.cmd build examples\bootstrap\trilingual.nova bin\HelloNova.exe
bin\HelloNova.exe
```

Інсталяція для поточного користувача без адміністратора (після отримання Windows-бінарників):

```powershell
.\install_windows.ps1 -AddToPath
```

Детальніше — `INSTALL_WINDOWS.md`.

## Linux x86-64 — працюючий приклад

Для готового Linux runtime з архіву потрібні сумісні системні бібліотеки. Для локальної перебудови один раз потрібні C++17, libcurl-dev та pkg-config:

```bash
sh build.sh
sh nova.sh doctor
sh nova.sh build examples/bootstrap/trilingual.nova NovaHello
./NovaHello
sh nova.sh inspect NovaHello
sh nova.sh unpack NovaHello hello.nbc
```

### Застосунок зі збереженням JSON (Windows/Linux)

```bash
sh nova.sh build examples/portable/nova_tasks_cli.nova NovaTasks
./NovaTasks -- add "Купити хліб"
./NovaTasks -- add "Zrobić kawę"
./NovaTasks -- list
./NovaTasks -- clear
```

На Windows аналогічно створіть `NovaTasks.exe` через `nova.ps1 build` та виконуйте `NovaTasks.exe add "Купити хліб"`. Застосунок зберігає дані у `%APPDATA%\NovaTasks\tasks.json` (Windows) або `$XDG_CONFIG_HOME/NovaTasks/tasks.json` (Linux; за відсутності змінної — у `~/.config/NovaTasks`).

## Пакувальник та формат

Файл містить нативний рушій + JSON-байткод (`.nbc`) + 32-байтовий службовий блок (magic, довжина, FNV-1a 64). `inspect` перевіряє пошкодження байткоду. **Це НЕ криптографічний підпис і НЕ захист від навмисної зміни**. Не запускайте неперевірені EXE; Windows-артефакти перед публічним поширенням слід підписувати *після пакування*.

- Максимальна довжина байткоду — 4 MiB.
- Перезапис файлів лише з `--force` (native/Linux) або `-Force` (PowerShell).
- Створювати Windows EXE на Linux можливо **тільки** за наявності справжнього попередньо зібраного Windows PE x64 Nova runtime, переданого через `--runtime`. Linux ELF не видається за Windows EXE.
- Пакувальник не є sandbox: `storage`, `http` та інші стандартні API працюють із правами користувача, який запустив застосунок.

## Self-hosted Win32 GUI (Нове в 0.9)

Компілятор `compiler/novac.nova`, написаний Nova, уперше компілює `import ui`, функції-обробники кнопок і виклики `ui.window`, `ui.label`, `ui.input`, `ui.textarea`, `ui.checkbox`, `ui.select`, `ui.menu`, `ui.place`, `ui.alert`, `ui.confirm`, `ui.get`, `ui.set`, `ui.close`, `ui.run`, відкриття/збереження файлів. Доступні також `field.get()` і `field.set(value)` для змінних-контролів.

**Важливо:** на Windows графічні елементи виконуються Win32 API через Nova C++ VM; Linux інтерпретатор повертає зрозумілу помилку «native ui ... Windows only». Обробники кнопок виконуються **тим самим байткодним інтерпретатором** з обмеженням загальної кількості кроків; помилка обробника закриває вікно і повідомляється користувачу. Для змінного стану між подіями використовуйте об'єкт на зразок `let state = {"count": 0}`, оскільки змінні всередині функцій мають локальну область видимості.

Після **одноразового** отримання `nova-native.exe` і `nova-windowed.exe` користувач може зробити програму без встановленого Python чи компілятора C++:

```powershell
.\nova.ps1 doctor
.\nova.ps1 build -Source examples\windows09\hello_gui.nova -Output bin\NovaHelloGUI.exe -Windowed
.\bin\NovaHelloGUI.exe
```

Приклади: `examples/windows09/hello_gui.nova`, `counter_gui.nova`, `notes_gui.nova`, `ui_smoke.nova`. Вони додаються до Windows CI як перевірка компіляції; `ui_smoke.nova` створює приховане вікно, перевіряє атрибути Win32-контролів і завершується без подій користувача. Інтерактивні натискання кнопок ще треба перевірити вручну на Windows.

**Самокомпіляція збережена:** компілятор Nova, зібраний у байткод, повторно компілює власний `novac.nova` у побайтово ідентичний байткод. C++ VM потрібна для виконання й підготовки нативних Windows runner-ів; повна сумісність з усіма командами Nova 1.2 не гарантована. Не підтримуються замикання, загальна підтримка `try/catch`, `match/case` та імпорти довільних `.nova` модулів у байткодному компіляторі.

**Залежності:** Python не потрібен для виконання Nova або компіляції Nova-коду; системні Win32 DLL потрібні за стандартних умов Windows, а SQLite — окремо (`sqlite3.dll`). Пакувальник перевіряє FNV-1a, це не цифровий підпис і не захист від зловмисного редагування EXE. Nova програми виконуються з правами користувача — не запускайте неперевірені програми.

**Статус тестів:** автоматизовані C++ збірки, компіляція та Linux-регресія перевірені локально. Готових випробуваних на Windows `.exe` у цьому архіві немає; сценарії GitHub Actions підготовлені, але їхній Windows результат не підтверджений тут.

## Укладені готові Linux x86-64 приклади

Для швидкого запуску на сумісній Linux-системі в архів також включено:

- `dist/linux-x64/NovaHello` — багатомовна демонстрація;
- `dist/linux-x64/NovaTasks` — готовий список справ із JSON;
- `dist/portable/novac-native` — запакований Nova-authored компілятор.

Це файли **Linux ELF**, а не Windows EXE. Вони можуть залежати від системних бібліотек, які використовував компілятор/операційна система під час збирання.

## Nova language + Studio update 12

- Condition-controlled loops in all three keyword languages: `while`, `поки`, `dopóki`.
- `break` / `continue` supported in these loops, both directly in the C++ interpreter and in Nova self-hosted bytecode.
- Nova Studio: new Save / Run / Compile / Build EXE / Stop toolbar and **Edit → Toggle comment** (`Ctrl+K`) for selected lines, preserving indentation, CRLF, and undo history.
- Demo: `examples/while_trilingual_12.nova`; regression tests: `tests/test_while12.py` and portable native Studio tests.

```nova
let n = 1
поки n <= 3
    show n
    n = n + 1
кінець
```
