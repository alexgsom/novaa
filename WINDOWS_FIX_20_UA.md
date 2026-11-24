# Nova Studio — Windows MSVC Fix 20

Це термінове виправлення збірки після Update 19.

## Причини

1. `studio/studio_designer15_win.hpp`: у повідомленні `MessageBoxW` був фізичний розрив рядка всередині C++ wide-string. MSVC видавав `C2001: newline in constant` та каскадні помилки. Замінено на `\n` у двох суміжних літералах.
2. `studio/nova_studio.cpp`: `std::max(1, rect.bottom)` і `std::max(1, rect.right)` змішували `int` та Windows `LONG`. Розміри приведено до `int`.
3. `tests/test_studio_next19.py`: додана регресійна перевірка обох проблем.

## Встановлення

- Розпакуйте `Nova_Studio_19_Windows_Fix_20_Patch.zip`.
- Скопіюйте **вміст** архіву в корінь локального репозиторію `novaa` із заміною файлів.
- У GitHub Desktop зробіть `Commit to main` та `Push origin`.
- В GitHub Actions дочекайтеся результату Windows-збірки.

Цей ZIP містить вихідний код, а не готовий `.exe`.

## Перевірка

Локально запускаються Python/CMake тести. MSVC/Windows GUI запускаються GitHub Actions; повний результат збірки з'явиться там.
