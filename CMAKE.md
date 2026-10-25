---
Author: "ThemisDB Team"
Created: "2026-10-25"
Last Updated: "2026-10-25"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# CMake and CTest Policy

This repository intentionally separates build configuration from test configuration.

## Canonical rule

`ctest` must be run only against a build tree configured with `THEMIS_BUILD_TESTS=ON`.

The release preset is not a test preset and is expected to produce `No tests were found` when used as a test target.

## Why this is intentional

The root CMake defaults and presets are explicit:

- [CMakePresets.json](CMakePresets.json) defines `windows-release` with `THEMIS_BUILD_TESTS: "OFF"`.
- [CMakePresets.json](CMakePresets.json) defines `windows-debug` with `THEMIS_BUILD_TESTS: "ON"`.
- [CMakeLists.txt](CMakeLists.txt) defaults Debug builds to test-enabled and Release builds to test-disabled.
- [tests/CMakeLists.txt](tests/CMakeLists.txt) exits early when `THEMIS_BUILD_TESTS` is false.

This is the repository’s standard pattern and is required for correct local and CI behavior.

## Required usage

### Local development / VS Code

Use the debug/test-enabled build tree:

```bash
cmake --preset windows-debug
cmake --build build-msvc-windows-debug --parallel
ctest --test-dir build-msvc-windows-debug --output-on-failure
```

### Release builds

Release builds are for packaging and runtime validation, not test execution:

```bash
cmake --preset windows-release
cmake --build build-msvc-windows-release --parallel
```

Do not run `ctest` against this directory unless it was explicitly reconfigured with tests enabled.

### GitHub Actions / CI

CI must configure a test-enabled build before invoking CTest. The existing pattern in [.github/workflows/gate-pr-core.yml](.github/workflows/gate-pr-core.yml) is the reference implementation:

```yaml
- name: Configure test build
  run: cmake --preset windows-debug -DTHEMIS_BUILD_TESTS=ON

- name: Build tests
  run: cmake --build build-msvc-windows-debug --parallel

- name: Run tests
  run: ctest --test-dir build-msvc-windows-debug --output-on-failure -j 1
```

## Expected behavior

Running CTest against the release profile is not a failure of the repository; it is expected because that preset intentionally disables tests.

Example:

```bash
ctest --preset windows-release --output-on-failure -V
```

This can legitimately return:

```text
No tests were found!!!
```

That result is correct for a non-test build tree.

## Rule for contributors

When a test command fails with `No tests were found`, the first check is:

1. Are we running against a release-only build directory?
2. Is `THEMIS_BUILD_TESTS` enabled in the current configure step?
3. Are we using the debug or dedicated CI test build tree?

If answers 1 or 2 are negative, the fix is not in the tests themselves — it is in the build/test selection.

## Related references

- [CMakePresets.json](CMakePresets.json)
- [CMakeLists.txt](CMakeLists.txt)
- [tests/CMakeLists.txt](tests/CMakeLists.txt)
- [CTEST.md](CTEST.md)
- [.github/workflows/gate-pr-core.yml](.github/workflows/gate-pr-core.yml)
