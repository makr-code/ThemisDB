# ThemisDB Namespace Analyzer

## Übersicht

Der Namespace Analyzer ist ein Python-Tool zur Analyse der ThemisDB-Codebasis. Es extrahiert und dokumentiert:

- **Namespaces** und ihre Hierarchien
- **Klassen, Structs und Enums** innerhalb jeder Namespace
- **Funktionen** und ihre Signaturen
- **Variablen und Konstanten**
- **Zeitliche Informationen** (wann jede Entität eingeführt/geändert wurde) via Git-Metadaten

## Verwendung

### Grundlegende Verwendung

```bash
cd /home/runner/work/ThemisDB/ThemisDB
python3 tools/namespace_analyzer.py
```

Dies analysiert den gesamten Quellcode und erstellt Berichte im Verzeichnis `./namespace_analysis/`.

### Optionen

```bash
python3 tools/namespace_analyzer.py [OPTIONEN]
```

**Verfügbare Optionen:**

- `--output-dir DIR` - Ausgabeverzeichnis für Berichte (Standard: `./namespace_analysis`)
- `--format FORMAT` - Ausgabeformat: `json`, `markdown`, `csv`, oder `all` (Standard: `all`)
- `--include-git` - Git-Metadaten einbeziehen (Zeitstempel, Autoren)
- `--verbose` - Ausführliche Ausgabe aktivieren
- `--repo-root DIR` - Repository-Root-Verzeichnis (Standard: auto-detect)

### Beispiele

**Alle Berichte mit Git-Metadaten generieren:**
```bash
python3 tools/namespace_analyzer.py --include-git
```

**Nur Markdown-Bericht erstellen:**
```bash
python3 tools/namespace_analyzer.py --format markdown
```

**In ein bestimmtes Verzeichnis ausgeben:**
```bash
python3 tools/namespace_analyzer.py --output-dir /tmp/themis_analysis --verbose
```

## Ausgabeformate

### 1. JSON Format (`namespace_analysis.json`)

Strukturierte Daten für maschinelle Verarbeitung:

```json
{
  "metadata": {
    "timestamp": "2025-12-27T12:00:00",
    "repo_root": "/path/to/ThemisDB",
    "total_namespaces": 123,
    "git_metadata_included": true
  },
  "namespaces": {
    "themis": {
      "name": "themis",
      "full_name": "themis",
      "classes": [...],
      "functions": [...],
      "variables": [...]
    }
  }
}
```

### 2. Markdown Format (`namespace_analysis.md`)

Menschenlesbarer Bericht mit:

- Zusammenfassung der Namespace-Statistiken
- Top-Namespaces nach Entitätenzahl
- Detaillierte Informationen für jeden Namespace
- Klassen mit Vererbungshierarchie
- Funktionssignaturen
- Variablen mit Typ-Informationen
- Optional: Git-Historie (wann eingeführt/zuletzt geändert)

### 3. CSV Format (mehrere Dateien)

Tabellarische Daten für Spreadsheet-Analyse:

- `namespaces.csv` - Übersicht aller Namespaces
- `classes.csv` - Alle Klassen mit Details
- `functions.csv` - Alle Funktionen mit Signaturen

## Funktionen im Detail

### Namespace-Analyse

- Erkennt verschachtelte Namespaces (z.B. `themis::llm::inference`)
- Zeigt Parent-Child-Beziehungen
- Listet alle Dateien, die zu einem Namespace beitragen

### Klassen-Analyse

Für jede Klasse wird erfasst:

- Name und Typ (class, struct, enum)
- Template-Parameter (falls vorhanden)
- Basisklassen (Vererbung)
- Datei und Zeilennummer
- Git-Historie (optional)

### Funktions-Analyse

Für jede Funktion wird erfasst:

- Name und vollständige Signatur
- Rückgabetyp
- Parameter
- Modifikatoren (static, virtual, const, template)
- Datei und Zeilennummer
- Git-Historie (optional)

### Variablen-Analyse

Für jede Variable wird erfasst:

- Name und Typ
- Modifikatoren (const, constexpr, static)
- Datei und Zeilennummer
- Git-Historie (optional)

### Git-Integration

Mit der Option `--include-git` wird für jede Entität erfasst:

- **Erster Commit**: Hash, Datum, Autor (wann wurde es eingeführt)
- **Letzter Commit**: Hash, Datum, Autor (wann zuletzt geändert)
- **Anzahl der Commits**: Wie oft wurde es geändert

Dies ermöglicht die Beantwortung von Fragen wie:

- Welche Klassen wurden zuletzt hinzugefügt?
- Wer hat eine bestimmte Funktion implementiert?
- Wie alt ist eine bestimmte Komponente?
- Welche Bereiche des Codes werden am häufigsten geändert?

## Technische Details

### Unterstützte Dateitypen

- C++ Header-Dateien (`.h`, `.hpp`, `.hh`)
- Durchsucht `include/` und `src/` Verzeichnisse

### Parser-Funktionen

Der Analyzer verwendet reguläre Ausdrücke, um zu erkennen:

- Namespace-Deklarationen
- Klassen/Struct/Enum-Deklarationen mit Template-Support
- Funktionsdeklarationen mit verschiedenen Modifikatoren
- Variablen- und Konstantendeklarationen

### Einschränkungen

- Der Parser ist vereinfacht und erkennt möglicherweise nicht alle komplexen C++-Konstrukte
- Makro-Definitionen werden nicht vollständig aufgelöst
- Inline-Implementierungen in Header-Dateien werden als Deklarationen behandelt
- Git-Metadaten-Abfragen können bei sehr großen Repositories langsam sein

## Beispiel-Output

### Zusammenfassung

```
Starting namespace analysis of ThemisDB at /path/to/ThemisDB
Git metadata: enabled
Found 314 header files to analyze
Analysis complete. Found 85 namespaces
  Total classes: 245
  Total functions: 1823
  Total variables: 167
```

### Top Namespaces

| Namespace | Classes | Functions | Variables | Total |
|-----------|---------|-----------|-----------|-------|
| `themis` | 45 | 312 | 28 | 385 |
| `themis::llm` | 18 | 95 | 12 | 125 |
| `themis::query` | 22 | 87 | 8 | 117 |
| `themis::storage` | 15 | 76 | 6 | 97 |

## Integration in ThemisDB

Der Namespace Analyzer kann in verschiedene Workflows integriert werden:

### 1. Dokumentationsgenerierung

```bash
# Erstelle aktuelle Namespace-Dokumentation
python3 tools/namespace_analyzer.py --format markdown --output-dir docs/api/namespaces/
```

### 2. CI/CD Pipeline

```yaml
# In GitHub Actions
- name: Analyze Namespaces
  run: |
    python3 tools/namespace_analyzer.py --format json --output-dir artifacts/
- name: Upload Analysis
  uses: actions/upload-artifact@v3
  with:
    name: namespace-analysis
    path: artifacts/namespace_analysis.json
```

### 3. Code Review

Vor großen Refactorings kann der Analyzer helfen, die aktuelle Struktur zu verstehen:

```bash
python3 tools/namespace_analyzer.py --include-git --format all
```

### 4. Metriken und Trends

Regelmäßige Analysen können zeigen, wie sich die Codebasis entwickelt:

```bash
# Monatliche Analyse
python3 tools/namespace_analyzer.py --include-git --output-dir "analysis_$(date +%Y%m)"
```

## Troubleshooting

### Problem: "Could not find repository root"

**Lösung:** Geben Sie den Repository-Root explizit an:
```bash
python3 tools/namespace_analyzer.py --repo-root /path/to/ThemisDB
```

### Problem: Git-Metadaten sind langsam

**Lösung:** Deaktivieren Sie Git-Integration oder nutzen Sie sie nur für wichtige Analysen:
```bash
python3 tools/namespace_analyzer.py  # ohne --include-git
```

### Problem: Zu viele oder zu wenige Entitäten gefunden

**Lösung:** Nutzen Sie den Verbose-Modus, um zu sehen, welche Dateien analysiert werden:
```bash
python3 tools/namespace_analyzer.py --verbose
```

## Weiterentwicklung

Mögliche Erweiterungen:

- [ ] Abhängigkeitsgraph zwischen Namespaces
- [ ] Zyklische Abhängigkeiten erkennen
- [ ] Code-Metriken (Komplexität, LOC)
- [ ] Visualisierung (interaktive HTML-Berichte)
- [ ] Vergleich zwischen Versionen/Branches
- [ ] Integration mit Doxygen
- [ ] Support für mehr C++-Konstrukte (concepts, modules)

## Lizenz

Dieses Tool ist Teil von ThemisDB und unterliegt derselben MIT-Lizenz.

## Autor

ThemisDB Development Team
