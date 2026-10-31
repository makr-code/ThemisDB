# GPU Module Build and Test Evidence

## Historical report (2026-08-01)

This file previously recorded a Windows `windows-release` build report for
`module_gpu_test_gpu_admin_api_focused.exe` and a reported 14/14 test run dated
2026-07-18 / 2026-08-01. It also claimed 47 GPU test files/targets were
available and passing. Those are historical report statements only; this
document does not retain the original logs, independently verify those results,
or assert that they describe the current tree.

## Current registration facts

`tests/gpu/CMakeLists.txt` globs `test_*.cpp` when `THEMIS_ENABLE_GPU` is on.
The current directory contains 63 such source files. CMake explicitly
quarantines six suites that target a superseded API:

- `test_gpu_error_handling`
- `test_gpu_error_handling_comprehensive`
- `test_gpu_memory_raii_batch1`
- `test_gpu_phase_c_integration`
- `test_gpu_phase2_phase3_focused`
- `test_gpu_resource_exhaustion`

The remaining sources are candidates for focused targets, subject to the CMake
target-exists check and the selected build configuration. Their presence or
registration is not evidence that they compile or pass. For example,
`test_gpu_fallback_all_paths` is registered with `release_critical` labels, but
the quarantined resource-exhaustion suite is not registered.

## Validation status

No current GPU build or test run was performed for this documentation update.
To establish present evidence, configure with GPU/tests enabled, build the
resulting focused targets, list the registered tests with `ctest -N`, and run
the intended subset with `ctest --output-on-failure`. Record the configuration,
backend/hardware availability, command, date, and result. A CPU-only or stub
test must not be reported as a physical-GPU run.

Canonical sources: [`tests/gpu/CMakeLists.txt`](../../tests/gpu/CMakeLists.txt),
GPU implementation/public declarations under `src/gpu/` and
`include/themis/gpu/`.
