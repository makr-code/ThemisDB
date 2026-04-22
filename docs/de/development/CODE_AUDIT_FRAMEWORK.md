# ThemisDB - Comprehensive Offline Source Code Audit

**Version:** 1.0  
**Stand:** 6. April 2026

## Übersicht

Das ThemisDB Code Audit Framework bietet umfassende Offline-Prüfungen des Quellcodes vor dem Build-Prozess. Es konsolidiert alle vorhandenen Qualitätsprüfungen und erweitert sie um zusätzliche Sicherheits- und Qualitätsanalysen.

## Verfügbare Skripte

| Skript | Beschreibung | Plattform |
|--------|--------------|-----------|
| `comprehensive-code-audit.sh` | Vollständiges Audit-Framework (Bash) | Linux/macOS/WSL |
| `pre-build-validation.ps1` | Pre-Build Validierung (PowerShell) | Windows |
| `check-quality.sh` | Schnelle Qualitätsprüfung (Bash) | Linux/macOS/WSL |
| `check-quality.ps1` | Schnelle Qualitätsprüfung (PowerShell) | Windows |
| `run_clang_quality_wsl.sh` | Clang-Tools Wrapper | WSL |

## Comprehensive Code Audit

### Verwendung

```bash
# Vollständiges Audit
./scripts/comprehensive-code-audit.sh

# Schnelles Audit (überspringt tiefe Analyse)
./scripts/comprehensive-code-audit.sh --quick

# Strenger Modus (Warnungen als Fehler)
./scripts/comprehensive-code-audit.sh --strict

# Bestimmte Tools überspringen
./scripts/comprehensive-code-audit.sh --skip=clang-tidy,cppcheck

# HTML-Report generieren
./scripts/comprehensive-code-audit.sh --format=html

# Verbose Output
./scripts/comprehensive-code-audit.sh --verbose
```

### Durchgeführte Prüfungen

Das Audit-Framework führt 15 Kategorien von Prüfungen durch:

#### 1. Project Structure Validation
- Prüft erforderliche Verzeichnisse (`src`, `include`, `tests`, `docs`, `scripts`, `config`)
- Prüft erforderliche Dateien (`CMakeLists.txt`, `vcpkg.json`, `README.md`, `SECURITY.md`, `LICENSE`)

#### 2. Header Guard Verification
- Prüft alle Header-Dateien auf `#pragma once` oder `#ifndef/#define` Guards
- Meldet fehlende Guards

#### 3. Include Dependency Analysis
- Erkennt problematische Include-Patterns:
  - `<bits/stdc++.h>` (nicht portabel)
  - `using namespace std;` in Headers
  - Tiefe relative Includes (`../../..`)
- Prüft auf potenzielle zirkuläre Includes

#### 4. Clang-Tidy Static Analysis
- Führt clang-tidy auf Quelldateien aus
- Nutzt `compile_commands.json` aus dem Build-Verzeichnis
- Im Quick-Modus: nur geänderte Dateien

#### 5. Cppcheck Static Analysis
- Umfassende statische Analyse
- Checks: warning, style, performance, portability
- Unterdrückt bekannte False Positives

#### 6. Hardcoded Secrets Detection
- Sucht nach Patterns wie:
  - `password = "..."`, `api_key = "..."`
  - Private Keys (`-----BEGIN.*PRIVATE KEY-----`)
  - AWS Credentials
- Integriert gitleaks falls vorhanden

#### 7. Unsafe Memory Pattern Detection
- Erkennt unsichere C-Funktionen:
  - `malloc`/`free` (RAII bevorzugt)
  - `new[]`/`delete[]` (`std::vector` bevorzugt)
  - `strcpy`, `sprintf`, `gets`, `strcat`, `scanf`
- Unterstützt `// NOLINT` zur Unterdrückung

#### 8. Code Complexity Analysis
- Erkennt große Funktionen (>100 Zeilen)
- Meldet große Dateien (>1000 Zeilen)
- Hilft bei der Identifikation von Refactoring-Kandidaten

#### 9. TODO/FIXME/HACK Detection
- Zählt Annotationen: `TODO`, `FIXME`, `HACK`, `XXX`, `BUG`, `OPTIMIZE`, `REFACTOR`
- Warnt bei kritischen Annotationen (`FIXME`, `BUG`)

#### 10. Deprecated API Detection
- Erkennt veraltete APIs:
  - `std::auto_ptr` → `std::unique_ptr`
  - `gets()` (entfernt in C11)
  - `std::random_shuffle` → `std::shuffle`
  - `std::bind` → Lambda

#### 11. Thread Safety Analysis
- Prüft Mutex-Verwendung
- Prüft Lock-Guard-Verwendung (RAII)
- Zählt `std::atomic` Variablen
- Prüft `thread_local` Verwendung

#### 12. Exception Safety Analysis
- Zählt `noexcept` Spezifikationen
- Warnt vor generischen `catch(...)` Blöcken
- Prüft `throw()` Spezifikationen

#### 13. License Header Compliance
- Prüft ob Quelldateien License-Header enthalten
- Sucht nach: `license`, `copyright`, `MIT`, `Apache`, `GPL`

#### 14. Documentation Coverage
- Zählt Doxygen-Kommentare (`///`, `/**`)
- Prüft README.md in Schlüsselverzeichnissen

#### 15. Source Code Statistics
- Gesamtstatistik: Dateien, Zeilen
- Aufschlüsselung nach `.cpp`, `.h/.hpp`, Tests

## Output und Reports

### Console Output

```
╔══════════════════════════════════════════════════════════════════╗
║     ThemisDB - Comprehensive Offline Source Code Audit          ║
╚══════════════════════════════════════════════════════════════════╝

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
▶ 1. Project Structure Validation
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  ✓ Directory exists: src (234 files)
  ✓ Directory exists: include (156 files)
  ...

AUDIT SUMMARY
  Errors:     0
  Warnings:   5
  Info:       12

╔══════════════════════════════════════════════════════════════════╗
║                    ✓ AUDIT PASSED                                ║
╚══════════════════════════════════════════════════════════════════╝
```

### Report-Dateien

Reports werden im Verzeichnis `.audit-reports/` gespeichert:

```
.audit-reports/
├── audit_20251202_143052.txt
├── audit_20251202_151230.txt
└── ...
```

## Integration in Build-Prozess

### Pre-Commit Hook

```bash
# .git/hooks/pre-commit
#!/bin/bash
./scripts/comprehensive-code-audit.sh --quick --strict
```

### CI/CD Integration

```yaml
# .github/workflows/code-audit.yml
name: Code Audit
on: [push, pull_request]

jobs:
  audit:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install tools
        run: |
          sudo apt-get update
          sudo apt-get install -y clang-tidy cppcheck
      - name: Run audit
        run: ./scripts/comprehensive-code-audit.sh --strict
```

### Build-Skript Integration

```bash
# build.sh
#!/bin/bash
set -e

echo "Running pre-build audit..."
./scripts/comprehensive-code-audit.sh --quick

echo "Building..."
cmake -B build -S .
cmake --build build
```

## Tool-Anforderungen

### Erforderlich
- Bash 4.0+
- Git

### Optional (für erweiterte Prüfungen)
- **clang-tidy** - Statische Analyse
- **cppcheck** - Statische Analyse
- **gitleaks** - Secret Scanning

### Installation

**Ubuntu/Debian:**
```bash
sudo apt-get install clang-tidy cppcheck
# gitleaks manuell installieren von https://github.com/gitleaks/gitleaks
```

**macOS:**
```bash
brew install llvm cppcheck gitleaks
```

**Windows (WSL):**
```bash
sudo apt-get install clang-tidy cppcheck
```

## Konfiguration

### Unterdrückung von Warnungen

#### Im Code
```cpp
// Einzelne Zeile
void legacy_function() { // NOLINT

// Block
// NOLINTBEGIN
legacy_code();
// NOLINTEND
```

#### cppcheck Suppressions
Erstelle `.cppcheck-suppressions`:
```
missingIncludeSystem
unusedFunction:src/deprecated/*
```

#### clang-tidy Konfiguration
`.clang-tidy` im Projektstamm konfiguriert die Checks.

## Exit Codes

| Code | Bedeutung |
|------|-----------|
| 0 | Audit bestanden |
| 1 | Audit fehlgeschlagen (Fehler gefunden) |
| 1 | Audit fehlgeschlagen (Warnungen in --strict Modus) |

## Best Practices

1. **Vor jedem Commit:** `./scripts/comprehensive-code-audit.sh --quick`
2. **Vor jedem PR:** `./scripts/comprehensive-code-audit.sh --strict`
3. **Wöchentlich:** Vollständiges Audit mit `--verbose`
4. **Reports archivieren:** `.audit-reports/` regelmäßig überprüfen

## Troubleshooting

### "compile_commands.json not found"
```bash
# CMake erneut ausführen mit:
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
```

### "clang-tidy not installed"
Das Audit wird fortgesetzt, überspringt aber clang-tidy Checks.
Installieren Sie clang-tidy für vollständige Analyse.

### False Positives
Verwenden Sie `// NOLINT` oder fügen Sie Patterns zu `.cppcheck-suppressions` hinzu.

## Siehe auch

- [Build Guide](BUILD_GUIDE.md)
- [Code Quality](development/code_quality.md)
- [Security Policy](../SECURITY.md)
- [Contributing Guide](CONTRIBUTING.md)
