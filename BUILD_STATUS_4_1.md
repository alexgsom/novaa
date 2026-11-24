# Nova Studio 4.1 — verified build status

- Basis: Nova Studio 4.0.2 Hyper40 CI.
- Core Nova source and functionality preserved. Windows shell changed in `studio/nova_studio.cpp`.
- Linux native CTest (Release): **10 / 10 PASS**.
- UI source integration checks: **4 / 4 PASS**.
- Windows MSVC compile of the **4.1** changes: **NOT RUN** in this environment.
- Windows visual/interactive test: **NOT RUN** in this environment.
- Windows `--gui-smoke` has additional self-checks; must be run by GitHub Actions on a Windows host.
- No built Windows EXE is included in the source archives.
