# Platform Badge

[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey)](https://github.com/makr-code/ThemisDB/blob/develop/CONTRIBUTING.md)

## What it shows

A static badge listing the operating systems on which ThemisDB can be built and run:

- **Linux** – primary target; Ubuntu 22.04 LTS is the reference platform used in CI.
- **macOS** – supported via CMake + Homebrew/vcpkg; tested on Apple Silicon and Intel.
- **Windows** – supported via CMake + vcpkg + MSVC (Visual Studio 2022); `.ps1` build script available.

## What it does NOT guarantee

- All features are verified on Linux. macOS and Windows support may lag for edge cases.
- GPU acceleration requires CUDA-capable hardware and is Linux/Windows only (CUDA is not supported on macOS).

## Source of truth

| Source | URL |
|--------|-----|
| CI platform (Linux) | [`.github/workflows/01-core_themis-core-ci.yml`](../../../.github/workflows/01-core_themis-core-ci.yml) |
| Windows build script | [`setup-build-env.ps1`](../../../setup-build-env.ps1) |
| Build documentation | [`SETUP.md`](../../../SETUP.md) |

## How contributors can verify

Follow the platform-specific instructions in [`SETUP.md`](../../../SETUP.md) to set up a build environment on your OS.
