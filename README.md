# Nova Windows GUI Diagnostics — Fix 06

**Current confirmed failure:** `NovaStudio.exe --gui-smoke` crashes with Windows status `0xC000041D` (fatal exception in a user callback). Earlier RichEdit cleanup change did not cure it. The underlying exception is **not yet identified**.

## What this patch changes

- `studio/nova_studio.cpp`: enables per-event tracing **only** for `--gui-smoke`; writes immediately flushed `NOVA_STUDIO_TRACE_FILE` log; marks window creation, actual control tests, teardown; records fatal first-chance exception code and address through a vectored exception observer without intercepting it. Also initializes `mainWindow=h` at `WM_CREATE` so callbacks have the correct window handle while the window is being created.
- `.github/workflows/windows-acceptance.yml`: prints the trace when the test fails and uploads the log into `nova-windows-diagnostics` as a failure artifact.

## Apply

1. In your GitHub `alexgsom/novaa` repository, **replace** `studio/nova_studio.cpp` with the supplied file (via Edit → paste all → Commit changes).
2. **Replace** `.github/workflows/windows-acceptance.yml` with the file included here, at the exact same GitHub path.
3. Check the new run in Actions. If the `--gui-smoke` test fails, expand its step and copy the output after `--- Nova Studio GUI trace ---` into ChatGPT.

The supplied Windows workflow was based on the Nova Beta 2 workflow from our build, so if you have made unrelated changes to yours, preserve them and copy only the diagnostic step plus the `runner.temp` log artifact entry.

## Important

No Windows environment exists in this workspace, so real `NovaStudio.exe --gui-smoke` behavior cannot be verified here. The patch intentionally **does not suppress the crash or declare success**. The trace locates where the crash occurs and records the first-chance exception address, enabling a targeted fix after the next Windows run.
