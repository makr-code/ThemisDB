# ThemisDB Quick Build Guide
## Erfolgreicher Build mit Optimierungen

**Problem:** Bisherige Docker/WSL-Builds sind fehlgeschlagen
**Lösung:** Direkter MSVC-Build auf Windows oder vereinfachter WSL-Build

---

## Option 1: MSVC Build (Windows Native) - EMPFOHLEN

```powershell
cd C:\VCC\themis

# Erstelle neues Build-Verzeichnis
mkdir -p build-final
cd build-final

# CMake Configure mit MSVC
cmake -S .. -B . -G "Visual Studio 17 2022" `
  -DCMAKE_BUILD_TYPE=Release `
  -DTHEMIS_BUILD_TESTS=OFF `
  -DTHEMIS_BUILD_BENCHMARKS=OFF `
  -DTHEMIS_ENABLE_TRACING=OFF

# Build mit maximalen Optimierungen
cmake --build . --config Release --target themis_server -- /MP /O2 /Oi /Ot /Ox
```

**Erwartete Dauer:** 20-30 Minuten
**Output:** `build-final\Release\themis_server.exe`

---

## Option 2: WSL Linux Build (faster für diesen Build)

```bash
# Von Windows PowerShell:
wsl -d Ubuntu -u root -e bash -c '
  cd /mnt/c/VCC/themis
  
  # Installiere CMake falls nicht vorhanden
  apt-get update && apt-get install -y cmake ninja-build build-essential openmp libssl-dev
  
  # Build
  mkdir -p build-optimized
  cd build-optimized
  cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -fopenmp -mtune=native -mavx2 -mfma" \
    -DTHEMIS_BUILD_TESTS=OFF \
    -DTHEMIS_BUILD_BENCHMARKS=OFF
  
  ninja -j20 themis_server
  
  # Validierung
  echo "=== Binary Info ==="
  file themis_server
  ls -lh themis_server
  
  echo "=== SIMD Flags Check ==="
  objdump -d themis_server | grep -E "vmul|vpadd|vpshuf" | head -5 || echo "No direct SIMD in main binary (probably in libs)"
  
  echo "=== OpenMP Check ==="
  nm themis_server | grep omp | head -5
'
```

**Erwartete Dauer:** 15-25 Minuten
**Output:** `build-optimized/themis_server`

---

## Option 3: Nutze existierenden MSVC Binary + Docker

Falls Build zu lange dauert, nutze bestehende Binary:

```powershell
# Existierende Binary:
# C:\VCC\themis\build-msvc\Release\themis_server.exe (9.6 MB, von 11:17)

# Docker Image mit dieser Binary:
docker build -t themis:with-existing-binary -f Dockerfile.optimized-local .

# Validiere im Container:
docker run --rm themis:with-existing-binary bash -c "
  objdump -d /app/themis_server | grep -E 'omp|simd' | head -10 || echo 'No OMP found'
  strings /app/themis_server | grep -i 'simd\|omp\|prefetch' | head -5 || echo 'No optimization strings'
"
```

---

## Kritische CMake Flags für Optimierungen

```cmake
# SIMD Vektorisierung aktivieren
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mavx2 -mfma -fopenmp -mtune=native")

# OpenMP aktivieren
find_package(OpenMP REQUIRED)
target_link_libraries(themis_core PUBLIC OpenMP::OpenMP_CXX)

# Link-Time Optimization (optional, verschlängert Build)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
```

---

## Post-Build Validierung

Nach erfolgreichem Build: Validiere dass Optimierungen kompiliert wurden

```bash
# 1. SIMD Instruktionen prüfen
objdump -d /path/to/themis_server | grep -E "vmulpd|vpaddq|vpblendvb|vpaddb" | wc -l
# Sollte > 0 sein

# 2. Prefetch Instruktionen prüfen
objdump -d /path/to/themis_server | grep -i prefetch | wc -l
# Sollte > 0 sein

# 3. OpenMP Symbols prüfen
nm /path/to/themis_server | grep -E "__omp_|GOMP_" | wc -l
# Sollte > 0 sein

# 4. Size vergleichen (mit Optimierungen sollte ähnlich oder kleiner sein)
ls -lh /path/to/themis_server
```

---

## Docker Build nach erfolgreichem lokalen Build

Sobald lokaler Build erfolgreich ist:

```powershell
# Kopiere Binary zu build-wsl wenn WSL Build verwendet
cp C:\VCC\themis\build-optimized\themis_server C:\VCC\themis\build-wsl\

# Docker Build mit lokaler Binary
docker build -t themis:optimized-final -f Dockerfile.optimized-local .

# Test Container
docker run --rm -e DEBUG=1 themis:optimized-final /app/themis_server --version
```

---

## Benchmark-Vergleich

Nach Docker-Build Benchmark ausführen:

```bash
docker run --rm themis:optimized-final bash -c "
  cd /app/benchmarks
  python3 run_complete_benchmarks.py --config vector_intensive --output /tmp/results.json
  cat /tmp/results.json
"
```

Vergleiche Results mit Baseline:
- Baseline: `BENCHMARK_RESULTS_COMPLETE_2025.json` (75% Grade B)
- Expected: 85-90% Grade A- (~40% improvement)

---

## Debugging wenn Build fehlschlägt

```bash
# 1. CMake Cache zurücksetzen
rm -rf build-* CMakeCache.txt

# 2. Abhängigkeiten validieren
cmake -LAH  # Zeige alle CMake Variablen

# 3. Kompilierungs-Fehler mit Verbosity
cmake --build . --verbose

# 4. Linker-Fehler debuggen
cmake --build . -- VERBOSE=1
```

---

## Zeitschätzung

| Build-Methode | Dauer | Qualität |
|---|---|---|
| MSVC (Windows) | 20-30 min | Höchste (native MSVC Optimierungen) |
| WSL Ninja | 15-25 min | Gut (GCC Optimierungen) |
| Docker Multi-stage | 60+ min | Gut (aber Zeit-intensiv) |
| Existierende Binary + Docker | 5 min | Schnell, validiert Best-Case |

**Empfehlung:** WSL Ninja Build oder MSVC Build (schneller, dann Docker wrapper)

