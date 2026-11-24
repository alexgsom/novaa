# Nova Windows CI — Checkout Diagnostics Fix 08

Це діагностика повторної помилки `./tests/smoke_selfhost07.ps1 is not recognized`.

## Встановлення

1. Відкрийте `.github/workflows/windows-acceptance.yml` у GitHub (гілка `main`).
2. Замініть **весь вміст** на вміст файлу `.github/workflows/windows-acceptance.yml` із цього архіву.
3. Натисніть Commit changes. Запустіть **новий** workflow на `main`.
4. Якщо з'явиться помилка, скопіюйте журнал кроку `Verify self-host test in checked-out Git revision`, а також `Native self-hosting A to B and Windows compiler EXE`.

## Що перевіряємо

- GitHub SHA і фактичний `git rev-parse HEAD`.
- Чи є файл у дереві коміту (`git ls-tree`).
- Чи є файл у `$env:GITHUB_WORKSPACE/tests` після checkout.
- Чи не зник файл у проміжних кроках; запускаємо через абсолютний шлях.

Це **діагностична зміна**, а не підтверджене виправлення причини. Вона не пропускає тест і не приховує помилки.
