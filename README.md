# Nova Windows CI Tools Fix 04

Error fixed: `python: can't open file ...\\tools\\verify_pe.py: [Errno 2] No such file or directory`.

The current workflow expects these three files **at the repository root**:

```
tools/verify_pe.py
tools/create_windows_portable.ps1
tools/test_windows_portable.ps1
```

## GitHub web upload

1. Unzip this archive.
2. Go to https://github.com/alexgsom/novaa and open **Code > Add file > Upload files**.
3. Drag the unzipped `tools` folder (NOT the zip, NOT the enclosing `Nova_Windows_CI_Tools_Fix_04` folder).
4. Check that paths are shown as `tools/verify_pe.py`, etc. Commit to the default branch.
5. Verify https://github.com/alexgsom/novaa/blob/main/tools/verify_pe.py opens. A new push should start Windows CI.

If you cannot drag a folder, use **Add file > Create new file** with the exact path `tools/verify_pe.py`, paste its contents from this archive, and commit. Repeat for the `.ps1` files to prevent failures in the later packaging steps.

## Note

`verify_pe.py` uses Python ONLY as a **CI inspection tool**. Nova itself does not require Python to run.

The fix is verified locally for script presence and synthetic PE fixtures. Only GitHub Actions can confirm this specific MSVC/Windows pipeline succeeds.
