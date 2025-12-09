# Workflow Testing & Validation Guide

## Kostenfreies Testen der Workflows

### 1. Lokales Testen mit `act` (Empfohlen)

[act](https://github.com/nektos/act) ermöglicht das Ausführen von GitHub Actions lokal in Docker-Containern.

#### Installation
```bash
# Linux/macOS (Homebrew)
brew install act

# Linux (Binary)
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash

# Windows (Scoop)
scoop install act
```

#### Konfiguration
```bash
# 1. Secrets-Datei erstellen (bereits im Repo vorhanden)
cp .github/act-secrets.example .github/act-secrets

# 2. Secrets ausfüllen (optional für die meisten Tests)
# Nur GITHUB_TOKEN wird für viele Tests benötigt
```

#### Workflow-Tests ohne Kosten

**Test 1: CI Workflow (Dry-Run)**
```bash
# Linux-Build ohne Push
act pull_request -W .github/workflows/ci.yml -j build-and-test-linux

# Windows-Build lokal testen (benötigt Windows-Container, optional)
act pull_request -W .github/workflows/ci.yml -j build-and-test-windows --container-architecture linux/amd64
```

**Test 2: Docker Build Workflow (Lokal)**
```bash
# Fast mode (mit pre-built binary)
act workflow_dispatch -W .github/workflows/docker-build.yml \
  -j build-docker-simple \
  --input use_prebuilt=true \
  --input version=1.0.1 \
  --input push=false

# Syntax-Check ohne Ausführung
act -l -W .github/workflows/docker-build.yml
```

**Test 3: Release Workflow (Validation)**
```bash
# Liste alle Jobs ohne Ausführung
act workflow_dispatch -W .github/workflows/release.yml -l

# Test Package-Jobs (ohne Build)
act workflow_dispatch -W .github/workflows/release.yml \
  -j package-linux-x64 \
  --input use_prebuilt=true \
  --input version=1.0.1
```

### 2. GitHub Actions Test-Strategien (Minimal Costs)

#### A. Pull Request Draft Mode
```bash
# Erstelle PR als Draft
gh pr create --draft --title "[TEST] Workflow validation" --body "Testing only"

# Draft PRs lösen Workflows aus, aber kennzeichnen Tests klar
# Schließe PR nach Test: gh pr close <number>
```

#### B. Workflow Syntax Validation
```bash
# Lokale Syntax-Prüfung (kostenlos)
# 1. YAML Linter
yamllint .github/workflows/*.yml

# 2. GitHub Action Linter
# Installiere actionlint: https://github.com/rhysd/actionlint
actionlint .github/workflows/*.yml

# 3. VS Code Extension
# GitHub Actions Extension validiert Syntax in Echtzeit
```

#### C. Conditional Execution (Kosten-Kontrolle)
```yaml
# In Workflows bereits implementiert:
if: github.event.inputs.use_prebuilt == 'true'  # Skip teure Builds

# Manual Dispatch mit use_prebuilt=true nutzen:
# - Spart ~27 Minuten Build-Zeit
# - Kostet nur Sekunden für Packaging
```

### 3. Staging-Strategie (Best Practice)

#### Stufenweises Testen
```bash
# Stufe 1: Syntax & Lint (lokal, kostenlos)
actionlint .github/workflows/*.yml

# Stufe 2: Act Tests (lokal, kostenlos)
act pull_request -W .github/workflows/ci.yml -j build-and-test-linux -n

# Stufe 3: Draft PR (GitHub, minimal)
gh pr create --draft

# Stufe 4: Einzelner Job-Test
# Workflow manuell auslösen mit minimal Job-Set

# Stufe 5: Full Integration (nur bei Bedarf)
```

### 4. Cost-Effective Testing Matrix

| Test-Methode | Kosten | Dauer | Empfehlung |
|--------------|--------|-------|------------|
| `actionlint` | €0 | <1s | ✅ Immer |
| `act` (lokal) | €0 | 1-5min | ✅ Immer |
| Draft PR | €0* | variabel | ✅ Bei Änderungen |
| Manual Dispatch (prebuilt) | ~€0.01 | 30s | ✅ Regression Tests |
| Manual Dispatch (full) | ~€0.50 | 30min | ⚠️ Nur vor Release |
| Tag-triggered Release | ~€1-2 | 45min | ⚠️ Nur für Releases |

*Free tier: 2000 Minuten/Monat für private Repos, unbegrenzt für public repos

## Build-Strategie: Best Practices Review

### ✅ Implementierte Best Practices

#### 1. **Hybride Dependency-Strategie** ✓ (NEU)
```yaml
# System Libraries (apt) + minimal vcpkg
# 6x schneller, 90% weniger Downloads
```
- ✅ Pre-built Libraries von Ubuntu
- ✅ Nur fehlende Pakete via vcpkg
- ✅ Dramatische Zeit- und Kosten-Reduktion

**Verfügbare System-Pakete:**
- RocksDB 8.9.1, Boost 1.83.0, OpenSSL 3.0.13
- spdlog 1.12.0, TBB 2021.11.0, yaml-cpp 0.8.0
- nlohmann-json 3.11.3, zstd 1.5.5, curl 8.5.0

**Minimale vcpkg-Installation (nur 4 Pakete):**
- simdjson, arrow, hnswlib, opentelemetry-cpp

#### 2. **Drei Build-Modi** ✓ (NEU)
```yaml
ci-fast.yml:      ~5 min   (apt + minimal vcpkg)
ci.yml:           ~30 min  (vollständiges vcpkg)
docker-build.yml: ~30 sec  (pre-built binary)
```

#### 3. **Caching-Strategie** ✓
```yaml
# Plattform-spezifische Keys
key: vcpkg-linux-x64-release-${{ hashFiles('vcpkg.json') }}
key: vcpkg-windows-x64-release-${{ hashFiles('vcpkg.json') }}
```
- ✅ Separate Caches pro Plattform
- ✅ Hash-basierte Invalidierung
- ✅ Restore-Keys für Fallbacks

#### 2. **Job-Dependencies & Parallelisierung** ✓
```yaml
needs: [build-binary]
if: always() && (needs.build-binary.result == 'success' || ...)
```
- ✅ Parallele Builds (Linux + Windows)
- ✅ Bedingte Ausführung
- ✅ Fail-Fast bei kritischen Fehlern

#### 3. **Artifact Management** ✓
```yaml
retention-days: 7  # Nicht 90 (Standard)
```
- ✅ Kurze Retention (Kosten-Optimierung)
- ✅ Separate Artifacts pro Plattform
- ✅ Pattern-basiertes Download

#### 4. **Matrix-Free Design** ✓
- ✅ Explizite Jobs statt Matrix (bessere Kontrolle)
- ✅ Ermöglicht plattform-spezifische Optimierungen
- ✅ Klarere Fehlermeldungen

#### 5. **Conditional Builds** ✓
```yaml
if: github.event.inputs.use_prebuilt != 'true'
```
- ✅ Skip teurer Builds bei Pre-built Mode
- ✅ 60x Zeitersparnis
- ✅ Workflow-Input Parameter

### 🔧 Empfohlene Verbesserungen

#### 1. **Timeout Protection**
```yaml
jobs:
  build-linux-x64:
    timeout-minutes: 60  # Verhindert endlose Builds
```

#### 2. **Concurrency Groups**
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true  # Stoppt alte Runs
```

#### 3. **Workflow Approval für Costs**
```yaml
environment:
  name: production
  # Benötigt manuelle Approval vor teuren Deployments
```

## Robustheit-Bewertung

### ✅ Stark
- **Fehlerbehandlung**: Explizite Fehler statt `|| true`
- **Validierung**: Prüft Binaries vor Nutzung
- **Versionierung**: Dynamische Pfade, keine Hardcodes
- **Dokumentation**: Klare Build Summaries

### ⚠️ Verbesserungspotential
- **Timeouts**: Noch nicht implementiert
- **Concurrency**: Keine Duplicate-Prevention
- **Secrets Validation**: Keine Pre-Checks für Docker-Push

### 🎯 Empfohlene Nächste Schritte

1. **Jetzt testen (kostenlos)**:
   ```bash
   actionlint .github/workflows/*.yml
   act pull_request -W .github/workflows/ci.yml -n
   ```

2. **Timeouts hinzufügen**:
   - CI: 30 Minuten
   - Docker Build: 45 Minuten
   - Release: 60 Minuten

3. **Concurrency Groups**:
   - Verhindert parallel laufende Releases
   - Spart Kosten bei schnellen Pushes

4. **Environment Protection**:
   - `production` Environment für Docker-Pushes
   - Manuelle Approval für Tag-Releases

## Monitoring & Alerts

### GitHub Actions Usage
```bash
# Prüfe monatliche Nutzung
gh api /repos/makr-code/ThemisDB/actions/billing/usage

# Monitor Workflow-Runs
gh run list --workflow=ci.yml --limit 10
```

### Best Practice Limits
- **CI (dry-run)**: <5 Minuten pro Run
- **Docker (prebuilt)**: <2 Minuten pro Run
- **Release (prebuilt)**: <10 Minuten total
- **Full Build**: Nur bei Tags (~45 Minuten akzeptabel)

## Fazit

Die aktuelle Build-Strategie ist **robust und best-practice-konform** mit folgenden Stärken:

✅ **Kosten-Optimierung**: 60x schneller mit Pre-built Mode  
✅ **Flexibilität**: Unterstützt beide Build-Modi  
✅ **Fehlerbehandlung**: Explizite Validierung  
✅ **Caching**: Plattform-spezifische Strategien  
✅ **Parallelisierung**: Windows + Linux gleichzeitig  

**Empfohlene Ergänzungen** (nicht kritisch):
- Timeouts für Schutz
- Concurrency Groups für Kosten
- Environment Protection für Releases

Die Workflows können sicher mit `act` lokal getestet werden - **keine GitHub Actions-Kosten erforderlich!**
