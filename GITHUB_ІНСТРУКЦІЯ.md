# Nova — готовий архів для GitHub

Це **повний вихідний код репозиторію Nova 1.0 Beta 2** із виправленнями Windows CI 01–08, а не вже зібраний Windows `.exe`.

## Як розпакувати й розташувати

Після розпакування ви повинні бачити в одній папці:

- `.github/workflows/windows-acceptance.yml`
- `src/nova_native.cpp` і `src/nova_win32_ui.hpp`
- `studio/nova_studio.cpp`
- `compiler/novac.nova` і `compiler/novac_selfhost.nbc`
- `tests/smoke_selfhost07.ps1` і решту `tests/`
- `tools/verify_pe.py`
- `build_windows.ps1` і `CMakeLists.txt`

**НЕ завантажуйте сам ZIP як файл у репозиторій:** GitHub не розпаковує ZIP автоматично. Також НЕ додавайте зайву зовнішню папку `Nova_GitHub_Complete_Ready` у репозиторій. У корені `novaa` мають бути ці папки та файли.

### Рекомендовано: GitHub Desktop

1. Встановіть GitHub Desktop і увійдіть у той самий GitHub-акаунт.
2. `File → Clone repository` → виберіть `alexgsom/novaa` → `Clone`.
3. `Repository → Show in Explorer`; відкриється локальна папка вашого репозиторію.
4. Скопіюйте **вміст** цього розпакованого архіву (не зовнішню папку) у неї. На запит Windows дозволяйте заміну наявних файлів. Папки `src`, `tests`, `.github` повинні знаходитися поруч із `README.md`.
5. У GitHub Desktop впишіть `Complete Nova sources and Windows CI fixes` у Summary, натисніть `Commit to main`, потім `Push origin`.
6. Відкрийте GitHub → Actions → новий запуск `Nova Windows x64 Acceptance`. Якщо він впаде, візьміть перший червоний крок і його журнал.

Не натискайте `Re-run jobs` у старому запуску: він посилається на старий коміт. Для успіху CI потрібен **новий** запуск із новим комітом.

## Статус

Виправлені відомі помилки `std::max` / Unicode курсора / Gdi32 linking. Додано діагностику графічного smoke-тесту й перевірку `tests/smoke_selfhost07.ps1` при checkout. Це **не доказ**, що весь Windows CI вже проходить: це підтвердить лише GitHub Actions після `Push origin`.
