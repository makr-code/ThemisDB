# Nightly Build System

Das Nightly Build System für ThemisDB führt automatisierte Builds über verschiedene Plattformen, Compiler und Build-Typen durch und erstellt automatisch GitHub Issues für erkannte Fehler.

## Übersicht

### Was macht das Nightly Build System?

1. **Automatische Builds**: Führt täglich um 2 Uhr UTC Builds auf verschiedenen Plattformen durch
2. **Fehler-Erkennung**: Parst Compiler-Ausgaben und identifiziert Fehler und Warnungen
3. **Issue-Erstellung**: Erstellt automatisch GitHub Issues für erkannte Build-Fehler
4. **Deduplizierung**: Vermeidet doppelte Issues durch intelligente Suche
5. **Reporting**: Generiert detaillierte Build-Reports im GitHub UI

### Build-Matrix

Das System testet folgende Kombinationen:

**Betriebssysteme:**
- Ubuntu 22.04
- Ubuntu 24.04
- Windows Latest
- macOS Latest

**Compiler:**
- GCC 11, 12, 13
- Clang 15, 16
- MSVC (Windows)

**Build-Typen:**
- Release
- Debug

## Workflow-Struktur

### Jobs

#### 1. build-matrix
Führt Builds für alle Kombinationen der Matrix durch:
- Checkout mit Submodules
- Installation der Dependencies
- CMake-Konfiguration mit strengen Compiler-Flags
- Build-Ausführung (mit `continue-on-error: true`)
- Fehler-Parsing mit Python-Script
- Upload von Build-Artifacts (Logs, Fehler-Summaries)
- Test-Ausführung (falls Build erfolgreich)

#### 2. create-error-issues
Aggregiert alle Fehler und erstellt GitHub Issues:
- Download aller Build-Artifacts
- Aggregation von Fehlern nach Datei
- Prüfung auf bereits existierende Issues
- Erstellung neuer Issues oder Kommentare auf bestehenden Issues

#### 3. summary
Generiert eine Zusammenfassung aller Builds:
- Download aller Artifacts
- Aggregation der Ergebnisse
- Erstellung einer übersichtlichen Tabelle im GitHub Step Summary

## Verwendung

### Automatischer Trigger

Der Workflow läuft automatisch täglich um 2 Uhr UTC:

```yaml
schedule:
  - cron: '0 2 * * *'
```

### Manueller Trigger

Über die GitHub UI kann der Workflow manuell gestartet werden:

```bash
# Via GitHub UI: Actions → Nightly Build → Run workflow
```

**Option:** `create_issues` (boolean, default: true)
- `true`: Issues werden automatisch erstellt
- `false`: Nur Builds durchführen, keine Issues erstellen

### Via GitHub CLI

```bash
gh workflow run nightly-build-with-issue-tracking.yml
gh workflow run nightly-build-with-issue-tracking.yml -f create_issues=false
```

## Fehler-Kategorisierung

Das System kategorisiert Fehler automatisch:

| Kategorie | Beschreibung | Beispiele |
|-----------|--------------|-----------|
| **linking** | Linker-Fehler | undefined reference, unresolved external |
| **syntax** | Syntax-Fehler | syntax error, expected ';' |
| **missing_file** | Fehlende Dateien | no such file, cannot find |
| **ambiguity** | Mehrdeutigkeiten | ambiguous call, overload resolution |
| **deprecated** | Deprecated Warnings | deprecated feature |
| **other** | Andere Fehler | Alle nicht kategorisierten Fehler |

## Issue-Management

### Issue-Erstellung

Für jeden erkannten Fehler wird ein strukturiertes Issue erstellt:

**Titel:** `[Nightly Build] Fehler in {file}`

**Body-Struktur:**
```markdown
## 🚨 Nightly Build Fehler

**Betroffene Datei:** `src/example.cpp`
**Build Run:** [#123456](link)
**Datum:** 2026-02-10

### Fehler Details

#### 1. ubuntu-22.04-gcc-12-Release
\`\`\`
src/example.cpp:42:15: error: 'xyz' was not declared in this scope
\`\`\`

### Reproduktion
\`\`\`bash
git checkout abc123
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
\`\`\`

### Nächste Schritte
- [ ] Fehlerursache analysieren
- [ ] Fix implementieren
- [ ] Tests hinzufügen
- [ ] PR erstellen
```

**Labels:**
- `build-error`: Kennzeichnet Build-Fehler
- `nightly-build`: Vom Nightly Build System erstellt
- `automated`: Automatisch erstellt
- `bug`: Bug-Label

### Deduplizierung

Das System prüft vor der Issue-Erstellung:
1. Suche nach offenen Issues mit Label `build-error`
2. Prüfung ob die betroffene Datei bereits ein Issue hat
3. Falls ja: Kommentar hinzufügen statt neues Issue
4. Falls nein: Neues Issue erstellen

## Artifacts

Folgende Artifacts werden für 30 Tage gespeichert:

| Artifact | Beschreibung | Format |
|----------|--------------|--------|
| `configure.log` | CMake-Konfigurationslog | Text |
| `build.log` | Build-Output | Text |
| `error_summary.json` | Strukturierte Fehler-Daten | JSON |
| `error_summary.md` | Markdown-Report | Markdown |

### JSON-Struktur

```json
{
  "total_errors": 5,
  "total_warnings": 12,
  "categories": {
    "linking": 2,
    "syntax": 3
  },
  "top_errors": [
    {
      "file": "src/example.cpp",
      "line": 42,
      "column": 15,
      "severity": "error",
      "message": "'xyz' was not declared in this scope"
    }
  ],
  "affected_files": ["src/example.cpp", "src/another.cpp"]
}
```

## Konfiguration anpassen

### Build-Matrix erweitern

```yaml
matrix:
  os: [ubuntu-22.04, ubuntu-24.04]  # Füge/entferne OS hinzu
  compiler: [gcc-11, gcc-12]        # Füge/entferne Compiler hinzu
  build_type: [Release, Debug]      # Füge/entferne Build-Types hinzu
```

### Schedule ändern

```yaml
schedule:
  - cron: '0 2 * * *'  # Täglich 2 Uhr UTC
  # Alternativen:
  # - cron: '0 */6 * * *'  # Alle 6 Stunden
  # - cron: '0 0 * * 1'    # Wöchentlich Montags
```

### Dependencies anpassen

Für Linux (Ubuntu):
```yaml
- name: Install dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y cmake ninja-build libssl-dev librocksdb-dev
```

Für macOS:
```yaml
- name: Install dependencies
  run: |
    brew install cmake ninja openssl rocksdb
```

Für Windows (MSVC):
```yaml
- name: Install dependencies
  run: |
    choco install cmake ninja
```

## Monitoring

### Build-Status prüfen

```bash
# Liste der letzten Workflow-Runs
gh run list --workflow=nightly-build-with-issue-tracking.yml

# Details zu einem spezifischen Run
gh run view <run-id>

# Logs herunterladen
gh run download <run-id>
```

### Issues überwachen

```bash
# Liste aller Build-Error Issues
gh issue list --label build-error

# Offene Build-Error Issues
gh issue list --label build-error --state open

# Nach Datei suchen
gh issue list --label build-error --search "src/example.cpp"
```

## Troubleshooting

### Workflow schlägt komplett fehl

1. Prüfe GitHub Actions Status
2. Prüfe Permissions (issues: write, contents: read)
3. Prüfe Branch-Schutz-Regeln

### Fehler werden nicht geparst

1. Prüfe Compiler-Output-Format
2. Passe Regex-Pattern in `parse_build_errors.py` an
3. Teste lokal mit echten Logs

### Doppelte Issues werden erstellt

1. Prüfe Issue-Suche im Workflow
2. Stelle sicher dass Labels korrekt gesetzt werden
3. Prüfe ob GitHub API-Rate-Limits erreicht sind

### Build-Matrix unvollständig

1. Prüfe verfügbare Runner (OS)
2. Prüfe Compiler-Installation
3. Prüfe Timeout-Einstellungen (default: 60 Minuten)

## Best Practices

### Issue-Behandlung

1. **Schnelle Triage**: Priorisiere Issues nach Häufigkeit
2. **Kategorien nutzen**: Nutze die Fehler-Kategorien für schnelle Einordnung
3. **Trends erkennen**: Überwache wiederkehrende Fehler
4. **Fix-Verifizierung**: Schließe Issues erst nach erfolgreichem Nightly Build

### Workflow-Optimierung

1. **fail-fast: false**: Alle Kombinationen durchlaufen lassen
2. **continue-on-error: true**: Fehler nicht den Workflow stoppen lassen
3. **Artifacts-Retention**: 30 Tage für ausreichende Analyse
4. **Matrix-Strategie**: Wichtigste Kombinationen zuerst

### Maintenance

1. **Regelmäßige Updates**: Compiler-Versionen aktualisieren
2. **Dependency-Updates**: Dependencies auf dem neuesten Stand halten
3. **Template-Pflege**: Issue-Templates bei Bedarf anpassen
4. **Dokumentation**: Diese Dokumentation aktuell halten

## Weiterführende Ressourcen

- [GitHub Actions Dokumentation](https://docs.github.com/en/actions)
- [GitHub Issues API](https://docs.github.com/en/rest/issues)
- [CMake Dokumentation](https://cmake.org/documentation/)
- [Compiler-spezifische Flags](../build-guide/)

## Support

Bei Fragen oder Problemen:
1. Prüfe diese Dokumentation
2. Suche in den Issues nach ähnlichen Problemen
3. Erstelle ein Issue mit Label `ci-cd` und `question`
