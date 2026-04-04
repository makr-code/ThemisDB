# ThemisDB Kompendium - Debug-YAML-Dateien

## Übersicht

Während des Build-Prozesses werden automatisch **Debug-YAML-Dateien** generiert, die alle Pattern-Matching-Ergebnisse dokumentieren. Diese Dateien ermöglichen einfaches Debugging und Validierung des Anchor- und Link-Systems.

## Generierte Debug-Dateien

Alle Debug-Dateien werden im `output/` Verzeichnis gespeichert:

```
output/
├── debug-references.yml         # Literaturverweise (IEEE-Format)
├── debug-citations.yml          # Zitationen im Fließtext
├── debug-headings.yml           # Alle Überschriften mit IDs
├── debug-tables.yml             # Alle Tabellen mit IDs
├── debug-figures.yml            # Alle Diagramme mit IDs
└── debug-anchors-complete.yml   # Vollständige Anchor-Registry
```

---

## 1. debug-references.yml

**Zweck:** Zeigt alle erkannten Literaturverweise im IEEE-Format

**Struktur:**
```yaml
total_references: 63
references:
- id: ref-1
  title: Literaturverweis [1]
  source_file: appendix_literatur
- id: ref-2
  title: Literaturverweis [2]
  source_file: appendix_literatur
# ... weitere Referenzen
```

**Verwendung:**
- Überprüfen, ob alle Literaturverweise erkannt wurden
- Validieren der Referenz-IDs
- Debugging von IEEE-Pattern-Matching

**Typische Probleme:**
- Doppelte IDs → Referenz wird in mehreren Dateien definiert
- Fehlende Referenzen → Pattern-Matching funktioniert nicht
- Falsche Nummerierung → Prüfe Markdown-Format

---

## 2. debug-citations.yml

**Zweck:** Zeigt alle Zitationen im Fließtext

**Struktur:**
```yaml
total_citations: 13
by_type:
  single: 4      # z.B. [1]
  list: 9        # z.B. [1,2,3]
  range: 0       # z.B. [1-5]
citations:
- citation: '[1]'
  type: single
- citation: '[1,2,3]'
  type: list
- citation: '[10-15]'
  type: range
```

**Verwendung:**
- Überprüfen, welche Zitationen erkannt wurden
- Validieren von Ranges und Listen
- Debugging von Zitations-Pattern-Matching

**Typische Probleme:**
- Zu viele `[0]` Zitationen → Wahrscheinlich Array-Indices oder Code-Beispiele
- Fehlende Zitationen → Pattern-Matching zu restriktiv
- Falsche Typ-Erkennung → Regex prüfen

---

## 3. debug-headings.yml

**Zweck:** Zeigt alle Überschriften mit generierten Anchor-IDs

**Struktur:**
```yaml
total_headings: 145
by_level:
  '2': 45
  '3': 78
  '4': 22
headings:
- id: rocksdb-storage-architecture
  title: RocksDB Storage Architecture
  level: '2'
  source_file: chapter_08_storage_layer
- id: performance-optimization
  title: Performance Optimization
  level: '3'
  source_file: chapter_21_performance
# ... weitere Überschriften
```

**Verwendung:**
- Überprüfen der Überschrifts-IDs (slugified)
- Validieren der Level-Verteilung
- Debugging von Heading-Anchor-Generierung

**Typische Probleme:**
- Doppelte IDs → Gleiche Überschrift in mehreren Kapiteln
- Leere IDs → Überschrift enthält nur Sonderzeichen
- Falsche Level → Markdown-Hierarchie prüfen

---

## 4. debug-tables.yml

**Zweck:** Zeigt alle Tabellen mit generierten IDs

**Struktur:**
```yaml
total_tables: 98
tables:
- id: table-chapter-01-introduction-1
  title: Tabelle 1
  source_file: chapter_01_introduction
- id: table-chapter-02-architecture-1
  title: Tabelle 1
  source_file: chapter_02_architecture
# ... weitere Tabellen
```

**Verwendung:**
- Überprüfen der Tabellen-IDs
- Validieren der Tabellen-Erkennung
- Debugging von Table-Anchor-Generierung

**Typische Probleme:**
- Fehlende Tabellen → Markdown-Tabelle nicht erkannt
- Falsche Counter → Counter wird nicht pro Kapitel resettet
- Doppelte IDs → Tabellen-ID-Format prüfen

---

## 5. debug-figures.yml

**Zweck:** Zeigt alle Diagramme mit IDs

**Struktur:**
```yaml
total_figures: 112
figures:
- id: diagram-1
  title: 'Abb. 1: ThemisDB Architecture Overview'
  source_file: chapter_01_introduction
- id: diagram-2
  title: 'Abb. 2: Multi-Model Data Storage'
  source_file: chapter_03_multimodel
# ... weitere Diagramme
```

**Verwendung:**
- Überprüfen der Diagramm-IDs
- Validieren der Diagramm-Titel
- Debugging von Figure-Anchor-Generierung

**Typische Probleme:**
- Fehlende Diagramme → SVG nicht generiert
- Falsche Nummerierung → SVG-Counter nicht korrekt
- Fehlende Titel → Mermaid-Kommentar fehlt

---

## 6. debug-anchors-complete.yml

**Zweck:** Vollständige Übersicht aller Anchors nach Typ

**Struktur:**
```yaml
version: v1.4.0
generated: '2026-01-13T12:30:00'
total_anchors: 336
by_type:
  chapter: 52
  part: 11
  figure: 112
  table: 98
  reference: 63
  heading-2: 45
  heading-3: 78
  heading-4: 22
anchors:
  chapter:
  - id: chapter_01_introduction
    title: Kapitel 1 - Einführung
    source: chapter_01_introduction
  - id: chapter_02_architecture
    title: Kapitel 2 - Architektur
    source: chapter_02_architecture
  # ... weitere Kapitel
  
  figure:
  - id: diagram-1
    title: 'Abb. 1: ThemisDB Architecture Overview'
    source: chapter_01_introduction
  # ... weitere Diagramme
  
  reference:
  - id: ref-1
    title: Literaturverweis [1]
    source: appendix_literatur
  # ... weitere Referenzen
```

**Verwendung:**
- Komplette Übersicht aller Anchors
- Validierung der Anchor-Verteilung
- Schnelle Suche nach spezifischen Anchors

**Typische Probleme:**
- Gesamtzahl stimmt nicht → Anchor-Registrierung fehlerhaft
- Typ-Verteilung unbalanciert → Pattern-Matching prüfen
- Fehlende Typen → Anchor-Typ nicht registriert

---

## Debugging-Workflow

### Problem: Link funktioniert nicht im PDF

**Schritt 1:** Überprüfe, ob Anchor existiert
```bash
# Suche in debug-anchors-complete.yml
grep "chapter_01_introduction" output/debug-anchors-complete.yml
```

**Schritt 2:** Überprüfe Anchor-Typ
```bash
# Ist es ein Kapitel, Überschrift, Tabelle?
grep -A2 "chapter_01_introduction" output/debug-anchors-complete.yml
```

**Schritt 3:** Validiere Referenzen
```bash
# Suche alle Stellen, die auf diesen Anchor verlinken
grep "chapter_01_introduction" output/ThemisDB-Kompendium-v1.4.0.html
```

### Problem: Literaturverweis wird nicht erkannt

**Schritt 1:** Prüfe debug-references.yml
```bash
# Suche nach Referenz-ID
grep "ref-42" output/debug-references.yml
```

**Schritt 2:** Prüfe Markdown-Format
```markdown
# Korrekt (wird erkannt):
[42] Autor, "Titel", Journal, 2024.

# Falsch (wird NICHT erkannt):
[42]Autor, "Titel"    # Fehlendes Leerzeichen
42. Autor, "Titel"    # Punkt statt eckige Klammer
```

### Problem: Zitation im Text wird nicht verlinkt

**Schritt 1:** Prüfe debug-citations.yml
```bash
# Suche nach Zitation
grep "\[42\]" output/debug-citations.yml
```

**Schritt 2:** Überprüfe HTML
```bash
# Wurde Zitation konvertiert?
grep '<a href="#ref-42">' output/ThemisDB-Kompendium-v1.4.0.html
```

**Schritt 3:** Prüfe, ob Zitation in Referenz-Definition steht
```markdown
# Wird NICHT verlinkt (ist Referenz-Definition):
[42] Autor, "Titel"

# Wird verlinkt (ist Zitation):
Wie in [42] gezeigt...
```

### Problem: Überschrifts-Anchor fehlt

**Schritt 1:** Prüfe debug-headings.yml
```bash
# Suche nach Überschrift
grep "Performance" output/debug-headings.yml
```

**Schritt 2:** Überprüfe Slug-Generierung
```python
# "Performance Optimization" → "performance-optimization"
# Regel: Kleinbuchstaben, Bindestriche, max. 60 Zeichen
```

**Schritt 3:** Prüfe auf Duplikate
```bash
# Gibt es mehrere Überschriften mit gleichem Text?
grep -c "performance-optimization" output/debug-headings.yml
```

---

## Pattern-Matching-Regeln

### IEEE-Literaturverweise
```regex
Pattern: <p>\[(\d+)\]\s+([^<]*(?:<[^p][^>]*>[^<]*</[^p][^>]*>[^<]*)*)
Beispiel: [1] Autor, "Titel", Journal, 2024.
```

### Zitationen im Text
```regex
Pattern: \[(\d+(?:\s*-\s*\d+)?(?:\s*,\s*\d+)*)\]
Beispiele:
  - [1]           → Single
  - [1-5]         → Range
  - [1,2,3]       → List
  - [10, 12, 15]  → List mit Leerzeichen
```

### Überschriften
```regex
Pattern: <h([1-6])(?![^>]*id=)>([^<]+)</h\1>
Beispiel: <h2>Performance Optimization</h2>
Result:   <h2 id="performance-optimization">Performance Optimization</h2>
```

### Tabellen
```regex
Pattern: <table(?![^>]*id=)(?:[^>]*)>.*?</table>
Result:  <table id="table-chapter-01-1">...</table>
```

---

## YAML-Validierung

### Syntax-Prüfung
```bash
# Mit Python
python -c "import yaml; yaml.safe_load(open('output/debug-references.yml'))"

# Mit PowerShell
Get-Content output/debug-references.yml | ConvertFrom-Yaml
```

### Konsistenz-Checks
```bash
# Anzahl Referenzen in debug-references.yml == Anzahl in debug-anchors-complete.yml?
grep "total_references:" output/debug-references.yml
grep "reference:" output/debug-anchors-complete.yml

# Anzahl Diagramme == SVG-Dateien?
ls output/mermaid_svg/*.svg | wc -l
grep "total_figures:" output/debug-figures.yml
```

---

## Automatische Validierung

Erstelle ein Validierungs-Script:

```python
#!/usr/bin/env python3
"""Validiere Debug-YAML-Dateien gegen Anchor-Registry."""

import yaml
import json
from pathlib import Path

OUTPUT_DIR = Path("output")

# Lade alle Debug-Dateien
with open(OUTPUT_DIR / "debug-anchors-complete.yml") as f:
    complete = yaml.safe_load(f)

with open(OUTPUT_DIR / "debug-references.yml") as f:
    refs = yaml.safe_load(f)

with open(OUTPUT_DIR / "debug-citations.yml") as f:
    cites = yaml.safe_load(f)

# Validierung
print("=== YAML Debug Validation ===")
print(f"Total anchors: {complete['total_anchors']}")
print(f"References: {refs['total_references']}")
print(f"Citations: {cites['total_citations']}")

# Prüfe Konsistenz
ref_count_complete = complete['by_type'].get('reference', 0)
ref_count_debug = refs['total_references']

if ref_count_complete == ref_count_debug:
    print("✓ Reference counts match")
else:
    print(f"✗ Mismatch: {ref_count_complete} vs {ref_count_debug}")

# Prüfe auf doppelte IDs
all_ids = [a['id'] for a in complete['anchors'].values() for a in a]
duplicates = [id for id in set(all_ids) if all_ids.count(id) > 1]

if duplicates:
    print(f"✗ Duplicate IDs found: {duplicates}")
else:
    print("✓ No duplicate IDs")
```

---

## Zusammenfassung

**Debug-YAML-Dateien bieten:**
✅ Transparenz über Pattern-Matching
✅ Einfaches Debugging von Anchor-Problemen
✅ Validierung der Anchor-Verteilung
✅ Nachvollziehbare Link-Generierung
✅ Automatische Konsistenz-Checks

**Best Practices:**
1. Immer Debug-YAMLs nach Build überprüfen
2. Bei Link-Problemen: Debug-YAML konsultieren
3. Pattern-Matching-Änderungen: Debug-Output vergleichen
4. Doppelte IDs sofort beheben
5. Regelmäßige Validierung der Konsistenz
