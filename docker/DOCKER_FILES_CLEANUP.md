# Docker Files - Bereinigung und Dokumentation
**Datum:** 10. Januar 2026  
**ThemisDB Version:** v1.4.0

---

## 📋 Analyse der existierenden Dockerfiles

### ✅ **BEHALTEN** - Aktiv genutzt (6 Dateien)

#### Core Build-Dateien
1. **Dockerfile.unified** ⭐ **HAUPT-DOCKERFILE**
   - Zweck: Alle 4 Editionen (MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER)
   - Multi-Arch Support (AMD64, ARM64)
   - Multi-Stage Build (builder → runtime → debug)
   - Status: **Aktiv genutzt, Hauptdatei**

2. **Dockerfile.dev** ⚡ **DEVELOPMENT**
   - Zweck: Schnelle lokale Entwicklung mit pre-built Binaries
   - Kopiert aus `build-wsl/` oder `build-msvc/`
   - Build-Zeit: 10-15 Sekunden
   - Status: **Aktiv, für lokale Entwicklung**

#### Base Image Builds
3. **Dockerfile.vcpkg-base** 🏗️ **BASE IMAGE**
   - Zweck: Pre-built vcpkg Foundation
   - Ubuntu 26.04 + bootstrapped vcpkg
   - Wird von anderen Images verwendet
   - Status: **Aktiv, für Base Image Pipeline**

4. **Dockerfile.vcpkg-deps** 🏗️ **BASE IMAGE**
   - Zweck: Pre-compiled Dependencies pro Edition
   - 4 Targets: minimal, community, enterprise, hyperscaler
   - Status: **Aktiv, für Base Image Pipeline**

5. **Dockerfile.llama-base** 🏗️ **BASE IMAGE**
   - Zweck: Pre-built llama.cpp für LLM-Support
   - Nutzt lokales `llama.cpp/` Verzeichnis
   - Status: **Aktiv, für LLM-enabled Builds**

#### Legacy/Spezial-Zwecke (behalten, dokumentiert)
6. **Dockerfile** 📦 **LEGACY MAIN**
   - Zweck: Original Haupt-Dockerfile (368 Zeilen)
   - vcpkg-Integration, Edition-Support
   - Status: **Behalten als Fallback/Referenz**
   - Hinweis: Kann durch `Dockerfile.unified` ersetzt werden, aber erst nach Tests

---

### 🗑️ **LÖSCHEN** - Ersetzt durch Dockerfile.unified (19 Dateien)

#### Edition-spezifische Builds (ersetzt durch --build-arg)
7. **Dockerfile.hyperscaler** ❌ → `Dockerfile.unified --build-arg THEMIS_EDITION=HYPERSCALER`
8. **Dockerfile.hyperscaler-simple** ❌ → `Dockerfile.unified`
9. **Dockerfile.hyperscaler-runtime** ❌ → `Dockerfile.unified`
10. **Dockerfile.minimal** ❌ → `Dockerfile.unified --build-arg THEMIS_EDITION=MINIMAL`
11. **Dockerfile.minimal-fast** ❌ → `Dockerfile.dev`
12. **Dockerfile.themis-server** ❌ → `Dockerfile.unified`

#### Quick/Fast Builds (ersetzt durch Dockerfile.dev)
13. **Dockerfile.fast** ❌ → `Dockerfile.dev`
14. **Dockerfile.prebuilt** ❌ → `Dockerfile.dev`
15. **Dockerfile.prebuild** ❌ → `Dockerfile.dev`
16. **Dockerfile.quick** ❌ → `Dockerfile.dev`
17. **Dockerfile.quick-linux** ❌ → `Dockerfile.dev`
18. **Dockerfile.optimized-local** ❌ → `Dockerfile.dev`

#### Build-in-Docker / Runtime-Only
19. **Dockerfile.build-in-docker** ❌ → `Dockerfile.unified`
20. **Dockerfile.runtime** ❌ → `Dockerfile.unified` (hat runtime stage)
21. **Dockerfile.simple** ❌ → `Dockerfile.unified`
22. **Dockerfile.docker-deploy** ❌ → `Dockerfile.unified`

#### Release/Package Builds
23. **Dockerfile.release** ❌ → `Dockerfile.unified`
24. **Dockerfile.release-packages** ❌ → `Dockerfile.unified`

#### Benchmark/Test-spezifisch
25. **Dockerfile.benchmark** ⚠️ → Zu prüfen (kann spezielle Benchmark-Tools enthalten)

---

### ⚠️ **BEHALTEN (Spezial-Zwecke)** - Nicht ersetzbar (4 Dateien)

26. **Dockerfile.qnap** 🔧 **QNAP NAS**
   - Zweck: QNAP NAS Container Station
   - Spezielle Konfiguration für QNAP Hardware
   - Status: **Behalten, spezielle Platform**

27. **Dockerfile.qnap.build** 🔧 **QNAP NAS**
   - Zweck: QNAP Build-Stage
   - Status: **Behalten**

28. **Dockerfile.qnap.runtime** 🔧 **QNAP NAS**
   - Zweck: QNAP Runtime-Stage
   - Status: **Behalten**

29. **Dockerfile.wire-protocol** 🔌 **WIRE PROTOCOL**
   - Zweck: Spezieller Build mit Wire Protocol Focus
   - Status: **Behalten, spezifisches Feature-Testing**

30. **Dockerfile.llm-raid-tests** 🧪 **TESTING**
   - Zweck: LLM + RAID Integration Tests
   - Status: **Behalten, Test-Infrastruktur**

---

## 📊 Zusammenfassung

| Kategorie | Anzahl | Aktion |
|-----------|--------|--------|
| ✅ **Aktiv (Core)** | 6 | Behalten |
| 🗑️ **Zu löschen** | 19 | Durch unified/dev ersetzt |
| ⚠️ **Spezial-Zweck** | 5 | Behalten (QNAP, Tests) |
| **GESAMT** | 30 | → **11 Dateien bleiben** |

---

## 🔄 Migration zu neuen Dockerfiles

### Vorher (alt)
```bash
# Hyperscaler
docker build -f docker/Dockerfile.hyperscaler -t themisdb:hyperscaler .

# Minimal
docker build -f docker/Dockerfile.minimal -t themisdb:minimal .

# Fast local build
docker build -f docker/Dockerfile.fast -t themisdb:fast .
```

### Nachher (neu)
```bash
# Hyperscaler
docker build -f docker/Dockerfile.unified --build-arg THEMIS_EDITION=HYPERSCALER --build-arg ENABLE_LLM=ON -t themisdb:hyperscaler .

# Minimal
docker build -f docker/Dockerfile.unified --build-arg THEMIS_EDITION=MINIMAL -t themisdb:minimal .

# Fast local build
docker build -f docker/Dockerfile.dev -t themisdb:dev .

# ODER einfach:
./docker/build-all-editions.sh 1.4.0 themisdb/themisdb linux/amd64
```

---

## 📂 Finale Dateistruktur (nach Bereinigung)

```
docker/
├── # ===== CORE BUILD FILES (6) =====
├── Dockerfile.unified          ⭐ Main (alle Editionen)
├── Dockerfile.dev              ⚡ Fast local dev
├── Dockerfile.vcpkg-base       🏗️ Base: vcpkg
├── Dockerfile.vcpkg-deps       🏗️ Base: dependencies
├── Dockerfile.llama-base       🏗️ Base: llama.cpp
├── Dockerfile                  📦 Legacy (Referenz)
│
├── # ===== SPEZIAL-ZWECKE (5) =====
├── Dockerfile.qnap             🔧 QNAP NAS
├── Dockerfile.qnap.build       🔧 QNAP Build
├── Dockerfile.qnap.runtime     🔧 QNAP Runtime
├── Dockerfile.wire-protocol    🔌 Wire Protocol Tests
├── Dockerfile.llm-raid-tests   🧪 LLM+RAID Tests
│
├── # ===== VCPKG MANIFESTS (5) =====
├── vcpkg-minimal.json
├── vcpkg-community.json
├── vcpkg-enterprise.json
├── vcpkg-hyperscaler.json
├── vcpkg.docker.json
├── vcpkg.qnap.json
│
├── # ===== BUILD SCRIPTS (3) =====
├── build-all-editions.sh       🔧 Build alle Editionen
├── build-base-images.sh        🔧 Build Base Images
├── build-hyperscaler.sh        🔧 Legacy Hyperscaler
│
├── # ===== DOKUMENTATION (4) =====
├── BUILD_STAGES_GUIDE.md
├── DOCKER_BUILD_OPTIMIZATION_ANALYSIS.md
├── DOCKER_BUILD_STRATEGY_QUICKREF.md
├── DOCKER_BUILD_CORRECTIONS.md
├── DOCKER_FILES_CLEANUP.md     📝 Diese Datei
│
└── # ===== COMPOSE & CONFIG =====
    ├── docker-compose*.yml (6 Dateien)
    ├── entrypoint.sh
    └── compose/, grafana/, prometheus/
```

**Dateien-Reduktion:** 30 → 11 Dockerfiles (**63% weniger**)

---

## ⚡ Zu löschende Dateien (19)

```bash
# Edition-spezifisch
rm docker/Dockerfile.hyperscaler
rm docker/Dockerfile.hyperscaler-simple
rm docker/Dockerfile.hyperscaler-runtime
rm docker/Dockerfile.minimal
rm docker/Dockerfile.minimal-fast
rm docker/Dockerfile.themis-server

# Fast/Quick Builds
rm docker/Dockerfile.fast
rm docker/Dockerfile.prebuilt
rm docker/Dockerfile.prebuild
rm docker/Dockerfile.quick
rm docker/Dockerfile.quick-linux
rm docker/Dockerfile.optimized-local

# Build/Runtime
rm docker/Dockerfile.build-in-docker
rm docker/Dockerfile.runtime
rm docker/Dockerfile.simple
rm docker/Dockerfile.docker-deploy

# Release
rm docker/Dockerfile.release
rm docker/Dockerfile.release-packages

# Benchmark (nach Prüfung)
rm docker/Dockerfile.benchmark  # ⚠️ Erst prüfen!
```

---

## ✅ Verifikation vor Löschung

### Schritt 1: Teste neue Dockerfiles
```bash
# Test MINIMAL
docker build -f docker/Dockerfile.unified --build-arg THEMIS_EDITION=MINIMAL -t test:minimal .

# Test COMMUNITY
docker build -f docker/Dockerfile.unified --build-arg THEMIS_EDITION=COMMUNITY -t test:community .

# Test HYPERSCALER
docker build -f docker/Dockerfile.unified --build-arg THEMIS_EDITION=HYPERSCALER --build-arg ENABLE_LLM=ON -t test:hyperscaler .

# Test DEV (fast local)
docker build -f docker/Dockerfile.dev -t test:dev .
```

### Schritt 2: Prüfe QNAP & Wire Protocol
```bash
# QNAP sollte unverändert funktionieren
docker build -f docker/Dockerfile.qnap -t test:qnap .

# Wire Protocol Tests
docker build -f docker/Dockerfile.wire-protocol -t test:wire .
```

### Schritt 3: Lösche alte Dockerfiles
```bash
./docker/cleanup-old-dockerfiles.sh  # Script erstellen
```

---

## 🎯 Vorteile nach Bereinigung

### Vorher
- 30 Dockerfiles
- Inkonsistente Struktur
- Duplicate Logik
- Schwer wartbar
- Verwirrend für neue Entwickler

### Nachher
- 11 Dockerfiles (core: 6, special: 5)
- Klare Struktur: unified, dev, base images
- Single source of truth
- Einfach wartbar
- Dokumentiert und konsistent

---

## 📝 Nächste Schritte

1. ✅ **Tests durchführen** (siehe Schritt 1-2 oben)
2. ⚠️ **Dockerfile.benchmark prüfen** (hat es spezielle Tools?)
3. 🗑️ **Alte Dockerfiles löschen** (Script ausführen)
4. 📝 **README.md aktualisieren** (neue Build-Commands)
5. 🔄 **CI/CD aktualisieren** (auf Dockerfile.unified umstellen)
6. 📢 **Team informieren** über neue Build-Strategie

---

**Status:** Bereit für Bereinigung nach Tests ✅
