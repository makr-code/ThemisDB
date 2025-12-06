Project: Themis (Database System)
Language: C++

Purpose:
- High-performance C++ vector database with RocksDB integration, AQL and MVCC.

What Copilot should help with:
- Generate idiomatic C++ code for core algorithms, concurrency control, and memory-safe structures.
- Suggest tests for correctness and performance; avoid unsafe patterns.

Coding style and constraints:
- Follow project C++ style; prefer modern C++ (C++17/20) features and RAII patterns.
- Document threading and locking choices in `docs/architecture.md`.

Documentation duties (./docs):
- Add `docs/design.md` describing AQL semantics, indexing strategy and storage layout.

Todo.md continuation:
- Add clear development tasks with benchmark targets and unit test coverage goals.

Examples for Copilot prompts:
- "Implement MVCC snapshot read path with minimal locking and add unit tests simulating concurrent writers."

Testing & CI:
- Provide unit tests and a micro-benchmark harness; CI should run static analysis and unit tests.

---

## Build-Strategie nach Plattform

### Linking-Strategie

| Plattform | Linking | CMake-Flag |
|-----------|---------|------------|
| **Docker/QNAP** | **Monolithisch (Statisch)** | `-DTHEMIS_STATIC_BUILD=ON` |
| **Windows** | **Dynamisch (DLL)** | `-DTHEMIS_STATIC_BUILD=OFF` |

### Docker Build: Hybrid Pre-built Binary

Der empfohlene Ansatz für Docker-Builds ist der **Hybrid Pre-built Binary** Workflow:

### Unified Docker Build Script
- `docker-build.ps1` - PowerShell (Windows/WSL)

### Workflow
1. Binary **monolithisch** mit vcpkg bauen: `cmake -DTHEMIS_STATIC_BUILD=ON ...`
2. Docker-Image mit `Dockerfile.simple` erstellen (schnell)
3. Ergebnis: Kleine Images (~100-200 MB), 100% offline-fähig

### Verwendung
```powershell
# Standard Build
.\docker-build.ps1

# Mit Binary-Build in WSL
.\docker-build.ps1 -BuildBinary

# QNAP Variante
.\docker-build.ps1 -Variant qnap

# Push zu Registry
.\docker-build.ps1 -Push
```

### Unterstützte Plattformen
| Plattform | Architektur | Linking | Use Case |
|-----------|-------------|---------|----------|
| `linux/amd64` | x86_64 | Statisch | Server, Desktop, QNAP NAS |
| `linux/arm64` | ARM64 | Statisch | Raspberry Pi 4/5, ARM Server |

### Entfernte/Ersetzte Skripte
Die folgenden Skripte wurden durch `docker-build.ps1` ersetzt:
- ~~`build-docker-qnap.ps1`~~ → `docker-build.ps1 -Variant qnap`
- ~~`build-docker-simple.ps1`~~ → `docker-build.ps1 -BuildBinary`
- ~~`build-rpi.ps1`~~ / ~~`build-rpi.sh`~~ → Lokal mit `-DTHEMIS_STATIC_BUILD=ON` bauen
- ~~`docker-build-push.ps1`~~ → `docker-build.ps1 -Push`

---

## Build-Pfade (aktive Konfigurationen):

1. build-qnap/
	- Ziel: Statischer Release-Build für QNAP / Ubuntu 20.04 (GLIBC 2.31)
	- Generator: Ninja
	- Toolchain: /opt/vcpkg (Manifest-Modus, Triplet x64-linux, manuelle Prefix-Hints)
	- Flags: -DTHEMIS_STATIC_BUILD=ON, -DTHEMIS_ENABLE_TRACING=ON, Tests & Benchmarks ON
	- Prefix: CMAKE_PREFIX_PATH=/src/vcpkg_installed/x64-linux
	- Wichtige Pakete: RocksDB, Arrow, OpenSSL 3.6.0, Curl (statisch), Zstd
	- Artefakte: libthemis_core.a, themis_server (Release, etwa ~35MB)
	- Besonderheiten: -DVCPKG_MANIFEST_INSTALL=OFF (Manifest schon vor Configure installiert); statische Abhängigkeiten (BUILD_SHARED_LIBS=OFF)

2. build-linux-gcc-release/
	- Ziel: Linux Release (GCC) mit statischem Build (THEMIS_STATIC_BUILD=ON)
	- Generator: Standard (vermutlich Unix Makefiles oder Ninja, Cache zeigt /usr/local/bin/cmake Erzeugung)
	- Toolchain: /opt/vcpkg (Manifest INSTALL ON)
	- Build-Type: Release
	- Artefakte: Entspricht regulärem Server/Linux Release Build (dynamische/teilweise statische Libs je nach vcpkg Port)

3. build-msvc-ninja-release/
	- Ziel: Windows Release Build (MSVC, Ninja)
	- Compiler: MSVC 14.44 (cl.exe)
	- Build-Type: Release
	- Toolchain: vcpkg x64-windows (Arrow_DIR, Boost_DIR etc. im lokalen vcpkg_installed)
	- Artefakte: themis_server.exe, libthemis_core.lib
	- Install-Prefix: C:/Program Files (x86)/Themis vorgesehen

4. Weitere vorhandene Verzeichnisse (Status: nicht im Fokus/QNAP):
	- build-msvc-ninja-debug/ (Windows Debug)
	- build-linux-clang-debug/, build-linux-clang-release/ (Clang Builds)
	- build-linux-clang-debug-bench/ (Clang + Benchmarks)
	- build-linux-gcc-release/ (siehe oben)
	- build-ninja/ (Allgemeiner Ninja Build – ggf. Standard Preset Test)
	- build/ (Legacy/Initial CMake Build-Ausgabe)

Pfad- und Strukturregeln für neue Build-Verzeichnisse:
	- Nutze eindeutige Namenskonvention: build-<platform>-<compiler>-<config>[-optionalTag]
	- Immer separates vcpkg_installed Unterverzeichnis, falls Manifest-Modus genutzt (automatisch durch CMake/vcpkg)
	- Für portable statische Builds: BUILD_SHARED_LIBS=OFF + THEMIS_STATIC_BUILD=ON + explizites CMAKE_PREFIX_PATH auf vcpkg_installed/<triplet>
	- Keine doppelte Manifest-Installation: Vor dem Configure einmal `vcpkg install`, dann `-DVCPKG_MANIFEST_INSTALL=OFF`

QNAP Release Rebuild Ablauf (Referenz):
	1. Manifest kopieren (vcpkg.qnap.json -> vcpkg.json)
	2. `bootstrap-vcpkg.sh`
	3. `vcpkg install --triplet x64-linux --clean-after-build` (mit Retry)
	4. `cmake -S . -B build-qnap -G Ninja -DCMAKE_BUILD_TYPE=Release -DTHEMIS_STATIC_BUILD=ON -DVCPKG_MANIFEST_INSTALL=OFF -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_PREFIX_PATH="$(pwd)/vcpkg_installed/x64-linux" -DRocksDB_DIR="$(pwd)/vcpkg_installed/x64-linux/share/rocksdb" -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake`
	5. `cmake --build build-qnap --target themis_server --parallel`
	6. Optional: GLIBC Prüfung via Ubuntu 20.04 Container (`ldd build-qnap/themis_server | grep GLIBC`)

Erwartungen an Copilot bei Build-Themen:
	- Bei Änderung an `build-qnap.sh`: Nur notwendige Flags anpassen, keine Preset-Einführung.
	- Statische Build-Optimierungsvorschläge nur nach Prüfung auf symbolische Duplikate (nm -C | sort | uniq -d).
	- Bei Paket-Find-Problemen zuerst CMAKE_PREFIX_PATH / *_DIR prüfen, erst danach find_package Log erweitern.
	- Docker-Builds immer mit `docker-build.ps1/.sh` (Hybrid Pre-built Binary Ansatz)

Bitte neue Build-Profile konsistent mit obiger Matrix ergänzen und Unterschiede klar dokumentieren.
