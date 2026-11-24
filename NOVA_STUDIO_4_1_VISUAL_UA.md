# Nova Studio 4.1 — Visual Redesign (Windows)

Це **реальна зміна Win32-інтерфейсу в `studio/nova_studio.cpp`**, а не нова палітра для старого вікна. Базою є Nova Studio 4.0.2 Hyper40 CI Fix.

## Що змінилося на екрані

- **Прибрано старе системне меню Windows з верхньої частини вікна.** Замість нього нова фірмова шапка `NOVA / STUDIO 4.1`, кнопка `≡ Menu` (у ній збережено всі старі меню File, Edit, View, Nova, AI) та поле виклику Command Center.
- **Дві окремі панелі команд:** основна зверху (Menu, Search, Save, Build, Run) і друга для створення/аналізу застосунків (Open Folder, New App / Forge, App Designer, Project X-Ray, Quality Gate, Compile, Stop).
- **Ліва вертикальна Activity Rail** з доступом до Explorer, пошуку, AI Assistant, режиму Focus та консолі. Додано підказки до кнопок.
- **Перероблена геометрія workspace:** окремі поверхні Explorer, Source Editor і AI Inspector, нові відступи, межі панелей, нижній статус-бар.
- **Нова спокійна темна палітра Quiet Slate** з приглушеним шавлієвим акцентом; більше немає кіберпанк-неону.
- **Консоль тепер можна сховати** через `Ctrl+J`, ліву панель `>_` або `Menu → View → Bottom console`. Редактор отримує весь звільнений простір.
- Автоматичне згортання бокових панелей на малих ширинах; відновити їх можна кнопками Activity Rail і через Menu.

## Що збережено

Усі наявні команди, Nova Forge, Project X-Ray, Quality Gate, Time Machine, App Designer, редактор, AI Proof, компілятор та runner залишаються у кодовій базі. Ми змінили **оболонку редактора**, але не виконання Nova-програм.

## Що перевірено

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ctest --test-dir build --output-on-failure`: 10/10 портативних C++ тестів пройшли у Linux.
- `python -m unittest discover -s tests -p test_studio41_shell.py -v`: 4/4 перевірки інтеграції нового shell у вихідний код пройшли.
- У Windows GUI smoke (`NovaStudio.exe --gui-smoke`) додано перевірку відсутності старого системного меню, створення нових контролів і згортання/відновлення консолі.

**Не підтверджено:** Windows MSVC збірку та візуальну поведінку нових панелей у живому Windows desktop. Linux-збірка не компілює Win32 `nova_studio.cpp`; результат Windows CI потрібно перевірити після завантаження патча. В архіві немає готового `.exe`.

## Встановлення / оновлення

1. Для цілого репозиторію використай `Nova_Studio_4_1_Visual_Redesign_Full.zip`.
2. Якщо вже встановлена Nova Studio 4.0.2, розпакуй `Nova_Studio_4_1_Visual_Redesign_Patch.zip` поверх неї зі збереженням шляхів каталогів.
3. Commit файлів у GitHub, далі **Actions → Nova Windows x64 Acceptance → Run workflow**.
4. Windows CI спочатку компілює `NovaStudio.exe`, потім запускає CTest та GUI smoke. Лише після зеленого результату завантажуй Artifacts для запуску.

### Нюанси

- Меню `≡ Menu` використовує системне **випадне** меню Windows, але постійна світла системна смуга меню відсутня.
- Редактор та компоненти Designer/AI Proof ще використовують деякі стандартні контроли Win32. Це редизайн оболонки Studio, **не повне переписування всіх діалогів**.
- Функціональність AI не змінилася, жодні файли не надсилаються без явного дозволу.
