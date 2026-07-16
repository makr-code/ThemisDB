# MSVC Include Discovery Fix Plan

## Goal
Stabilize MSVC standard-library include discovery for the `windows-release` preset so focused targets build without generating rootless `-I\include` arguments.

## Affected Files
- `cmake/CompilerOptions.cmake`
- `cmake/msvc_includes_fix.cmake`

## Root Cause
- Visual Studio is installed under `C:/Program Files/Microsoft Visual Studio/...` on this machine.
- Current fallback discovery prefers `ProgramFiles(x86)` and only globs there, so `_VC_TOOLS_DIR` stays empty.
- `cmake/CompilerOptions.cmake` also re-adds MSVC/SDK include directories later without guarding against an empty `_VC_TOOLS_DIR`, which injects `-I\include` into compile commands.

## Changes
1. Search both `ProgramFiles(x86)` and `ProgramFiles` for `VC/Tools/MSVC/*`.
2. Deduplicate and sort discovered toolset roots before selecting the newest path.
3. Guard the later `include_directories()` block in `cmake/CompilerOptions.cmake` so empty toolset paths cannot emit invalid include flags.
4. Add a warning when MSVC headers still cannot be resolved automatically.

## Acceptance Checks
- `cmake --preset windows-release` succeeds.
- Configure output shows a concrete MSVC include path under `C:/Program Files/Microsoft Visual Studio/.../include`.
- `cmake --build --preset windows-release --target module_philosophy_test_philosophy_loader_focused --parallel 16` no longer fails on missing standard headers like `string`, `atomic`, or `cstdint`.

## Test Scope
- Configure only.
- Focused build for `module_philosophy_test_philosophy_loader_focused`.
