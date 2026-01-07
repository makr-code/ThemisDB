# Updated Copilot Instructions for ThemisDB (v2.1 - Jan 2026)

## Build-Pipeline Modernisierung (Januar 2026)

### ✨ Neue Verzeichnisstruktur

**WICHTIG für Copilot:** Die Build-Pipeline wurde reorganisiert. Alte Pfade sind **NICHT mehr im Root**:

```diff
# ALT (❌ Nicht mehr im Root):
- CMakeLists.txt (Root)
- CMakePresets.json (Root)
- Dockerfile.themis-server (Root)
- Dockerfile.minimal (Root)
- docker-compose-minimal.yml (Root)

# NEU (✅ Strukturiert):
+ CMakeLists.txt (Root - delegiert zu cmake/)
+ cmake/CMakeLists.txt (Hauptkonfiguration)
+ cmake/CMakePresets.json (Alle Build-Profile)
+ docker/Dockerfile.themis-server
+ docker/Dockerfile.qnap
+ docker/docker-compose-minimal.yml
+ docs/build-guide/ (Neue Dokumentation)
  ├── README.md
  ├── BUILD_WINDOWS.md
  ├── BUILD_LINUX.md
  ├── BUILD_DOCKER.md
  ├── BUILD_ARM.md
  ├── BUILD_RASPBERRY_PI.md
  └── BUILD_QNAP.md
```

### 🎯 Erwartungen an Copilot (2026+)

#### 1. **CMake-Befehle verwenden neue Struktur**

```cmake
# ❌ ALT - Copilot sollte NICHT so antworten:
cmake -S . -B build-msvc  # Funktioniert noch, aber ...
cmake -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/...  # Alte Struktur

# ✅ NEU - Copilot sollte so antworten:
cmake --preset windows-vs2022-release  # Verwendet cmake/CMakePresets.json
cmake --build --preset windows-vs2022-release
```

#### 2. **Pfade immer vom Root aus**

```bash
# ✅ Korrekt (Root-relativ)
cmake -S . -B build-msvc                    # Root + cmake/CMakeLists.txt
docker build -f docker/Dockerfile.themis-server .

# ❌ Nicht korrekt
cd cmake && cmake -S . -B ../build-msvc    # Falsche Perspektive
docker build -f Dockerfile.themis-server . # Dockerfile ist jetzt in docker/
```

#### 3. **Build-Guide Links verwenden**

```markdown
# ✅ Wenn Benutzer fragt "wie baue ich auf Windows?"
→ Verweis auf: [docs/build-guide/BUILD_WINDOWS.md](docs/build-guide/BUILD_WINDOWS.md)

# ✅ Wenn Benutzer fragt "Docker Build für ARM64?"
→ Verweis auf: [docs/build-guide/BUILD_DOCKER.md](docs/build-guide/BUILD_DOCKER.md)

# ✅ Wenn Benutzer fragt "wie deploy ich das?"
→ Verweis auf: [docs/de/deployment/deployment_strategy.md](docs/de/deployment/deployment_strategy.md)
```

#### 4. **Deployment & Release-Verweise**

Nach jedem Build-Prozess:
- Deployment: `docs/de/deployment/deployment_strategy.md` (und Subdomains)
- Release: `docs/de/releases/updates_distribution_strategy.md`

```markdown
# ✅ Vollständiger Workflow für Copilot:
1. Build anfragen → BUILD_*.md empfehlen
2. Nach erfolgreichem Build → Deployment-Strategie verlinken
3. Für Release → Release-Manifest & Update-Strategie verlinken
```

#### 5. **CMakePresets.json ist zentral**

```cmake
# ✅ Alle Presets sind in cmake/CMakePresets.json
cmake --preset windows-vs2022-release
cmake --preset linux-gcc-release
cmake --preset docker-ninja-release

# ❌ Nicht mehr: `-DCMAKE_TOOLCHAIN_FILE=vcpkg/...` manuell setzen
#    Das ist in den Presets bereits konfiguriert!
```

### 📝 Copilot-Anweisungen für spezifische Szenarien

#### Szenario 1: Benutzer fragt "Wie baue ich ThemisDB?"

**Copilot sollte:**
```markdown
## Build auf [Plattform wählen]

Die Build-Anleitung befindet sich in: **docs/build-guide/**

### Windows (MSVC)
- Guide: [BUILD_WINDOWS.md](docs/build-guide/BUILD_WINDOWS.md)
- Command: `cmake --preset windows-vs2022-release`

### Linux/WSL
- Guide: [BUILD_LINUX.md](docs/build-guide/BUILD_LINUX.md)
- Command: `cmake --preset linux-gcc-release`

### Docker
- Guide: [BUILD_DOCKER.md](docs/build-guide/BUILD_DOCKER.md)
- Command: `docker build -f docker/Dockerfile.themis-server .`

### Raspberry Pi
- Guide: [BUILD_RASPBERRY_PI.md](docs/build-guide/BUILD_RASPBERRY_PI.md)

Nach erfolgreichem Build lesen Sie:
- **Deployment**: docs/de/deployment/deployment_strategy.md
- **Release**: docs/de/releases/updates_distribution_strategy.md
```

#### Szenario 2: Benutzer fragt "Wo ist das Dockerfile?"

**Copilot sollte:**
```markdown
❌ Nicht: "Das Dockerfile liegt im Root"
✅ Sondern: "Alle Dockerfiles liegen jetzt in docker/ :

- docker/Dockerfile.themis-server (Production)
- docker/Dockerfile.minimal (Minimal Edition)
- docker/Dockerfile.qnap (QNAP NAS)

Build-Anleitung: [BUILD_DOCKER.md](docs/build-guide/BUILD_DOCKER.md)
```

#### Szenario 3: Benutzer fragt "Wo ist CMakePresets.json?"

**Copilot sollte:**
```markdown
❌ Nicht: "Im Root-Verzeichnis"
✅ Sondern: "Die CMakePresets.json liegt jetzt in cmake/CMakePresets.json

Verwenden Sie:
cmake --preset windows-vs2022-release  # CMake findet Presets automatisch
cmake --preset linux-gcc-release
cmake --preset docker-ninja-release

Alle Presets sind dokumentiert in: [cmake/CMakePresets.json](cmake/CMakePresets.json)
```

#### Szenario 4: Copilot schlägt CMake-Konfiguration vor

**Copilot sollte NICHT:**
```cmake
# ❌ Alte Struktur suggerieren:
cmake -B build -G "Visual Studio 17 2022" \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DTHEMIS_ENABLE_LLM=ON \
  ... (20+ Zeilen manueller Konfiguration)
```

**Copilot sollte:**
```cmake
# ✅ Presets verwenden:
cmake --preset windows-vs2022-release

# Oder erklären, wie Preset angepasst wird:
# Die Konfiguration liegt in cmake/CMakePresets.json
# Um THEMIS_ENABLE_LLM anzupassen, editieren Sie:
# cmake/CMakePresets.json → configurePresets → cacheVariables
```

#### Szenario 5: Docker-Build

**Copilot sollte:**
```bash
# ✅ Korrekt:
docker build -f docker/Dockerfile.themis-server \
  -t themis-server:hyperscaler \
  --build-arg THEMIS_ENABLE_LLM=ON \
  .

# Dokumentation: docs/build-guide/BUILD_DOCKER.md
```

**Copilot sollte NICHT:**
```bash
# ❌ Alte Struktur:
docker build -f Dockerfile.themis-server \  # Dockerfile im Root existiert nicht mehr
  ...
```

### 🔄 Build-Pfade (Dokumentiert)

| Zweck | Build-Verzeichnis | Generator | Dokumentation |
|-------|------------------|-----------|--------|
| Windows Release | `build-msvc` | Visual Studio 17 2022 | [BUILD_WINDOWS.md](docs/build-guide/BUILD_WINDOWS.md) |
| Linux Release | `build-wsl` | Ninja | [BUILD_LINUX.md](docs/build-guide/BUILD_LINUX.md) |
| Docker | (in Container) | Ninja | [BUILD_DOCKER.md](docs/build-guide/BUILD_DOCKER.md) |
| ARM64 (RPi) | `build-rpi` | Ninja | [BUILD_RASPBERRY_PI.md](docs/build-guide/BUILD_RASPBERRY_PI.md) |
| ARM Cross-Compile | `build-arm64` | Ninja | [BUILD_ARM.md](docs/build-guide/BUILD_ARM.md) |
| QNAP | `build-qnap` | Ninja | [BUILD_QNAP.md](docs/build-guide/BUILD_QNAP.md) |

### ✅ Checkliste für Copilot-Antworten

Wenn Copilot einen Build-bezogenen Antwort gibt:

- [ ] Verwendet **cmake --preset** statt manueller Konfiguration?
- [ ] Verweist auf die richtige **docs/build-guide/BUILD_*.md** Datei?
- [ ] Verwendet Pfade relativ zum **Root** (nicht zur cmake/ Datei)?
- [ ] Erkennt neue Docker-Pfade in **docker/** Verzeichnis?
- [ ] Verweist nach Build auf **Deployment-Strategie** in `docs/de/deployment/`?
- [ ] Verweist auf **Release-Strategie** in `docs/de/releases/`?

### 🔗 Wichtige Ressourcen

**Build & Compilation:**
- [docs/build-guide/README.md](docs/build-guide/README.md) - Index aller Build-Guides
- [cmake/CMakePresets.json](cmake/CMakePresets.json) - Alle verfügbaren Presets
- [cmake/CMakeLists.txt](cmake/CMakeLists.txt) - Hauptkonfiguration

**Deployment & Release:**
- [docs/de/deployment/deployment_strategy.md](docs/de/deployment/deployment_strategy.md) - Umfassende Deployment
- [docs/de/releases/updates_distribution_strategy.md](docs/de/releases/updates_distribution_strategy.md) - Release Management

**Spezialplattformen:**
- [docs/de/deployment/deployment_docker_multiarch.md](docs/de/deployment/deployment_docker_multiarch.md) - Docker Multi-Arch
- [docs/de/deployment/deployment_raspberry_tuning.md](docs/de/deployment/deployment_raspberry_tuning.md) - RPi
- [docs/de/deployment/deployment_qnap.md](docs/de/deployment/deployment_qnap.md) - QNAP NAS

### 🚀 Zusammenfassung für Copilot

**Alte Welt (❌):**
```
Root mit 40+ Build-Dateien
CMakeLists.txt, CMakePresets.json, 15x Dockerfiles im Root
Pfade überall verteilt, schwer zu navigieren
```

**Neue Welt (✅):**
```
Sauberer Root (nur essenzielle Dateien)
cmake/ - Alle CMake-Dateien + Presets
docker/ - Alle Docker-Dateien
docs/build-guide/ - Neue Dokumentation mit Links
Deployment & Release Strategien in docs/de/
```

**Erwartung an Copilot:**
```
✅ Struktur verstehen
✅ Presets verwenden
✅ Richtige Dokumentation verlinken
✅ Deployment/Release-Pfade kennen
✅ Alle Plattformen (Windows, Linux, Docker, ARM) unterstützen
```
