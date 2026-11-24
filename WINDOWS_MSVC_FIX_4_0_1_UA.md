# Nova Studio 4.0.1 — MSVC Windows build fix (24.11.2026)

## Що показав GitHub Actions

Windows CI успішно зібрав `bin/nova-native.exe` та `bin/nova-windowed.exe`, але зупинився на `bin/NovaStudio.exe` під час компіляції `studio/nova_studio.cpp` (MSVC 14.44):

- `studio/studio_designer15_win.hpp:21` — `C2059 syntax error: constant`;
- `studio/nova_studio.cpp:1585–1586` — `C2672 std::max/std::min` із несумісними `LONG` та `int`.

`C4456`, `C4100`, `C4312` у цьому логу є попередженнями, а не причиною завершення із кодом 1.

## Виправлено

1. У Win32 App Designer замінено три загальні імена ID команд (`CENTER_X`, `CENTER_Y`, `DUPLICATE`) на унікальні `ID_DESIGNER_CENTER_X`, `ID_DESIGNER_CENTER_Y`, `ID_DESIGNER_DUPLICATE`. Змінено відповідні кнопки та обробники `WM_COMMAND`. Це усуває конфлікт із Windows-макросами.
2. У `Studio::openCommandCenter()` координати `RECT` (Win32 `LONG`) тепер приводяться до `int` **до** `std::min` та `std::max`.

Інші функції Nova Studio 4.0 не змінювалися.

## Застосування

**Варіант A — повний ZIP:** розпакувати `Nova_Studio_4_0_1_MSVC_Fix_Full_Source.zip` у свій репозиторій замість попередньої вихідної версії Nova Studio 4.0 (не копіювати ZIP як один файл). Закомітити та запушити зміни.

**Варіант B — маленький патч:** розпакувати `Nova_Studio_4_0_1_MSVC_Fix_Patch.zip` у корінь існуючого репозиторію Nova Studio 4.0 із перезаписом шляхів `studio/…`, закомітити та запушити.

На GitHub відкрийте **Actions → Nova Windows x64 Acceptance → Run workflow** або дочекайтеся workflow після push. Тест має пройти крок **Build real Windows x64 console runtime, GUI runner and Nova Studio**. Лише після повного зеленого workflow завантажуйте готовий Windows ZIP з **Artifacts → nova-windows-x64**.

## Підтверджені перевірки

- Linux: усі 10 із 10 C++ тестів CTest успішні після змін.
- Пакувальні сценарії Python `test_apps08.py`: 2/2 після попереднього збирання `bin/nova-native`.
- Windows MSVC **не запускали в цьому середовищі**; остаточну перевірку зробить GitHub Actions.

Архіви містять **вихідний код**, а не готовий `.exe`.
