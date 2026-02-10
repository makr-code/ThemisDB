# ThemisDB - Complete Automated CI/CD Strategy

> **⚠️ HISTORICAL DOCUMENT**  
> This document describes the CI/CD strategy from January 2026. The workflows described here have been **superseded by the consolidated CI/CD architecture** implemented in February 2026.  
> **For current CI/CD documentation, see:**
> - [docs/ci-cd/ci-architecture.md](ci-cd/ci-architecture.md) - Current CI/CD architecture
> - [.github/workflows/README.md](../.github/workflows/README.md) - Active workflows documentation
> - [docs/ci-cd/consolidation-plan.md](ci-cd/consolidation-plan.md) - Consolidation details

## Übersicht der Workflow-Strategie

Dieses Dokument beschreibt die vollständige, harmonisierte CI/CD-Strategie für ThemisDB mit allen vorhandenen und neuen Workflows.

## Workflow-Architektur

### 1. Entwicklungs-Workflows (Development)

#### **feature-ci.yml** - Feature/Bugfix Validation
- **Trigger**: PR zu `develop` von `feature/*` oder `bugfix/*`
- **Zweck**: Validierung von Feature- und Bugfix-Branches
- **Dauer**: ~30-45 Minuten
- **Plattformen**: Ubuntu (primary), optional Windows/macOS
- **Status**: ✅ Aktiv (bestehend)

#### **ci-develop.yml** - Development Pipeline (NEU)
- **Trigger**: Push/PR zu `develop`
- **Zweck**: Schnelles Feedback für Entwickler auf develop
- **Dauer**: ~15-30 Minuten
- **Plattformen**: Ubuntu (required), Windows, macOS (optional)
- **Status**: ✅ Neu erstellt
- **Vorteil**: Schneller als feature-ci.yml durch System-Libraries

#### **develop-ci.yml** - Develop Integration (BESTEHEND)
- **Trigger**: Push/PR zu `develop`
- **Zweck**: Integration Testing auf develop
- **Dauer**: ~30-45 Minuten
- **Status**: ✅ Aktiv (bestehend)
- **Empfehlung**: Kann mit ci-develop.yml konsolidiert werden

### 2. Release-Workflows (Production)

#### **release-ci.yml** - Release Preparation (BESTEHEND)
- **Trigger**: Push zu `release/*`, PR zu `main`
- **Zweck**: Release-Vorbereitung und Validierung
- **Dauer**: ~45-60 Minuten
- **Status**: ✅ Aktiv (bestehend)

#### **build-and-test.yml** - Main Branch Protection (NEU)
- **Trigger**: PR zu `main` (nur von release/* oder hotfix/*)
- **Zweck**: Umfassende Tests vor Merge zu main
- **Dauer**: ~45-60 Minuten
- **Plattformen**: Ubuntu, Windows, macOS (alle required)
- **Status**: ✅ Neu erstellt
- **Besonderheit**: Alle Plattformen müssen erfolgreich sein

#### **main-ci.yml** - Main Branch Deployment (BESTEHEND)
- **Trigger**: Tag push `v*`
- **Zweck**: Verifikation und Deployment nach Merge
- **Status**: ✅ Aktiv (bestehend)

#### **release.yml** - Automated Release (NEU)
- **Trigger**: Tag push `v*`
- **Zweck**: Vollautomatische Release-Erstellung
- **Dauer**: ~60-90 Minuten
- **Features**: 
  - Multi-Platform Builds (Ubuntu, Windows, macOS)
  - Automatische GitHub Release Creation
  - Artifact Upload (.tar.gz, .deb, .zip)
  - Release Notes Generation
- **Status**: ✅ Neu erstellt
- **Vorteil**: Vollautomatisch, keine manuelle Schritte

### 3. Hotfix-Workflows (Emergency)

#### **hotfix-ci.yml** - Hotfix Validation (BESTEHEND)
- **Trigger**: PR zu `main` von `hotfix/*`
- **Zweck**: Fast-Track für kritische Fixes
- **Dauer**: ~20-30 Minuten
- **Status**: ✅ Aktiv (bestehend)

### 4. Spezial-Workflows (Specialized)

#### **security-scan.yml** - Security Scanning
- **Trigger**: 
  - Push zu main/develop
  - PR zu main/develop
  - Schedule: Sonntags 02:00 UTC
- **Zweck**: SAST, Gitleaks, Dependency Scanning
- **Status**: ✅ Aktiv

#### **fuzzing.yml** - Fuzzing Tests
- **Trigger**: Schedule: Sonntags 00:00 UTC
- **Zweck**: AFL++ Fuzzing
- **Status**: ✅ Aktiv

#### **sbom.yml** - Software Bill of Materials
- **Trigger**: Tag push `v*`
- **Zweck**: SBOM-Generierung für Releases
- **Status**: ✅ Aktiv

### 5. Dokumentations-Workflows

#### **docs.yml** - Documentation Build
- **Trigger**: Tag push
- **Zweck**: MkDocs Documentation Build & Deploy
- **Status**: ✅ Aktiv

#### **docs-compendium.yml** - Compendium Build
- **Trigger**: Tag push
- **Zweck**: PDF Compendium Generation
- **Status**: ✅ Aktiv

#### **wiki-sync.yml** - Wiki Synchronization
- **Trigger**: Tag push
- **Zweck**: Sync zu GitHub Wiki
- **Status**: ✅ Aktiv

### 6. SDK Testing Workflows

#### **python-sdk-test.yml** - Python SDK
- **Trigger**: Workflow dispatch (Dry-Run)
- **Status**: ✅ Aktiv (Dry-Run Mode)

#### **java-sdk-test.yml** - Java SDK
- **Trigger**: Workflow dispatch (Dry-Run)
- **Status**: ✅ Aktiv (Dry-Run Mode)

#### **csharp-sdk-test.yml** - C# SDK
- **Trigger**: Workflow dispatch (Dry-Run)
- **Status**: ✅ Aktiv (Dry-Run Mode)

### 7. Deployment Workflows

#### **helm-chart-test.yml** - Helm Chart
- **Trigger**: Workflow dispatch (Dry-Run)
- **Status**: ✅ Aktiv (Dry-Run Mode)

#### **wordpress-theme-deploy.yml** - WordPress Theme
- **Trigger**: Tag push, Push zu wordpress-theme branches
- **Status**: ✅ Aktiv

### 8. Utility Workflows

#### **ci.yml** - General CI
- **Trigger**: Workflow dispatch
- **Zweck**: Allgemeiner CI-Check (supplementär)
- **Status**: ✅ Aktiv

#### **ci-local-test.yml** - Local Testing
- **Trigger**: Workflow dispatch
- **Zweck**: System Libraries Only (für lokales Testing)
- **Status**: ✅ Aktiv

---

## Harmonisierte Workflow-Strategie

### Phase 1: Feature-Entwicklung

```
Developer creates feature branch
         ↓
    feature/my-feature
         ↓
   Push to GitHub
         ↓
Create PR to develop
         ↓
[feature-ci.yml] ODER [ci-develop.yml]
         ↓
  Build & Test (Ubuntu + optional Windows/macOS)
         ↓
    Code Review
         ↓
  Merge to develop
         ↓
[develop-ci.yml] runs on develop
```

**Optimierung**: feature-ci.yml und ci-develop.yml haben ähnliche Aufgaben
- **Empfehlung**: ci-develop.yml als primär nutzen (schneller)
- feature-ci.yml für spezielle Feature-Validierung behalten

### Phase 2: Release-Vorbereitung

```
Release Manager creates release branch
         ↓
  release/v1.5.0
         ↓
Update VERSION, CHANGELOG.md
         ↓
[release-ci.yml] runs on push
         ↓
Create PR to main
         ↓
[build-and-test.yml] runs (NEU)
  - ALL platforms required
  - Security scan
  - Full test suite
         ↓
   Code Review + Approval
         ↓
  Merge to main
```

**Neue Integration**: build-and-test.yml ergänzt release-ci.yml
- release-ci.yml: Validierung auf release branch
- build-and-test.yml: Final checks vor main merge

### Phase 3: Release-Veröffentlichung

```
After merge to main
         ↓
Tag creation: v1.5.0
         ↓
[release.yml] triggers (NEU)
  ├─ Build Ubuntu (.tar.gz, .deb)
  ├─ Build Windows (.zip)
  └─ Build macOS (.tar.gz)
         ↓
GitHub Release created automatically
         ↓
[main-ci.yml] triggers (BESTEHEND)
  - Verification
  - Additional deployments
         ↓
[sbom.yml] triggers
  - Generate SBOM
         ↓
[docs.yml] triggers
  - Deploy documentation
         ↓
[docs-compendium.yml] triggers
  - Generate PDF
         ↓
[wiki-sync.yml] triggers
  - Sync to wiki
```

**Neue Automatisierung**: release.yml übernimmt vollständige Release-Erstellung
- Parallele Builds für alle Plattformen
- Automatische Artifact-Upload
- Keine manuellen Schritte mehr nötig

### Phase 4: Hotfix-Prozess

```
Critical Issue discovered
         ↓
hotfix/v1.4.1 from main
         ↓
Fix + Update VERSION
         ↓
Create PR to main
         ↓
[hotfix-ci.yml] runs (BESTEHEND)
  - Fast-track validation
         ↓
[build-and-test.yml] runs (NEU)
  - Full platform checks
         ↓
Fast-track approval
         ↓
Merge to main
         ↓
Tag v1.4.1
         ↓
[release.yml] creates hotfix release
         ↓
Merge back to develop
```

---

## Workflow-Trigger-Matrix

| Workflow | develop push | develop PR | main PR | main push | Tag v* | Schedule | Manual |
|----------|-------------|------------|---------|-----------|--------|----------|--------|
| feature-ci.yml | - | ✅ | - | - | - | - | - |
| ci-develop.yml | ✅ | ✅ | - | - | - | - | - |
| develop-ci.yml | ✅ | ✅ | - | - | - | - | - |
| release-ci.yml | - | - | ✅ | release/* | - | - | ✅ |
| build-and-test.yml | - | - | ✅ | - | - | - | - |
| hotfix-ci.yml | - | - | ✅ | hotfix/* | - | - | - |
| main-ci.yml | - | - | - | - | ✅ | - | ✅ |
| release.yml | - | - | - | - | ✅ | - | - |
| security-scan.yml | ✅ | ✅ | - | ✅ | - | ✅ (So) | - |
| fuzzing.yml | - | - | - | - | - | ✅ (So) | - |
| sbom.yml | - | - | - | - | ✅ | - | - |
| docs.yml | - | - | - | - | ✅ | - | - |
| docs-compendium.yml | - | - | - | - | ✅ | - | - |
| wiki-sync.yml | - | - | - | - | ✅ | - | - |
| SDK Tests | - | - | - | - | - | - | ✅ |
| ci.yml | - | - | - | - | - | - | ✅ |

---

## Konsolidierungs-Empfehlungen

### 1. Develop Branch Workflows (EMPFEHLUNG)

**Aktuell**: 3 Workflows für develop
- feature-ci.yml
- ci-develop.yml (NEU)
- develop-ci.yml

**Empfehlung**: Konsolidierung zu 2 Workflows
1. **ci-develop.yml** - Primär (schnell, system libs)
   - Für Push/PR zu develop
   - Ubuntu required, Windows/macOS optional
   
2. **feature-ci.yml** - Spezialisiert (falls zusätzliche Feature-Checks nötig)
   - Optional aktiviert nur für bestimmte Feature-Types
   - Oder als Backup

**Migration**: develop-ci.yml kann deaktiviert oder mit ci-develop.yml merged werden

### 2. Main/Release Workflows (OPTIMAL)

**Aktuell**: Gut getrennt
- release-ci.yml: Release branch validation
- build-and-test.yml: Main branch protection (NEU)
- main-ci.yml: Post-merge verification
- release.yml: Automated release (NEU)

**Status**: ✅ Optimale Trennung, keine Änderung nötig

### 3. Tag-Triggered Workflows (PARALLELISIERUNG)

**Aktuell**: Mehrere Workflows bei Tag v*
- main-ci.yml
- release.yml (NEU)
- sbom.yml
- docs.yml
- docs-compendium.yml
- wiki-sync.yml

**Status**: ✅ Läuft parallel, gut organisiert
**Optimierung**: Alle sind unabhängig und können parallel laufen

---

## Branch Protection Konfiguration

### develop Branch
**Required Status Checks**:
- ✅ `Build & Test - Ubuntu` (von ci-develop.yml)
- ✅ `Validate Changes` (von ci-develop.yml)
- ⚠️ `Feature/Bugfix CI` (optional, von feature-ci.yml)

**Einstellungen**:
- Require 1 approval
- Require branches to be up to date
- Allow force pushes: NO

### main Branch  
**Required Status Checks**:
- ✅ `Full Build & Test - Ubuntu` (von build-and-test.yml)
- ✅ `Full Build & Test - Windows` (von build-and-test.yml)
- ✅ `Full Build & Test - macOS` (von build-and-test.yml)
- ✅ `Security Scan` (von build-and-test.yml)
- ✅ `Validate Release` (von release-ci.yml)

**Einstellungen**:
- Require 1+ approvals
- Require branches to be up to date
- Restrict to release/* and hotfix/* branches
- Allow force pushes: NO
- Restrict who can push: Maintainers only

---

## Vollständiger Release-Ablauf (Automatisiert)

### Standard Release (z.B. v1.5.0)

```bash
# 1. Release Branch erstellen
git checkout develop
git pull origin develop
git checkout -b release/v1.5.0

# 2. Version vorbereiten
echo "1.5.0" > VERSION
# CHANGELOG.md aktualisieren mit Release Notes

# 3. Commit & Push
git add VERSION CHANGELOG.md
git commit -m "chore(release): Prepare version 1.5.0"
git push origin release/v1.5.0
# → Triggers: release-ci.yml (Validierung)

# 4. PR zu main erstellen
# GitHub UI: Create Pull Request
# Base: main, Compare: release/v1.5.0
# → Triggers: build-and-test.yml (ALLE Plattformen required)

# 5. Warten auf CI & Review
# - Alle Platform-Builds müssen erfolgreich sein
# - Security Scan muss bestehen
# - Mindestens 1 Approval erforderlich

# 6. Merge PR
# Nach Approval über GitHub UI mergen

# 7. Tag erstellen & pushen
git checkout main
git pull origin main
git tag -a v1.5.0 -m "Release v1.5.0

- Feature A: Beschreibung
- Feature B: Beschreibung
- Bug fix C: Beschreibung

See CHANGELOG.md for complete details."

git push origin v1.5.0

# → AUTOMATISCH AUSGEFÜHRT:
# ✅ release.yml: Build Binaries (Ubuntu, Windows, macOS)
# ✅ release.yml: Create GitHub Release
# ✅ release.yml: Upload Artifacts
# ✅ main-ci.yml: Post-merge verification
# ✅ sbom.yml: Generate SBOM
# ✅ docs.yml: Deploy documentation
# ✅ docs-compendium.yml: Generate PDF
# ✅ wiki-sync.yml: Sync to wiki

# 8. Zurück zu develop mergen
git checkout develop
git pull origin develop
git merge main -m "chore: Merge release v1.5.0 back to develop"
git push origin develop
```

**Ergebnis**: Vollständig automatisierter Release in ~60-90 Minuten

### Pre-Release (z.B. v1.5.0-beta.1)

```bash
# Gleicher Prozess wie Standard Release, aber:
echo "1.5.0-beta.1" > VERSION
git tag -a v1.5.0-beta.1 -m "Beta Release v1.5.0-beta.1"
git push origin v1.5.0-beta.1

# → release.yml erkennt automatisch Pre-Release (enthält "-")
# → GitHub Release wird als "Pre-release" markiert
```

### Hotfix (z.B. v1.4.1)

```bash
# 1. Hotfix Branch von main
git checkout main
git pull origin main
git checkout -b hotfix/v1.4.1

# 2. Fix implementieren
# ... code changes ...

# 3. Version aktualisieren
echo "1.4.1" > VERSION
# CHANGELOG.md mit Hotfix-Details aktualisieren

# 4. Commit & Push
git add .
git commit -m "fix(security): Critical authentication vulnerability"
git push origin hotfix/v1.4.1

# 5. PR zu main (Fast-Track)
# → Triggers: hotfix-ci.yml (schnelle Validierung)
# → Triggers: build-and-test.yml (volle Checks)

# 6. Fast-Track Approval & Merge
# Höhere Priorität bei kritischen Fixes

# 7. Tag & automatischer Release
git checkout main
git pull origin main
git tag -a v1.4.1 -m "Hotfix v1.4.1: Security patch"
git push origin v1.4.1
# → release.yml erstellt Hotfix-Release automatisch

# 8. Zurück zu develop
git checkout develop
git merge main -m "chore: Merge hotfix v1.4.1 to develop"
git push origin develop
```

---

## SDK & Deployment Aktivierung

### SDK Workflows (Derzeit Dry-Run)

Um SDK-Workflows für Production zu aktivieren:

#### Python SDK
```yaml
# In python-sdk-test.yml aktivieren:
- name: Publish to PyPI
  env:
    TWINE_USERNAME: __token__
    TWINE_PASSWORD: ${{ secrets.PYPI_API_TOKEN }}
  run: twine upload dist/*
```

#### Java SDK
```yaml
# In java-sdk-test.yml aktivieren:
- name: Deploy to Maven Central
  env:
    MAVEN_USERNAME: ${{ secrets.MAVEN_USERNAME }}
    MAVEN_PASSWORD: ${{ secrets.MAVEN_PASSWORD }}
  run: mvn deploy -DskipTests -B
```

#### C# SDK
```yaml
# In csharp-sdk-test.yml aktivieren:
- name: Push to NuGet
  run: dotnet nuget push **/*.nupkg --api-key ${{ secrets.NUGET_API_KEY }}
```

#### Helm Chart
```yaml
# In helm-chart-test.yml aktivieren:
- name: Package and Push Helm chart
  run: |
    helm package helm/themisdb
    helm push themisdb-*.tgz oci://ghcr.io/${{ github.repository }}/charts
```

### Trigger-Aktivierung

SDK-Workflows können aktiviert werden durch:

```yaml
on:
  push:
    tags:
      - 'v*'
    paths:
      - 'clients/python/**'  # Python SDK
      - 'clients/java/**'    # Java SDK
      - 'clients/csharp/**'  # C# SDK
      - 'helm/**'            # Helm Chart
```

---

## Monitoring & Observability

### Workflow-Status überwachen

**GitHub Actions Dashboard**:
- https://github.com/makr-code/ThemisDB/actions

**Per Branch**:
- develop: ci-develop.yml + security-scan.yml
- main: build-and-test.yml + main-ci.yml
- release/*: release-ci.yml
- hotfix/*: hotfix-ci.yml

**Per Tag**:
- v*: release.yml + sbom.yml + docs.yml + wiki-sync.yml

### Fehlerbehandlung

**Bei Feature-CI Failure**:
1. Check Logs in GitHub Actions
2. Fix lokal
3. Push zu feature branch
4. CI läuft automatisch erneut

**Bei Release-CI Failure**:
1. Fix auf release branch
2. Push updates
3. PR CI läuft erneut
4. Nach Success → Merge

**Bei Release-Workflow Failure**:
1. Tag löschen: `git push --delete origin v1.5.0`
2. Fix das Problem
3. Tag neu erstellen und pushen
4. release.yml läuft erneut

---

## Zusammenfassung der Verbesserungen

### Neue Workflows
1. ✅ **ci-develop.yml**: Schnelleres Feedback für Entwickler
2. ✅ **build-and-test.yml**: Strikte main-Branch Protection
3. ✅ **release.yml**: Vollautomatische Releases

### Automatisierung
- ✅ Multi-Platform Builds automatisiert
- ✅ Release-Erstellung komplett automatisch
- ✅ Artifact-Upload automatisch
- ✅ Release Notes Generation
- ✅ Pre-Release Erkennung

### Qualitätssicherung
- ✅ Alle Plattformen vor main-Merge required
- ✅ Security Scanning integriert
- ✅ Version-Validierung
- ✅ CHANGELOG-Check

### Dokumentation
- ✅ Vollständige CONTRIBUTING.md
- ✅ PR Template
- ✅ Workflow README
- ✅ Version Management (version.h)

---

## Nächste Schritte

1. **Branch Protection aktivieren** (GitHub Settings)
2. **Secrets konfigurieren** (für SDK Deployments)
3. **Test-Release durchführen** (z.B. v1.4.1-test)
4. **Team schulen** (neue Release-Prozesse)
5. **Monitoring einrichten** (Slack/Discord Notifications optional)
6. **SDK Workflows aktivieren** (wenn bereit für Production)

---

**Status**: ✅ Vollständig harmonisierte CI/CD-Strategie implementiert
**Version**: 1.0.0
**Letzte Aktualisierung**: 2026-01-09
