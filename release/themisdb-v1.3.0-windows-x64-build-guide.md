# ThemisDB v1.3.0 Windows x64 Build Guide

**Stand:** 22. Dezember 2025  
**Version:** v1.3.0  
**Plattform:** Windows 11/Server 2022, Visual Studio 2022

---

## Schnellstart (5 Minuten)

### Voraussetzungen
- Visual Studio 2022 Community/Professional
- CMake 3.20+
- vcpkg (bereits in `C:\VCC\themis\vcpkg`)
- Python 3.9+

### Build v1.3.0 Debug
```powershell
cd C:\VCC\themis

# Configure
cmake --preset windows-vs2022-debug `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_BUILD_TESTS=ON

# Build
cmake --build build-msvc-debug --config Debug --parallel 4

# Verify
.\build-msvc-debug\Debug\themis_server.exe --help
```

### Build v1.3.0 Release
```powershell
# Configure
cmake --preset windows-vs2022-release `
  -DTHEMIS_ENABLE_LLM=ON `
  -DTHEMIS_BUILD_BENCHMARKS=ON

# Build
cmake --build build-msvc --config Release --parallel 8

# Run
.\build-msvc\Release\themis_server.exe --help
```

## Test Suite ausführen

```powershell
cd C:\VCC\themis\build-msvc-debug

# Run all GTests
ctest --output-on-failure -C Debug

# Run specific test
ctest -N | grep vector_search
ctest --output-on-failure -C Debug -R vector_search
```

## Benchmarks ausführen

```powershell
cd C:\VCC\themis\build-msvc-debug

# Core performance benchmarks
.\Debug\bench_core_performance.exe

# Comprehensive suite
.\Debug\bench_comprehensive.exe

# Specific benchmarks
.\Debug\bench_compression.exe
.\Debug\bench_mvcc.exe
.\Debug\bench_encryption.exe
```

## Docker Alternative (empfohlen für Tests)

```powershell
# Build Docker image
docker build -t themis:v1.3.0 -f Dockerfile .

# Run with tests
docker run --rm themis:v1.3.0 /bin/bash -c "cmake --build build-wsl -j8 && ctest --output-on-failure"

# Run benchmarks
docker run --rm themis:v1.3.0 /bin/bash -c "./build-wsl/bench_core_performance"
```

## Build Options

| Option | Wert | Beschreibung |
|--------|------|-------------|
| `DTHEMIS_ENABLE_LLM` | ON/OFF | LLM Plugin mit llama.cpp |
| `DTHEMIS_ENABLE_CUDA` | ON/OFF | NVIDIA CUDA GPU |
| `DTHEMIS_BUILD_TESTS` | ON/OFF | GTest Testsuite |
| `DTHEMIS_BUILD_BENCHMARKS` | ON/OFF | Performance Benchmarks |
| `DCMAKE_BUILD_TYPE` | Debug/Release | Optimierung |

## Troubleshooting

### CMake Fehler: gRPC nicht gefunden
**Ursache:** gRPC optional, nicht nötig für Tests  
**Lösung:** Ignorieren oder `vcpkg install grpc:x64-windows` (optional)

### MSBUILD Fehler: Projektdatei nicht vorhanden
**Ursache:** CMake Targets richtig registriert  
**Lösung:** `cmake --build . --config Debug` ohne `--target`

### LLM Build fehlgeschlagen
**Ursache:** llama.cpp nicht geclont  
**Lösung:** `git clone https://github.com/ggerganov/llama.cpp.git llama.cpp` im Rootverzeichnis

## Dokumentation

- [Architecture](../docs/de/architecture/README.md)
- [Performance Benchmarks](../docs/de/performance/README.md)
- [Security](../docs/de/security/README.md)
- [Test Suite](../docs/de/development/README.md)

