---
name: Build Error
about: Report eines automatisch erkannten Build-Fehlers aus dem Nightly Build
title: '[Nightly Build] Fehler in '
labels: build-error, nightly-build, automated, bug
assignees: ''
---

## 🚨 Nightly Build Fehler

**Betroffene Datei:** `path/to/file`
**Build Run:** [#123456](link)
**Datum:** YYYY-MM-DD
**Commit:** `abc123`

### Fehler Details

#### Konfiguration 1: OS-Compiler-BuildType
```
Fehlerausgabe hier
```

#### Konfiguration 2: OS-Compiler-BuildType
```
Fehlerausgabe hier
```

### Betroffene Build-Konfigurationen

| Konfiguration | Status | Details |
|---------------|--------|---------|
| ubuntu-22.04-gcc-12-Release | ❌ | Fehler beim Kompilieren |
| ubuntu-24.04-clang-16-Debug | ❌ | Fehler beim Kompilieren |

### Reproduktion

```bash
# Checkout des fehlerhaften Commits
git checkout abc123

# Build konfigurieren
cmake -B build -DCMAKE_BUILD_TYPE=Release -DTHEMIS_BUILD_TESTS=ON

# Build ausführen
cmake --build build
```

### Fehleranalyse

**Kategorie:** [linking|syntax|missing_file|ambiguity|deprecated|other]

**Vermutliche Ursache:**
- [ ] Fehlende Header-Datei
- [ ] Linking-Problem mit Dependencies
- [ ] Syntax-Fehler
- [ ] Platform-spezifisches Problem
- [ ] Compiler-spezifisches Problem

### Nächste Schritte

- [ ] Fehlerursache analysieren
- [ ] Fix implementieren
- [ ] Tests hinzufügen/aktualisieren
- [ ] PR erstellen
- [ ] Nach Fix: Nightly Build überwachen

### Zusätzliche Informationen

**Betroffene Dependencies:**
- 

**Verwandte Issues:**
- 

**Notizen:**
- 

---
*Dieses Issue wurde automatisch vom Nightly Build System erstellt.*
*Bei Duplikaten bitte mit dem ältesten Issue zusammenführen.*
