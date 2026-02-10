# LoRA Framework Build-Anleitung

Vollständige Anleitung zum Erstellen und Testen des ThemisDB LoRA Adapter Frameworks mit CMake.

## 📋 Inhaltsverzeichnis

1. [Docker Quick Start](#docker-quick-start) ⭐ **Neu!**
2. [Voraussetzungen](#voraussetzungen)
3. [Schnellstart](#schnellstart)
4. [Build-Optionen](#build-optionen)
5. [Abhängigkeiten](#abhängigkeiten)
6. [Erstellen](#erstellen)
7. [Tests ausführen](#tests-ausführen)
8. [Benchmarks ausführen](#benchmarks-ausführen)
9. [Fehlerbehebung](#fehlerbehebung)

---

## Docker Quick Start

🐳 **Schnellster Einstieg** - Komplette Umgebung in < 5 Minuten!

### Mit Docker Compose

```bash
# Zum Docker-Verzeichnis navigieren
cd docker

# Alle Dienste starten (ThemisDB + Prometheus + Grafana)
./scripts/start.sh

# Zugriff auf Dienste:
# - ThemisDB:   http://localhost:8529
# - Prometheus: http://localhost:9091
# - Grafana:    http://localhost:3000 (admin/admin)
```

### Entwicklungsmodus

```bash
# Mit Hot-Reload und Debug-Unterstützung starten
./scripts/start.sh dev

# Tests in Docker ausführen
./scripts/test.sh

# Logs anzeigen
./scripts/logs.sh themisdb
```

**Vollständige Docker-Dokumentation**: Siehe [docker/README.md](docker/README.md)

---

## Voraussetzungen

### Erforderlich

- **CMake** ≥ 3.20
- **C++17** kompatibler Compiler (GCC ≥ 9, Clang ≥ 10, MSVC ≥ 2019)
- **vcpkg** (empfohlen für Abhängigkeitsverwaltung)

### Erforderliche Abhängigkeiten

```bash
# Kern-Abhängigkeiten (immer erforderlich)
vcpkg install openssl zlib rocksdb fmt spdlog nlohmann-json boost-system boost-filesystem protobuf grpc
```

### Optionale Abhängigkeiten

```bash
# Für Unit-Tests
vcpkg install gtest

# Für Benchmarks
vcpkg install benchmark

# Für Metriken (Prometheus/Grafana-Integration)
vcpkg install prometheus-cpp
```

---

## Schnellstart

### 1. Build mit Standardoptionen

```bash
# Repository klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Build-Verzeichnis erstellen
mkdir build && cd build

# Mit vcpkg konfigurieren
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Erstellen
cmake --build . -j$(nproc)
```

### 2. Build mit Tests und Benchmarks

```bash
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DTHEMIS_BUILD_TESTS=ON \
    -DTHEMIS_BUILD_BENCHMARKS=ON \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --target test_lora_framework bench_lora_framework
```

---

## Build-Optionen

### Kern-Optionen

| Option | Standard | Beschreibung |
|--------|----------|--------------|
| `THEMIS_BUILD_TESTS` | `ON` | Unit-Tests erstellen |
| `THEMIS_BUILD_BENCHMARKS` | `ON` | Performance-Benchmarks erstellen |
| `THEMIS_ENABLE_LLM` | `OFF` | LLM-Features aktivieren |
| `CMAKE_BUILD_TYPE` | `Debug` | Build-Typ: Debug, Release, RelWithDebInfo |

### Beispiel-Konfigurationen

**Entwicklungs-Build** (schnelle Kompilierung, Debugging):
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DTHEMIS_BUILD_TESTS=ON
```

**Produktions-Build** (optimiert):
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DTHEMIS_STRICT_BUILD=ON
```

**Test-Build** (mit Tests und Benchmarks):
```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DTHEMIS_BUILD_TESTS=ON \
    -DTHEMIS_BUILD_BENCHMARKS=ON
```

---

## Abhängigkeiten

### Abhängigkeitserkennung

CMake erkennt verfügbare Abhängigkeiten automatisch:

✅ **Gefunden**: Feature aktiviert mit voller Funktionalität  
⚠️ **Nicht gefunden (Optional)**: Feature wird deaktiviert  
❌ **Nicht gefunden (Erforderlich)**: Build schlägt mit klarer Fehlermeldung fehl

### Abhängigkeiten prüfen

```bash
# CMake mit ausführlicher Ausgabe ausführen
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Auf diese Meldungen achten:
# ✅ "GTest found - tests enabled"
# ✅ "Google Benchmark found - benchmarks enabled"
# ✅ "Prometheus C++ client found - metrics enabled"
# ⚠️ "Prometheus C++ client not found - metrics collection disabled"
```

### Fehlende Abhängigkeiten installieren

#### Mit vcpkg (Empfohlen)

```bash
# Alle optionalen Abhängigkeiten installieren
vcpkg install gtest benchmark prometheus-cpp

# Oder einzeln installieren
vcpkg install gtest           # Für Unit-Tests
vcpkg install benchmark       # Für Performance-Benchmarks
vcpkg install prometheus-cpp  # Für Metrik-Sammlung
```

#### Mit System-Paketmanager

**Ubuntu/Debian:**
```bash
sudo apt-get install libgtest-dev libbenchmark-dev
```

**macOS (Homebrew):**
```bash
brew install googletest google-benchmark prometheus-cpp
```

**Windows:**
vcpkg verwenden (empfohlen)

---

## Erstellen

### Build-Ziele

#### Alle Ziele

```bash
# Alles erstellen
cmake --build . -j$(nproc)
```

#### Spezifische Ziele

```bash
# Nur LoRA Framework Tests erstellen
cmake --build . --target test_lora_framework

# Nur LoRA Framework Benchmarks erstellen
cmake --build . --target bench_lora_framework

# Mit ausführlicher Ausgabe erstellen
cmake --build . --target test_lora_framework --verbose
```

### Build-Ausgabe

```
build/
├── tests/
│   └── test_lora_framework          # Unit-Test Executable
├── benchmarks/
│   └── bench_lora_framework         # Benchmark Executable
└── compile_commands.json            # Für IDE-Integration
```

---

## Tests ausführen

### Basis-Test-Ausführung

```bash
# Alle LoRA Framework Tests ausführen
./tests/test_lora_framework

# Mit ausführlicher Ausgabe ausführen
./tests/test_lora_framework --gtest_verbose=true

# Spezifische Test-Suite ausführen
./tests/test_lora_framework --gtest_filter="*StorageService*"

# Alle verfügbaren Tests auflisten
./tests/test_lora_framework --gtest_list_tests
```

### CTest verwenden

```bash
# Alle Tests über CTest ausführen
ctest --output-on-failure

# Mit ausführlicher Ausgabe ausführen
ctest -V

# Spezifischen Test ausführen
ctest -R LoRAFramework

# Tests parallel ausführen
ctest -j$(nproc)
```

### Test-Ausgabeformate

**XML-Ausgabe** (für CI/CD):
```bash
./tests/test_lora_framework --gtest_output=xml:test_results.xml
```

**JSON-Ausgabe**:
```bash
./tests/test_lora_framework --gtest_output=json:test_results.json
```

### Benutzerdefiniertes Test-Ziel

```bash
# Tests über benutzerdefiniertes Ziel ausführen
make run_tests
# oder
cmake --build . --target run_tests
```

---

## Benchmarks ausführen

### Basis-Benchmark-Ausführung

```bash
# Alle Benchmarks ausführen
./benchmarks/bench_lora_framework

# Spezifischen Benchmark ausführen
./benchmarks/bench_lora_framework --benchmark_filter="Manager_HotSwap"

# Mit minimaler Zeit pro Benchmark ausführen
./benchmarks/bench_lora_framework --benchmark_min_time=0.1

# Mit Wiederholungen für statistische Analyse ausführen
./benchmarks/bench_lora_framework --benchmark_repetitions=10
```

### Benchmark-Ausgabeformate

**JSON-Ausgabe** (für Analyse-Tools):
```bash
./benchmarks/bench_lora_framework --benchmark_out=results.json --benchmark_out_format=json
```

**CSV-Ausgabe**:
```bash
./benchmarks/bench_lora_framework --benchmark_out=results.csv --benchmark_out_format=csv
```

### Benutzerdefinierte Benchmark-Ziele

```bash
# Alle Benchmarks ausführen
make run_benchmarks

# Schnelle Benchmarks ausführen (< 10 Sekunden)
make run_benchmarks_quick

# Detaillierte Benchmarks mit Statistiken ausführen
make run_benchmarks_detailed
```

### Ergebnisse analysieren

```bash
# Zwei Benchmark-Läufe vergleichen
./benchmarks/bench_lora_framework --benchmark_out=baseline.json
# ... Änderungen vornehmen ...
./benchmarks/bench_lora_framework --benchmark_out=current.json

# Vergleichen (benötigt Benchmark-Tools)
python3 compare.py benchmarks baseline.json current.json
```

---

## Fehlerbehebung

### Häufige Probleme

#### 1. GTest nicht gefunden

**Fehler:**
```
GTest not found - tests will not be built
Install with: vcpkg install gtest
```

**Lösung:**
```bash
vcpkg install gtest
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

#### 2. Google Benchmark nicht gefunden

**Fehler:**
```
Google Benchmark not found - benchmarks will not be built
Install with: vcpkg install benchmark
```

**Lösung:**
```bash
vcpkg install benchmark
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

#### 3. Prometheus nicht gefunden (nicht kritisch)

**Warnung:**
```
Prometheus C++ client not found - metrics collection disabled
```

**Lösung (Optional):**
```bash
vcpkg install prometheus-cpp
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

Dies ist optional - Metrik-Features werden deaktiviert, aber das Framework funktioniert weiterhin.

#### 4. Fehlende Header in Tests/Benchmarks

**Fehler:**
```
fatal error: llm/lora_framework/lora_adapter_manager.h: No such file or directory
```

**Lösung:**
Include-Verzeichnisse überprüfen:
```bash
# Prüfen, ob Header existieren
ls -la include/llm/lora_framework/

# CMake neu konfigurieren
rm -rf build/*
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

#### 5. Linker-Fehler

**Fehler:**
```
undefined reference to `themis::llm::lora::LoRAAdapterManager::loadAdapter`
```

**Lösung:**
Implementierungsdateien sind noch nicht kompiliert. Dies ist zu erwarten, bis die Hauptbibliothek erstellt wird.
Die Tests und Benchmarks enthalten derzeit Platzhalter-/Mock-Implementierungen.

---

## Erweiterte Konfiguration

### Cross-Kompilierung

```bash
# Für ARM64
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=arm64-linux
```

### Statisches Linken

```bash
cmake .. \
    -DTHEMIS_STATIC_BUILD=ON \
    -DCMAKE_BUILD_TYPE=Release
```

### Mit Sanitizers (Debug)

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DTHEMIS_ENABLE_ASAN=ON
```

---

## CI/CD-Integration

### GitHub Actions Beispiel

```yaml
name: LoRA Framework Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Abhängigkeiten installieren
        run: |
          vcpkg install gtest benchmark prometheus-cpp
      
      - name: Konfigurieren
        run: |
          mkdir build && cd build
          cmake .. \
            -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
            -DTHEMIS_BUILD_TESTS=ON \
            -DTHEMIS_BUILD_BENCHMARKS=ON
      
      - name: Erstellen
        run: cmake --build build --target test_lora_framework
      
      - name: Testen
        run: |
          cd build
          ctest --output-on-failure
```

---

## Performance-Tipps

### Build-Performance

1. **Ninja statt Make verwenden:**
   ```bash
   cmake .. -G Ninja
   ninja test_lora_framework
   ```

2. **Parallele Builds:**
   ```bash
   cmake --build . -j$(nproc)
   ```

3. **Ccache für schnellere Rebuilds:**
   ```bash
   cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
   ```

### Test-Performance

1. **Tests parallel ausführen:**
   ```bash
   ctest -j$(nproc)
   ```

2. **Langsame Tests während Entwicklung überspringen:**
   ```bash
   ./tests/test_lora_framework --gtest_filter=-*Slow*
   ```

---

## Nächste Schritte

1. **Tests erkunden**: Siehe `LORA_TESTING_AND_METRICS_GUIDE.md`
2. **Beispiele ansehen**: Siehe `LORA_USAGE_EXAMPLES.md`
3. **Architektur lernen**: Siehe `LLM_LORA_UNIFIED_ARCHITECTURE.md`
4. **Integration**: Siehe `LLM_LORA_LLAMACPP_INTEGRATION.md`

---

## Support

Für Fragen oder Probleme:
- 📖 Dokumentation prüfen: `docs/`
- 🐛 Bugs melden: GitHub Issues
- 💬 Diskutieren: GitHub Discussions
