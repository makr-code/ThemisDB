# Local Quality Gate (Windows)

This guide provides a local quality workflow for ThemisDB with command-line and GUI execution.

## Scope

The local quality gate runs these checks in one sequence:

1. CMake configure
2. CMake build
3. CTest
4. clang-tidy
5. cppcheck
6. semgrep
7. CodeQL
8. Doxygen

All report files are written to `reports/local-quality` by default.

## Scripts

- PowerShell runner: `scripts/quality-gate.ps1`
- Tkinter launcher: `scripts/quality_gate_gui.py`

## Prerequisites

Install required and optional tools:

- Required: `cmake`
- Optional but recommended: `clang-tidy`, `cppcheck`, `semgrep`, `codeql`, `doxygen`

Example using `winget` and `pip`:

```powershell
winget install LLVM.LLVM
winget install Cppcheck.Cppcheck
winget install Python.Python.3.12
winget install GitHub.codeql
winget install DimitriVanHeesch.Doxygen
pip install semgrep
```

If an optional tool is missing, the script skips the corresponding step and reports a warning.

## Command-Line Usage

Run the full gate:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\quality-gate.ps1
```

Run with selected skips:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\quality-gate.ps1 -SkipCodeQL -SkipSemgrep
```

Run all steps and continue after failures:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\quality-gate.ps1 -ContinueOnError
```

Show built-in help:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\quality-gate.ps1 -Help
```

## GUI Usage (Tkinter)

Start the GUI:

```powershell
python .\scripts\quality_gate_gui.py
```

GUI features:

- Preset fields for configure/build/test
- Per-step skip toggles
- Continue-on-error toggle
- Live output view
- Open reports folder button
- Save log button

The GUI calls `scripts/quality-gate.ps1` and streams output in real time.

## Preset Defaults

Default values match current Windows workflows:

- Configure preset: `windows-release`
- Build preset: `windows-release`
- Test preset: `windows-release`
- Reports directory: `reports/local-quality`

## Output Artifacts

Typical output files include:

- `01-configure.log`
- `02-build.log`
- `03-ctest.log`
- `04-clang-tidy.log`
- `05-cppcheck.log`
- `05-cppcheck.xml`
- `06-semgrep.log`
- `06-semgrep.json`
- `07-codeql-create.log`
- `07-codeql-analyze.log`
- `07-codeql.sarif`
- `08-doxygen.log`

## Notes

- `clang-tidy` and `cppcheck --project` require `compile_commands.json` from CMake configure.
- CodeQL database creation runs a separate build command and can take longer than other steps.
- For fast local feedback, run with selective skips and execute full checks before PR creation.
