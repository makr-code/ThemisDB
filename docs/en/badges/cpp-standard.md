# C++ Standard Badge

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20)

## What it shows

A static badge indicating that ThemisDB targets the **C++20** standard. The codebase uses modern C++ features such as concepts, ranges, coroutines (where supported), and structured bindings.

## What it does NOT guarantee

- Specific compiler support depends on the toolchain version (GCC ≥ 11, Clang ≥ 13, MSVC 19.29+).
- Not all C++20 features may be used uniformly across all modules; some platform-specific code may target a narrower subset.

## Source of truth

| Source | URL |
|--------|-----|
| Build configuration | [`CMakeLists.txt`](../../../CMakeLists.txt) in the repository root |
| C++20 reference | <https://en.cppreference.com/w/cpp/20> |

## How contributors can verify

```bash
cmake -S . -B build
cmake --build build --verbose 2>&1 | grep "std=c++20\|-std:c++20"
```
