# Nova Studio Next Preview 4 — Combined Release 16/17/18

Three consecutive Preview updates in one repository revision:

- **Update 16: Nova Trinity** — English/Ukrainian/Polish keyword projection in a new unsaved tab, source preservation and actual trilingual compiled-bytecode parity example.
- **Update 17: Nova Living UI** — visual designer canonical form metadata, reimport and reviewed re-export to the original unsaved document; rejects modified generated sources rather than overwriting user code.
- **Update 18: Nova AI Proof** — after explicit AI fix, review before/after and compile the entire candidate in an independent temporary source file on a worker thread. Reject failure/stale snapshot and obtain a second confirmation for apply. No execution of generated code or automatic writes to disk.

See `NOVA_STUDIO_NEXT_16_18_UA.md` for workflow and limits.

## Verification

- Linux GCC 14.2/C++17 CMake build: **success**.
- Portable CTest: **7/7 passed**.
- Studio Python regression tests: **37/37 passed**.
- Trinity: original three sources **compile to identical bytecode** with existing self-hosted compiler; runtime output matched.
- Windows MSVC compilation, GUI interaction, and real AI provider requests: **not tested in the current environment**. CI was updated to run new native tests; please check new Windows Actions run.
- Full Python tests for all subsystems: not claimed as passed in this release.

## Delivery

`Nova_Studio_Next_Updates_16_17_18_Patch.zip`: only changed and new repository files. Overlay inside the `novaa` repository clone and commit/push.

`Nova_Studio_Next_Updates_16_17_18_Full.zip`: all repository source files at archive root; no extra wrapper folder, no prebuilt Windows EXEs.
