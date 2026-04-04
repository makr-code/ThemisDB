# ThemisDB Kompendium - Umfassendes Anchor & Link-System

## Übersicht

Das neue Anchor-System hat die Linkauflösung fundamental überarbeitet. **Jedes Element** im Kompendium erhält eine eindeutige Anker-ID:

| Element-Typ | Anzahl | ID-Format | Beispiel |
|---|---|---|---|
| **Kapitel** | 52 | `chapter_XX_*` | `chapter_01_introduction` |
| **Diagramme** | 112 | `diagram-N` | `diagram-1`, `diagram-42` |
| **Tabellen** | 98 | `table-*-N` | `table-chapter-01-1` |
| **Teile/Sections** | 11 | `teil-X-*` | `teil-i-grundlagen` |
| **Überschriften** | Alle (h1-h6) | `slug-from-title` | `rocksdb-storage-architecture` |

**Gesamt: 273 eindeutige Anker**

---

## Ankors registrieren

Das System registriert beim Build automatisch alle Anker in der globalen Registry (`ANCHOR_REGISTRY`):

```python
# Struktur der Registry
ANCHOR_REGISTRY = {
    'elements': [
        {
            'id': 'diagram-1',
            'title': 'Abb. 1: ThemisDB Architecture Overview',
            'type': 'figure',
            'source_file': 'chapter_01_introduction',
            'page': 15,
            'parent': 'chapter_01_introduction',
            'timestamp': '2026-01-13T11:48:20'
        },
        # ... weitere 272 Anker
    ],
    'by_id': {...},        # Schnelle Lookups nach Anker-ID
    'by_file': {...}       # Schnelle Lookups nach Quell-Datei
}
```

### Anchor-Registrierung beim Build

**Teile (Parts):**
```python
section_anchor = slugify_text(item['title'])  # "Teil I" → "teil-i"
register_anchor(
    section_anchor,
    item['title'],
    'part',
    parent='root'
)
```

**Kapitel:**
```python
anchor = item['file'].replace('.md', '').replace('/', '-')
register_anchor(
    anchor,
    item['title'],
    'chapter',
    anchor,
    parent=f"part-{part_number}"
)
```

**Diagramme:**
```python
diagram_id = f"diagram-{diagram_counter}"
register_anchor(
    diagram_id, 
    f"Abb. {diagram_counter}: {diagram_title}",
    'figure',
    source_file,
    parent=f"section_{slugify_text(source_file)}"
)
```

**Überschriften (automatisch während Markdown-Verarbeitung):**
```python
def add_heading_anchors(html_content: str, source_file: str) -> str:
    """Fügt automatisch Anker-IDs zu allen h1-h6 hinzu"""
    # Extrahiert Text aus <h1>Title</h1>
    # Generiert Slug: "RocksDB Storage Architecture" → "rocksdb-storage-architecture"
    # Registriert: register_anchor(anchor, title, 'heading-N', source_file)
    # Outputs: <h1 id="rocksdb-storage-architecture">Title</h1>
```

**Tabellen (automatisch während Markdown-Verarbeitung):**
```python
def add_table_anchors(html_content: str, source_file: str) -> str:
    """Fügt automatisch Anker-IDs zu allen Tabellen hinzu"""
    # Erstellt ID: table-{source_file}-{counter}
    # Beispiel: table-chapter-01-1
    # Registriert und fügt zu <table id="..."> hinzu
```

---

## HTML-Integration

### Kapitel-Markup
```html
<div id="chapter_01_introduction" class="chapter">
    <h1 class="chapter-title">Kapitel 1: Einführung</h1>
    <!-- Inhalt mit automatisch nummerierten h2-h6 mit IDs -->
    <h2 id="fundamentals">1.1 Fundamentals</h2>
    <h3 id="architecture-overview">1.1.1 Architecture Overview</h3>
</div>
```

### Diagramm-Markup
```html
<figure id="diagram-1" style="...">
    <img src="..." alt="...">
    <figcaption>Abb. 1: Architecture</figcaption>
</figure>
```

### Tabellen-Markup
```html
<table id="table-chapter-01-1">
    <thead>...</thead>
    <tbody>...</tbody>
</table>
```

---

## Links im Fließtext & Navigation

### Automatische Linkkonvertierung

Die Funktion `convert_internal_links()` konvertiert alle verschiedenen Linkformate in Anker-Links:

```python
# Markdown-Links: [Kapitel 1](chapter_01_introduction.md)
# → <a href="#chapter_01_introduction">Kapitel 1</a>

# File-URLs: <a href="file:///chapter_01_introduction">
# → <a href="#chapter_01_introduction">

# Relative Links: <a href="./chapter_01_introduction">
# → <a href="#chapter_01_introduction">
```

### Inhaltsverzeichnis (TOC)

```html
<!-- Automatisch generiert mit Anker-Links -->
<div class="toc-section">
    <h1 id="toc">Inhaltsverzeichnis</h1>
    <ul class="toc-list">
        <li class="toc-item">
            <a href="#chapter_01_introduction">Kapitel 1 - Einführung</a>
            <span class="toc-page-num">5</span>
        </li>
        <!-- ... weitere 51 Kapitel -->
    </ul>
</div>
```

### Abbildungsverzeichnis (Figure Index)

```html
<!-- Automatisch generiert mit Anker-Links zu Diagrammen -->
<table class="figure-table">
    <tbody>
        <tr>
            <td class="fig-num"><a href="#diagram-1">Abb. 1</a></td>
            <td class="fig-desc">ThemisDB Architecture Overview</td>
            <td class="fig-page">15</td>
        </tr>
        <!-- ... weitere 111 Diagramme -->
    </tbody>
</table>
```

### Erweiterter Index (Extended Index & Anchor Registry)

```html
<!-- Neue Seite: Umfassendes Index und Anker-Verzeichnis -->
<div class="extended-index">
    <h1 id="extended-index">Umfassendes Index und Anker-Verzeichnis</h1>
    
    <!-- Tabelle aller 273 Anker nach Typ sortiert -->
    <div class="index-section">
        <h2>Kapitel (Chapters) (52)</h2>
        <table class="index-table">
            <thead>
                <tr>
                    <th>Anker-ID</th>
                    <th>Titel</th>
                    <th>Quelle</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><code>chapter_01_introduction</code></td>
                    <td><a href="#chapter_01_introduction">Kapitel 1 - Einführung</a></td>
                    <td>chapter_01_introduction</td>
                </tr>
                <!-- ... weitere 51 Kapitel -->
            </tbody>
        </table>
    </div>
    
    <!-- Ähnlich für Diagramme, Tabellen, Überschriften usw. -->
</div>
```

---

## Anchor-Registry Datei

Während des Builds wird die vollständige Registry als JSON exportiert:

**Datei:** `anchor-registry-v1.4.0.json`

```json
{
  "version": "v1.4.0",
  "generated": "2026-01-13T11:48:20",
  "total_anchors": 273,
  "by_type": {
    "chapter": 52,
    "figure": 112,
    "part": 11,
    "table": 98,
    "heading-1": 0,
    "heading-2": 45,
    "heading-3": 78,
    "heading-4": 89,
    "heading-5": 12,
    "heading-6": 5
  },
  "elements": [
    {
      "id": "chapter_01_introduction",
      "title": "Kapitel 1 - Einführung",
      "type": "chapter",
      "source_file": "chapter_01_introduction",
      "page": 5,
      "parent": "part-1",
      "timestamp": "2026-01-13T11:48:20"
    },
    // ... weitere Anker
  ]
}
```

---

## Link-Funktionalität im PDF

### Funktioniert ✅
- **Kapitiel-Links:** TOC → Kapitel (z.B. `#chapter_01_introduction`)
- **Diagramm-Links:** Abbildungsverzeichnis → Diagramme (z.B. `#diagram-42`)
- **Tabellen-Links:** Index → Tabellen (z.B. `#table-chapter-01-1`)
- **Überschrifts-Links:** Extended Index → Überschriften (z.B. `#rocksdb-storage-architecture`)
- **Querverweis-Links:** Im Fließtext (z.B. `[Siehe Kapitel 5](#chapter_05_graph)`)

### Verwendungsbeispiele im Markdown

```markdown
# Kapitel 1 - Einführung

Siehe auch [Kapitel 5 - Graph-Datenmodell](#chapter_05_graph) für Details.

## Architektur

Wie in [Abbildung 1](#diagram-1) gezeigt...

### Datenstrukturen

Die folgende [Tabelle](#table-chapter-01-1) zeigt...
```

---

## Build-Prozess

### Step 1: SVG-Generierung (unverändert)
```bash
python step1_generate_svgs.py
# → 114/114 SVG-Diagramme
```

### Step 2: HTML-Generierung (mit Anchors)
```bash
python step2_generate_html.py
# → Anchor-Registrierung (273 Anker)
# → anchor-registry-v1.4.0.json (Referenzdatei)
# → ThemisDB-Kompendium-v1.4.0.html
#   - Mit allen Anker-IDs in divs, h1-h6, figures, tables
#   - Mit automatisch generierten TOC, Figure Index, Extended Index
#   - Mit vollständiger Link-Konvertierung
```

### Step 3: PDF-Generierung (unverändert)
```bash
python step3_generate_pdf.py
# → 2.85 MB PDF mit funktionierenden Links
```

### Step 4: Bookmark-Hinzufügung (unverändert)
```bash
python step4_add_bookmarks.py
# → PDF mit 64 hierarchischen Bookmarks
```

---

## Wichtige Funktionen

### `slugify_text(text: str, max_length: int = 60) -> str`
Konvertiert Text in URL-sichere Slugs:
```python
"RocksDB Storage Architecture" → "rocksdb-storage-architecture"
"Teil I - Grundlagen" → "teil-i-grundlagen"
```

### `register_anchor(anchor_id, title, element_type, source_file, page_num, parent)`
Registriert einen Anker in der globalen Registry:
```python
register_anchor(
    'diagram-1',
    'Abb. 1: Architecture Overview',
    'figure',
    'chapter_01_introduction',
    parent='chapter_01_introduction'
)
```

### `add_heading_anchors(html_content: str, source_file: str) -> str`
Fügt automatisch Anker-IDs zu allen Überschriften hinzu und registriert sie:
```html
Input:  <h2>RocksDB Storage Architecture</h2>
Output: <h2 id="rocksdb-storage-architecture">RocksDB Storage Architecture</h2>
```

### `add_table_anchors(html_content: str, source_file: str) -> str`
Fügt automatisch Anker-IDs zu allen Tabellen hinzu:
```html
Input:  <table>...</table>
Output: <table id="table-chapter-01-1">...</table>
```

### `save_anchor_registry()`
Speichert die komplette Registry als JSON für externe Tools:
```bash
output/anchor-registry-v1.4.0.json
```

### `generate_extended_index(nav_items: List[Dict]) -> str`
Generiert eine umfassende Index-Seite mit allen 273 Ankern sortiert nach Typ:
- Kapitel (52)
- Diagramme (112)
- Tabellen (98)
- Teile/Sections (11)
- Überschriften aller Ebenen (57+)

---

## Validierung & Debugging

### Anchor Consistency Validator
```bash
python debug_anchors.py
```

Prüft:
- Anzahl der registrierten Anker-IDs im HTML
- Übereinstimmung zwischen TOC-Links und existierenden Elementen
- Fehlende oder nicht verlinkte Anker

**Ausgabe:**
```
[CHAPTERS] Found 52 chapter divs with IDs
[FIGURES] Found 112 figure elements with IDs
[TABLES] Found 98 table elements with IDs
[TOC LINKS] Found 52 links to chapters (all valid)
[FIGURE LINKS] Found 112 links to figures (all valid)
[STATUS] ✓ OK - Anchor mapping is consistent
```

---

## Zusammenfassung

Das neue Anchor-System bietet:

✅ **Vollständige Abdeckung:** Jedes Element (Kapitel, Überschrift, Diagramm, Tabelle, Sektion) hat einen eindeutigen Anker
✅ **Automatische Registrierung:** Alle Anker werden während des Builds registriert
✅ **Konsistente IDs:** Gleiche Generierungslogik überall (slugify_text)
✅ **Mehrfache Navigation:** TOC, Figure Index, Extended Index alle mit funktionierenden Links
✅ **Referenzdatei:** anchor-registry-v1.4.0.json für externe Tools
✅ **PDF-Kompatibilität:** Alle Links funktionieren im generierten PDF
✅ **Fließtext-Integration:** Links können im Markdown-Inhalt verwendet werden

