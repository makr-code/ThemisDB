> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Docker RAID Benchmark - Build Instructions

## Status

✅ **Benchmark-Code erstellt** (1.300+ Zeilen)  
✅ **CMake-Integration vorbereitet**  
✅ **Dokumentation vollständig**  
⚠️ **Build noch nicht durchgeführt** (fehlende Benchmark-Stub-Dateien)

## Problem

Das ThemisDB CMakeLists.txt definiert viele Benchmark-Targets deren Quelldateien nicht existieren. Beim Aktivieren von `THEMIS_BUILD_BENCHMARKS=ON` schlägt die CMake-Konfiguration aufgrund dieser fehlenden Dateien fehl.

## Lösung 1: Stub-Dateien erstellen (Empfohlen)

Erstelle leere/minimale C++ Dateien für alle fehlenden Benchmarks:

```bash
cd C:\VCC\themis\compendium\benchmarks

# Liste aller fehlenden Benchmark-Dateien (aus CMake-Errors extrahiert)
$missingBenchmarks = @(
    "bench_content_versioning",
    "bench_text_extraction",
    "bench_image_analysis",
    "bench_image_analysis_latency",
    # ... (vollständige Liste im CMake Error Log)
)

foreach ($bench in $missingBenchmarks) {
    $stubContent = @"
// Stub benchmark for $bench
#include <benchmark/benchmark.h>

static void BM_Placeholder(benchmark::State& state) {
    for (auto _ : state) {
        // TODO: Implement benchmark
    }
}
BENCHMARK(BM_Placeholder);

BENCHMARK_MAIN();
"@
    $stubContent | Out-File -FilePath "$bench.cpp" -Encoding UTF8
}
```

## Lösung 2: Isolierter Build (Temporär)

Erstelle einen separaten CMakeLists.txt nur für den Docker RAID Benchmark:

```cmake
cmake_minimum_required(VERSION 3.20)
project(DockerRAIDBenchmark CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(benchmark CONFIG REQUIRED)

# Falls themis_core nicht verfügbar ist, erstelle Mock
add_library(themis_core_mock INTERFACE)
target_include_directories(themis_core_mock INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

add_executable(bench_docker_raid_comprehensive
    bench_docker_raid_comprehensive.cpp
)

target_link_libraries(bench_docker_raid_comprehensive
    PRIVATE
        themis_core_mock
        benchmark::benchmark
        benchmark::benchmark_main
)
```

**Build-Befehl:**
```powershell
cd C:\VCC\themis\compendium\benchmarks
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows `
    ..
cmake --build . --config Release
```

## Lösung 3: Manueller Compile (Schnelltest)

Falls Visual Studio Developer Command Prompt verfügbar:

```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"

cd C:\VCC\themis\compendium\benchmarks

cl /EHsc /std:c++17 ^
   /I"..\..\build-msvc\vcpkg_installed\x64-windows\include" ^
   /I"..\..\include" ^
   bench_docker_raid_comprehensive.cpp ^
   /link ^
   /LIBPATH:"..\..\build-msvc\vcpkg_installed\x64-windows\lib" ^
   benchmark.lib ^
   /OUT:bench_docker_raid_comprehensive.exe
```

## Test nach erfolgreichem Build

### Quick-Test (~10 Min)
```powershell
.\bench_docker_raid_comprehensive.exe --benchmark_filter="SmallDocumentWrite/3/0" --benchmark_min_time=1 --benchmark_repetitions=1
```

### Vollständiger Test (~1-2 Stunden)
```powershell
.\run_docker_raid_benchmark.ps1 -MinTime 60 -Repetitions 3
```

## Verifikation

Nach erfolgreichem Build sollte die Datei existieren:
- `C:\VCC\themis\build-msvc\Release\bench_docker_raid_comprehensive.exe` (~5-15 MB)

Test mit:
```powershell
.\bench_docker_raid_comprehensive.exe --help
```

Erwartete Ausgabe:
```
benchmark [--benchmark_list_tests={true|false}]
          [--benchmark_filter=<regex>]
          [--benchmark_min_time=<min_time>]
          [--benchmark_repetitions=<num_repetitions>]
          ...
```

## Nächste Schritte

1. **Stub-Dateien erstellen** für fehlende Benchmarks im ThemisDB Projekt
2. **CMake regenerieren** mit `THEMIS_BUILD_BENCHMARKS=ON`
3. **Docker RAID Benchmark bauen**
4. **QuickTest ausführen**
5. **Vollständigen Benchmark-Run** durchführen (1+ Stunde)
6. **Ergebnisse analysieren** mit `analyze_raid_benchmarks.py`

## Dateien

- **Benchmark-Code**: [bench_docker_raid_comprehensive.cpp](../bench_docker_raid_comprehensive.cpp)
- **Dokumentation**: [DOCKER_RAID_BENCHMARK_SUITE_README.md](DOCKER_RAID_BENCHMARK_SUITE_README.md)
- **Runner-Script**: [run_docker_raid_benchmark.ps1](../run_docker_raid_benchmark.ps1)
- **Analyse-Tool**: [analyze_raid_benchmarks.py](../analyze_raid_benchmarks.py)
- **Implementation Summary**: [DOCKER_RAID_IMPLEMENTATION_SUMMARY.md](DOCKER_RAID_IMPLEMENTATION_SUMMARY.md)

## Kontakt

Bei Fragen oder Problemen siehe [DOCKER_RAID_BENCHMARK_SUITE_README.md](DOCKER_RAID_BENCHMARK_SUITE_README.md) Abschnitt "Troubleshooting".
