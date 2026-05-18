# Verwendung von Ankors im Fließtext - Beispiele

Diese Datei zeigt, wie man interne Links und Ankors im Kompendium verwendet.

## Kapitel-Links

```markdown
# Kapitel-Querverweis
Für Details zur Architektur siehe [Kapitel 2 - Architektur](#chapter_02_architecture).

Alternative Syntax:
- [Einführung](#chapter_01_introduction)
- [Graph-Modell](#chapter_06_graph)
- [Machine Learning](#chapter_18_ml)
```

**Im Fließtext:**
```markdown
Das ThemisDB-System ist in [Kapitel 2](#chapter_02_architecture) ausführlich dokumentiert. 
Weitere Informationen zur Storage-Schicht finden Sie in [Kapitel 8](#chapter_08_storage_layer).
```

---

## Diagramm-Links

```markdown
# Diagramm-Referenzen
Die folgende Abbildung (siehe [Abbildung 1](#diagram-1)) zeigt die Systemarchitektur.

Weitere Diagramme:
- [Diagramm zur Datenverarbeitung](#diagram-42)
- [Performance-Metriken](#diagram-99)

Im Text:
Wie in [Abb. 5](#diagram-5) dargestellt wird die Query-Verarbeitung durch mehrere Phasen 
optimiert, einschließlich des in [Diagramm 12](#diagram-12) gezeigten Caching-Mechanismus.
```

---

## Überschrifts-Links

```markdown
# Referenz zu spezifischen Abschnitten
Siehe den Abschnitt "[Datenmodellierung](#datenmodellierung)" für Details.

Oder: Siehe auch "[Performance-Optimierungen](#performance-optimierungen)" im Anhang.

Im Fließtext:
Diese Technik wird in "[Storage-Architektur](#storage-architecture)" ausführlich erläutert.
```

---

## Tabellen-Links

```markdown
# Tabellen-Referenzen
Die [Konfigurationstabelle](#table-chapter-01-1) zeigt alle verfügbaren Optionen.

Alternative:
Siehe [diese Vergleichstabelle](#table-chapter-05-2) für die Unterschiede zwischen 
relationalen und Graph-Datenmodellen.

Im Text:
Wie in [Tabelle 3](#table-chapter-02-3) gezeigt werden die Performance-Charakteristiken 
verschiedener Ansätze verglichen.
```

---

## Komplexe Querverweise

```markdown
# Mehrschichtige Navigation
1. Start mit [Kapitel 1 - Einführung](#chapter_01_introduction)
2. Studieren Sie [Abbildung 2](#diagram-2) für das Architektur-Diagramm
3. Referenzieren Sie [Tabelle 5](#table-chapter-02-5) für die Komponenten
4. Implementieren Sie basierend auf [Abschnitt 3.2](#section-32)
5. Testen Sie gegen [Benchmark-Metriken](#performance-benchmarks)
```

---

## Anchor-Namenskonventionen

| Element | Pattern | Beispiel |
|---------|---------|----------|
| Kapitel | `chapter_NN_*` | `#chapter_01_introduction` |
| Diagramm | `diagram-N` | `#diagram-42` |
| Tabelle | `table-*-N` | `#table-chapter-01-1` |
| Teil | `teil-*` | `#teil-i-grundlagen` |
| Überschrift | `lowercase-slug` | `#rocksdb-storage-architecture` |
| H2 Sektion | `h2-title-slug` | (auto-generiert) |
| H3 Untersektion | `h3-title-slug` | (auto-generiert) |

---

## Automatische Anker-Generierung

Das System generiert automatisch Anker für:

### Überschriften (h1-h6)
```markdown
## RocksDB Storage Architecture
# Automatisch wird: id="rocksdb-storage-architecture"
# Link: [Hier](#rocksdb-storage-architecture)
```

### Diagramme
```markdown
```mermaid
%% Figure-Title
graph TD
    A --> B
```
# Automatisch: id="diagram-42" (sequenziell nummeriert)
# Link: [Siehe Diagramm](#diagram-42)
```

### Tabellen
```markdown
| Col1 | Col2 |
|------|------|
| A    | B    |
# Automatisch: id="table-chapter-01-1" (chapter + counter)
# Link: [Siehe Tabelle](#table-chapter-01-1)
```

---

## Best Practices

### ✅ Gute Link-Praktiken
```markdown
✓ [Kapitel 5](#chapter_05_relational) für relationale Datenmodelle
✓ wie in [Abbildung 12](#diagram-12) gezeigt
✓ Die [Konfigurationstabelle](#table-chapter-02-3) enthält alle Optionen
✓ Für weitere Details siehe Abschnitt [Storage-Layer](#storage-layer-architecture)
```

### ❌ Vermeiden
```markdown
✗ Siehe Kapitel 5 (keine Link)
✗ [Link](chapter_05_relational.md) (Markdown-Link-Format)
✗ [Link](file:///chapter_05) (File-URL-Format)
✗ Abbildung 12 (keine Referenz)
```

---

## Navigation durchs Kompendium

### Startpunkte
- **Inhaltsverzeichnis:** [Kapitel-Übersicht](#toc)
- **Abbildungsverzeichnis:** [Alle Diagramme](#figure-index)
- **Umfassendes Index:** [Alle Anker (273)](#extended-index)

### Häufige Navigationspfade

**Für Anfänger:**
1. [Kapitel 0 - Genesis](#chapter_00_genesis) - Überblick
2. [Kapitel 1 - Einführung](#chapter_01_introduction) - Grundkonzepte
3. [Kapitel 2 - Architektur](#chapter_02_architecture) - System-Design
4. [Diagramm 1](#diagram-1) - Architektur-Übersicht

**Für Datenmodelle:**
1. [Kapitel 3 - Multimodal](#chapter_03_multimodel) - Konzepte
2. [Kapitel 5 - Relational](#chapter_05_relational) - SQL-ähnliche Modelle
3. [Kapitel 6 - Graph](#chapter_06_graph) - Graphdatenbanken
4. [Kapitel 7 - Document](#chapter_07_document) - JSON/Dokument-Modelle
5. [Vergleichstabelle](#table-chapter-03-2) - Ableiten

**Für Performance:**
1. [Kapitel 20 - Performance](#chapter_21_performance) - Tuning
2. [Kapitel 34 - Query-Optimierung](#chapter_34_query_optimization) - Advanced
3. [Performance-Benchmark](#diagram-99) - Metriken
4. [Tuning-Cookbook](#chapter_39_performance_tuning_cookbook) - Praktische Tipps

---

## Programmatischer Zugriff (für Tools)

Die Anchor-Registry steht als JSON verfügbar:

```bash
output/anchor-registry-v1.4.0.json
```

Beispiel-Abfrage:
```python
import json

with open('anchor-registry-v1.4.0.json', 'r') as f:
    registry = json.load(f)

# Alle Diagramme
diagrams = [e for e in registry['elements'] if e['type'] == 'figure']

# Alle Tabellen in Kapitel 1
tables_ch1 = [
    e for e in registry['elements'] 
    if e['type'] == 'table' and 'chapter_01' in e['source_file']
]

# Alle Überschriften Level 2
headings_h2 = [e for e in registry['elements'] if e['type'] == 'heading-2']

print(f"Total anchors: {registry['total_anchors']}")
print(f"Diagrams: {len(diagrams)}")
print(f"Tables: {len(tables_ch1)}")
```

---

## Troubleshooting

### Link funktioniert nicht
**Problem:** `[Link](#chapter_01_introduction)` funktioniert nicht im PDF

**Lösungen:**
1. Überprüfen Sie die Anker-ID gegen die Registry:
   ```bash
   grep "chapter_01" anchor-registry-v1.4.0.json
   ```

2. Nutzen Sie den Validator:
   ```bash
   python debug_anchors.py
   ```

3. Verwenden Sie die Extended Index Seite, um gültige Anker zu finden: [Index](#extended-index)

### Neue Überschriften werden nicht verlinkt
**Problem:** Neue Überschrift wird nicht mit Anker versehen

**Lösung:** Das System generiert automatisch - machen Sie einen neuen Build:
```bash
python step2_generate_html.py  # Liest .md Dateien, generiert Anker
```

### Diagramm-Link ist ungültig
**Problem:** `[Abb. 5](#diagram-5)` existiert nicht

**Lösungen:**
1. Überprüfen Sie die aktuelle Diagramm-Nummer:
   ```bash
   grep "diagram-" anchor-registry-v1.4.0.json | tail -10
   ```

2. Rekonstruieren Sie SVGs und HTML:
   ```bash
   python step1_generate_svgs.py
   python step2_generate_html.py
   ```

