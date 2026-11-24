## Recommended: build on GitHub-hosted Windows without installing Visual Studio locally

Follow [WINDOWS_CI.md](WINDOWS_CI.md) to run the **Nova Windows x64 Acceptance** workflow. After a green job, download `nova-windows-x64` from **Artifacts**. The artifact contains a ZIP with `NovaStudio.exe`, `nova-native.exe` and `nova-windowed.exe` (and SHA256 checksum). **These files are produced on GitHub's Windows runner, not in this source archive.**

---

# Nova Native 1.0 Alpha — швидкий старт Windows 10/11 x64

Потрібні **один раз** MSVC C++ Build Tools 2022 (Developer PowerShell) або MinGW-w64. Готових перевірених Windows `.exe` у ZIP поки немає.

```powershell
.\build_windows.ps1 -Compiler MSVC
.\bin\NovaStudio.exe
```

Nova Studio має відкривати `.nova`, компілювати (F6), запускати (F5), збирати EXE (F7) та показувати повідомлення про помилки. Для роботи потрібні `bin/nova-native.exe` і `compiler/novac_selfhost.nbc`, обидва знаходяться в розпакованому архіві/зібраній Windows-папці після компіляції. Щоб створити графічний EXE, використовується `bin/nova-windowed.exe`.

```powershell
.\install_windows.ps1 -AddToPath -StartMenu
.\register_nova_files.ps1    # за бажанням: контекстне меню Провідника
```

Для видалення встановленого середовища: `.\uninstall_windows.ps1 -RemoveFromPath -RemoveStartMenu -RemoveExplorerMenu`. Для видалення лише пункту меню: `./unregister_nova_files.ps1`. Скрипт реєстрації **не** змінює програму за замовчуванням для `.nova`.

Якщо C++-компілятора немає, можна використовувати Windows-пакет зі зібраними бінарниками, **лише якщо відповідний GitHub Actions workflow успішно завершився**. Повністю протестованого Windows-пакета в цьому ZIP немає.

---

# Windows 10/11 x64 — Nova Native 0.9

Nova Native 0.9 реалізує збирання `.exe` **без Visual Studio на комп’ютері кінцевого розробника**, але лише **після отримання готового Windows runtime**. У локальному архіві готових перевірених Windows `.exe` немає — Windows збірка/запуск не підтверджені.

## Варіант A — отримати готові Windows-бінарники

Після успішного виконання GitHub Actions workflow `.github/workflows/native-build.yml` скачайте Windows artifact `nova-09-windows-GUI-portable` і розпакуйте папку з файлами:

```text
bin/nova-native.exe
bin/nova-windowed.exe
compiler/novac_selfhost.nbc
nova.ps1
```

Готовий артефакт CI не створюється в цьому середовищі автоматично: для цього необхідно запустити workflow у власному репозиторії. Він **не перевірений до першого запуску**.

## Варіант B — один раз зібрати Windows runtime

Відкрийте **Developer PowerShell for Visual Studio** (Build Tools 2022, MSVC x64) у папці проєкту:

```powershell
.\build_windows.ps1 -Compiler MSVC
```

Альтернатива — MinGW-w64 (`.\build_windows.ps1 -Compiler MinGW` або `build_windows.bat`). Для подальшого збирання кожного `.nova` проєкту C++ інструменти вже не потрібні.

## Запуск та діагностика

```powershell
.\nova.ps1 doctor
.\nova.ps1 build -Source examples\bootstrap\trilingual.nova -Output bin\NovaHello.exe
.\bin\NovaHello.exe
.\nova.ps1 inspect -Source bin\NovaHello.exe
.\nova.ps1 unpack -Source bin\NovaHello.exe -Output bin\NovaHello.nbc
```

Також підтримуються короткі команди:

```cmd
nova.cmd doctor
nova.cmd build examples\bootstrap\trilingual.nova bin\NovaHello.exe
```

`-Force` дозволяє свідомо замінити існуючий вихідний файл, наприклад:

```powershell
.\nova.ps1 build -Source examples\bootstrap\trilingual.nova -Output bin\NovaHello.exe -Force
```

**Графічний режим 0.9:** self-hosted компілятор підтримує `import ui`, кнопки з обробниками й роботу з контролами Win32 через байткодну VM. Потрібний попередньо зібраний `bin/nova-windowed.exe`; нове `.exe` створюється без повторної C++-компіляції:

```powershell
.\nova.ps1 build -Source examples\windows09\hello_gui.nova -Output bin\NovaHelloGUI.exe -Windowed
.\bin\NovaHelloGUI.exe
```

Можна також зібрати `notes_gui.nova` й `counter_gui.nova`. `-Windowed` лише змінює підсистему виконуваного файла, **не додає Win32-вікна автоматично**: програма має викликати `ui.window` і `ui.run`. Потрібні Windows 10/11 та desktop session. У цьому середовищі не було доступу до Windows-компілятора; UI-запуск на Windows потребує ручної перевірки. Проєкт не є sandbox і не підписує EXE.

## Встановлення

```powershell
.\install_windows.ps1 -AddToPath -StartMenu
```

Без адміністратора Nova копіюється в `%LOCALAPPDATA%\Programs\NovaNative`. Після нового запуску термінала `nova build ...` працює через `nova.cmd`, якщо додано PATH.

**PowerShell:** якщо Windows заблокувала довірений вами скрипт, можна переглянути його вміст і явно розблокувати лише цей файл командою `Unblock-File .\nova.ps1` — не відключайте захисні налаштування системи без потреби.

**Безпека:** завантажені EXE непідписані, можуть потребувати підтвердження SmartScreen; перевіряйте джерело. `sqlite3.dll` додається лише з довіреного офіційного джерела. FNV checksum пакета не є криптографічним підписом.
