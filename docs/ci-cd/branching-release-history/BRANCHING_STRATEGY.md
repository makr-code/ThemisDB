# Git Branching Strategy für ThemisDB

## Übersicht

ThemisDB verwendet eine modifizierte **Git Flow** Strategie, bei der der `main` Branch immer produktionsreif ist und als Release-Branch fungiert. Die gesamte Entwicklungsarbeit erfolgt über den `develop` Branch.

## Branch-Struktur

### 🎯 Hauptbranches

#### `main` Branch
- **Zweck**: Production-ready Release Branch
- **Schutz**: ✅ Vollständig geschützt
- **Merges**: Nur von `release/*` und `hotfix/*` Branches
- **Status**: Jeder Commit repräsentiert eine produktionsreife Version
- **Tags**: Alle Release-Tags (z.B. `v1.4.0`) werden hier erstellt
- **CI/CD**: Automatisches Deployment zu Production

**Regeln:**
- ❌ Direkte Commits verboten
- ❌ Keine Feature-Branches direkt mergen
- ✅ Nur über Pull Requests mit Code Review
- ✅ Alle CI/CD Checks müssen bestehen
- ✅ Mindestens 1 Maintainer-Approval erforderlich

#### `develop` Branch
- **Zweck**: Integration Branch für laufende Entwicklung
- **Schutz**: ✅ Geschützt
- **Merges**: Von `feature/*`, `bugfix/*`, und `release/*` (nach Release)
- **Status**: Enthält die neuesten abgeschlossenen Features
- **CI/CD**: Automatische Tests und Validierung

**Regeln:**
- ❌ Direkte Commits vermeiden (Ausnahme: Dokumentation)
- ✅ Feature-Branches werden hier gemerged
- ✅ Pull Requests mit Code Review
- ✅ CI/CD Checks müssen bestehen

### 🚀 Supporting Branches

#### `feature/*` Branches
- **Zweck**: Entwicklung neuer Features
- **Basis**: `develop`
- **Merge Ziel**: `develop`
- **Lebensdauer**: Bis Feature abgeschlossen
- **Naming**: `feature/<issue-nr>-<beschreibung>` oder `feature/<beschreibung>`

**Beispiele:**
```bash
feature/123-vector-search-optimization
feature/llm-streaming-support
feature/postgres-wire-protocol
```

**Workflow:**
```bash
# Feature Branch erstellen
git checkout develop
git pull origin develop
git checkout -b feature/123-vector-search

# Feature entwickeln
git add .
git commit -m "feat(search): Implement vector search optimization"

# Feature fertigstellen
git push origin feature/123-vector-search
# Erstelle Pull Request zu develop
```

#### `bugfix/*` Branches
- **Zweck**: Bug-Fixes für develop Branch
- **Basis**: `develop`
- **Merge Ziel**: `develop`
- **Lebensdauer**: Bis Bug behoben
- **Naming**: `bugfix/<issue-nr>-<beschreibung>` oder `bugfix/<beschreibung>`

**Beispiele:**
```bash
bugfix/456-connection-pool-leak
bugfix/query-timeout-handling
```

#### `hotfix/*` Branches
- **Zweck**: Kritische Bugfixes für Production
- **Basis**: `main`
- **Merge Ziel**: `main` UND `develop`
- **Lebensdauer**: Sofort nach Fix
- **Naming**: `hotfix/<version>-<beschreibung>`

**Beispiele:**
```bash
hotfix/1.3.4-security-vulnerability
hotfix/1.3.4-critical-crash
```

**Workflow:**
```bash
# Hotfix Branch erstellen
git checkout main
git pull origin main
git checkout -b hotfix/1.3.4-security-fix

# Fix implementieren
git add .
git commit -m "fix(security): Patch critical vulnerability"

# Version bumpen
echo "1.3.4" > VERSION
git add VERSION
git commit -m "chore: Bump version to 1.3.4"

# Merge zu main
git checkout main
git merge --no-ff hotfix/1.3.4-security-fix
git tag -a v1.3.4 -m "Release v1.3.4 - Security Fix"
git push origin main --tags

# Merge zurück zu develop
git checkout develop
git merge --no-ff hotfix/1.3.4-security-fix
git push origin develop

# Hotfix Branch löschen
git branch -d hotfix/1.3.4-security-fix
git push origin --delete hotfix/1.3.4-security-fix
```

#### `release/*` Branches
- **Zweck**: Release-Vorbereitung und Stabilisierung
- **Basis**: `develop`
- **Merge Ziel**: `main` UND `develop` (nach Release)
- **Lebensdauer**: Bis Release abgeschlossen
- **Naming**: `release/<version>`

**Beispiele:**
```bash
release/1.4.0
release/2.0.0-beta.1
```

**Workflow:**
```bash
# Release Branch erstellen
git checkout develop
git pull origin develop
git checkout -b release/1.4.0

# Version und Dokumentation vorbereiten
echo "1.4.0" > VERSION
./.github/workflows/04-release_create-release-archive.yml 1.4.0
git add VERSION CHANGELOG.md
git commit -m "chore: Prepare release v1.4.0"

# Bugfixes während Release-Phase
git commit -m "fix(docs): Update release notes"

# Release fertigstellen
git checkout main
git merge --no-ff release/1.4.0
git tag -a v1.4.0 -m "Release v1.4.0"
git push origin main --tags

# Merge zurück zu develop
git checkout develop
git merge --no-ff release/1.4.0
git push origin develop

# Release Branch löschen
git branch -d release/1.4.0
git push origin --delete release/1.4.0
```

## Merge-Strategie

> [!IMPORTANT]
> **ThemisDB verwendet unterschiedliche Merge-Methoden je nach Branch-Typ:**

| Branch-Typ | Merge-Methode | Begründung |
|-----------|---------------|------------|
| **feature/** → develop | **Squash and merge** ✅ | Hält develop-Historie sauber, ein Commit pro Feature |
| **bugfix/** → develop | **Squash and merge** ✅ | Hält develop-Historie sauber, ein Commit pro Fix |
| **release/** → main | **Merge commit** | Erhält vollständige Release-Historie und Commit-Metadaten |
| **hotfix/** → main | **Merge commit** | Erhält vollständige Hotfix-Historie für Audit-Zwecke |

**Warum Squash Merge für Features/Bugfixes?**
- ✅ Saubere, lesbare Git-Historie
- ✅ Ein logischer Commit pro Feature/Fix
- ✅ Einfacher zu reverten bei Bedarf
- ✅ Bessere Changelog-Generierung
- ❌ Entwicklungs-Commits (WIP, fix typo, etc.) bleiben im Feature-Branch

**GitHub Repository-Einstellungen konfigurieren:**

Maintainer sollten die Repository-Einstellungen auf GitHub entsprechend konfigurieren:
1. Gehe zu Settings → General → Pull Requests
2. Aktiviere "Allow squash merging" ✅
3. Aktiviere "Allow merge commits" ✅ (benötigt für Releases)
4. Deaktiviere "Allow rebase merging" ❌ (optional)
5. Setze "Squash merging" als Standard für das Repository

## Branch Protection Rules

### `main` Branch Protection

**Erforderliche Settings (via GitHub):**

```yaml
# Beispiel .github/branch-protection.yml
main:
  required_pull_request_reviews:
    required_approving_review_count: 1
    require_code_owner_reviews: true
    dismiss_stale_reviews: true
  required_status_checks:
    strict: true
    contexts:
      - "CI / Build & Test (ubuntu-latest)"
      - "CI / Build & Test (windows-latest)"
      - "Code Quality / clang-tidy"
      - "Code Quality / cppcheck"
      - "Security / Gitleaks"
  enforce_admins: true
  required_linear_history: false  # Erlaubt merge commits
  allow_force_pushes: false
  allow_deletions: false
  restrictions:
    users: []
    teams: ["maintainers"]
```

**Manuelle Konfiguration:**
1. GitHub Repo Settings → Branches → Add rule
2. Branch name pattern: `main`
3. Enable:
   - ✅ Require a pull request before merging
   - ✅ Require approvals (1)
   - ✅ Require status checks to pass before merging
   - ✅ Require branches to be up to date before merging
   - ✅ Require conversation resolution before merging
   - ✅ Include administrators
   - ✅ Do not allow bypassing the above settings

### `develop` Branch Protection

```yaml
develop:
  required_pull_request_reviews:
    required_approving_review_count: 1
    dismiss_stale_reviews: false
  required_status_checks:
    strict: true
    contexts:
      - "CI / Build & Test (ubuntu-latest)"
      - "Code Quality / clang-tidy"
  enforce_admins: false
  allow_force_pushes: false
  allow_deletions: false
```

## Release Process

### 1. Feature Freeze (Start Release Cycle)

```bash
# Entwicklung abschließen
git checkout develop
git pull origin develop

# Release Branch erstellen
git checkout -b release/1.4.0

# Version bumpen
echo "1.4.0" > VERSION
./.github/workflows/04-release_create-release-archive.yml 1.4.0

git add VERSION CHANGELOG.md RELEASE_NOTES_v1.4.0.md
git commit -m "chore: Prepare release v1.4.0"
git push origin release/1.4.0
```

### 2. Release Testing & Stabilization

```bash
# Auf Release Branch arbeiten
git checkout release/1.4.0

# Bugfixes (nur kritische!)
git commit -m "fix(docs): Correct installation instructions"
git commit -m "fix(build): Resolve build warning on Windows"

# Push updates
git push origin release/1.4.0
```

### 3. Release Finalization

```bash
# Final checks
cd build
ctest --output-on-failure
cd ..
./scripts/check-quality.sh

# Merge zu main
git checkout main
git pull origin main
git merge --no-ff release/1.4.0 -m "Release v1.4.0"

# Tag erstellen
git tag -a v1.4.0 -m "Release v1.4.0

Highlights:
- Feature A: Description
- Feature B: Description
- Performance improvements

See RELEASE_NOTES_v1.4.0.md for details."

# Push main und Tags
git push origin main
git push origin v1.4.0

# Merge zurück zu develop
git checkout develop
git pull origin develop
git merge --no-ff release/1.4.0 -m "Merge release v1.4.0 back to develop"
git push origin develop

# Release Branch löschen
git branch -d release/1.4.0
git push origin --delete release/1.4.0
```

### 4. Automatisches Deployment

GitHub Actions wird automatisch getriggert durch den Tag `v1.4.0`:
- Docker Images bauen und pushen
- Release Notes generieren
- GitHub Release erstellen
- Dokumentation deployen

## Pull Request Workflows

### Feature Development → develop

```bash
# Feature entwickeln
git checkout -b feature/new-awesome-feature develop
# ... commits ...
git push origin feature/new-awesome-feature
```

**PR erstellen:**
- **Base**: `develop`
- **Compare**: `feature/new-awesome-feature`
- **Title**: `feat(module): Add awesome feature`
- **Labels**: `enhancement`, `feature`
- **Reviewers**: Team member auswählen

**PR Checklist:**
- [ ] Code follows style guidelines
- [ ] Tests added and passing
- [ ] Documentation updated
- [ ] No new warnings
- [ ] CI checks passing

### Hotfix → main + develop

```bash
# Hotfix erstellen
git checkout -b hotfix/1.3.4-critical-bug main
# ... fix ...
git push origin hotfix/1.3.4-critical-bug
```

**PR zu main erstellen:**
- **Base**: `main`
- **Compare**: `hotfix/1.3.4-critical-bug`
- **Title**: `hotfix: Fix critical production bug`
- **Labels**: `hotfix`, `critical`
- **Priority**: HIGH

**Nach Merge zu main:**
- Cherry-pick zu develop oder neuer PR zu develop

## Merge Strategies

### Feature → develop
- **Strategie**: Squash and Merge (bevorzugt) oder Merge Commit
- **Grund**: Saubere History, Feature als eine Einheit

### Release → main
- **Strategie**: Merge Commit (--no-ff)
- **Grund**: Release-Geschichte bewahren

### Release → develop (back-merge)
- **Strategie**: Merge Commit (--no-ff)
- **Grund**: Änderungen aus Release-Phase übernehmen

### Hotfix → main
- **Strategie**: Merge Commit (--no-ff)
- **Grund**: Hotfix-Geschichte bewahren

## Best Practices

### ✅ DOs

1. **Immer von develop branchen** (außer Hotfixes)
   ```bash
   git checkout develop
   git pull origin develop
   git checkout -b feature/my-feature
   ```

2. **Regelmäßig develop pullen**
   ```bash
   git checkout feature/my-feature
   git pull origin develop
   # Merge conflicts lösen falls nötig
   ```

3. **Aussagekräftige Commit Messages**
   ```bash
   feat(storage): Add incremental backup support
   fix(query): Resolve off-by-one error in pagination
   docs(readme): Update installation instructions
   ```

4. **Branch nach Merge löschen**
   ```bash
   git push origin --delete feature/my-feature
   ```

5. **Klein und fokussiert bleiben**
   - Ein Feature = Ein Branch
   - Regelmäßig committen
   - Früh Pull Request öffnen (Draft PR für Feedback)

### ❌ DON'Ts

1. ❌ **Nicht direkt auf main/develop pushen**
2. ❌ **Keine langen Feature-Branches** (> 2 Wochen)
3. ❌ **Kein Cherry-Picking ohne Grund**
4. ❌ **Keine Merge von main in Feature-Branches**
5. ❌ **Keine Force-Pushes auf shared Branches**

## Migration Guide für Existing Contributors

### Für Entwickler mit offenen PRs zu main

1. **PR Basis ändern:**
   ```bash
   # Lokalen Branch aktualisieren
   git checkout your-feature-branch
   git fetch origin
   
   # Rebase auf develop
   git rebase origin/develop
   
   # Force push (nur für Feature-Branches!)
   git push origin your-feature-branch --force-with-lease
   ```

2. **PR Target auf GitHub ändern:**
   - PR öffnen
   - "Edit" neben Base Branch klicken
   - Von `main` zu `develop` ändern

### Für neue Features

Ab sofort:
```bash
# NEU: Von develop branchen
git checkout develop
git pull origin develop
git checkout -b feature/my-feature

# ALT (nicht mehr verwenden):
# git checkout main
# git checkout -b feature/my-feature
```

## Versioning Schema

ThemisDB folgt **Semantic Versioning 2.0.0**:

```
MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]

Beispiele:
1.4.0         - Standard Release
1.4.1         - Patch Release (Bugfix)
2.0.0         - Major Release (Breaking Changes)
2.0.0-beta.1  - Pre-Release
2.0.0+20231215 - Build Metadata
```

**Version Bump Regeln:**
- **MAJOR**: Breaking Changes (API Änderungen)
- **MINOR**: Neue Features (backward-compatible)
- **PATCH**: Bugfixes (backward-compatible)

## CI/CD Integration

### GitHub Actions Trigger

**develop Branch:**
```yaml
on:
  push:
    branches: [develop]
  pull_request:
    branches: [develop]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run tests
        run: |
          ./build.sh
          cd build && ctest
```

**main Branch (Release):**
```yaml
on:
  push:
    branches: [main]
    tags: ['v*']

jobs:
  release:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build and Deploy
        run: ./scripts/deploy-release.sh
```

## Troubleshooting

### Problem: Feature-Branch ist veraltet

**Symptom**: Merge conflicts mit develop

**Lösung:**
```bash
git checkout feature/my-feature
git fetch origin
git rebase origin/develop

# Conflicts lösen
git add .
git rebase --continue

# Force push (sicher für Feature-Branch)
git push origin feature/my-feature --force-with-lease
```

### Problem: Versehentlicher Commit auf develop

**Lösung:**
```bash
# Letzten Commit rückgängig machen (lokal)
git checkout develop
git reset --soft HEAD~1

# Feature Branch erstellen
git checkout -b feature/my-feature
git commit -m "feat: My feature"
git push origin feature/my-feature
```

### Problem: Hotfix muss auch zu develop

**Lösung 1: Cherry-Pick**
```bash
git checkout develop
git cherry-pick <hotfix-commit-sha>
git push origin develop
```

**Lösung 2: Merge**
```bash
git checkout develop
git merge --no-ff hotfix/1.3.4-fix
git push origin develop
```

## Zusätzliche Ressourcen

- **Git Flow**: https://nvie.com/posts/a-successful-git-branching-model/
- **Semantic Versioning**: https://semver.org/
- **Conventional Commits**: https://www.conventionalcommits.org/
- **GitHub Flow**: https://guides.github.com/introduction/flow/

## FAQ

### Wann erstelle ich einen release/* Branch?

Wenn develop stabil ist und bereit für Production:
- Alle geplanten Features für die Version sind merged
- Tests sind grün
- Dokumentation ist aktuell

### Kann ich mehrere Features gleichzeitig entwickeln?

Ja! Erstelle separate Feature-Branches:
```bash
git checkout -b feature/feature-a develop
git checkout -b feature/feature-b develop
```

### Was wenn mein Feature auf einem anderen Feature aufbaut?

**Option 1**: Warte bis das erste Feature in develop gemerged ist

**Option 2**: Branche temporär vom anderen Feature-Branch:
```bash
git checkout feature/feature-a
git checkout -b feature/feature-b-depends-on-a

# Später: Rebase auf develop wenn feature-a gemerged
git rebase origin/develop
```

### Wie lang darf ein Feature-Branch leben?

**Empfehlung**: Max. 1-2 Wochen

**Warum**: Je länger der Branch lebt, desto höher die Wahrscheinlichkeit von Merge-Konflikten

**Tipp**: Große Features in kleinere Tasks aufteilen

## Kontakt

Bei Fragen zur Branching Strategy:
- GitHub Discussions: https://github.com/makr-code/ThemisDB/discussions
- GitHub Issues: Erstelle ein Issue mit Label `question`

---

**Stand**: 2026-04-06  
**Version**: 1.0  
**Maintainer**: ThemisDB Core Team
