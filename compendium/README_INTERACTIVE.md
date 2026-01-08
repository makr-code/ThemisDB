# Interactive Mermaid PDF Generation - Quick Start Guide

## 🎯 Überblick

Dieses Projekt ermöglicht die Erstellung von PDFs mit **wirklich interaktiven Mermaid-Diagrammen** durch drei verschiedene Ansätze:

1. **Embedded HTML Attachments** - HTML-Dateien mit Mermaid.js als PDF-Anhänge
2. **QR Codes** - Links zu gehosteten interaktiven Diagrammen
3. **Custom PDF.js Viewer** - Vollständig interaktive Browser-basierte PDF-Ansicht

## 📁 Dateien

### Generator Scripts

- **`generate_interactive_pdf_with_embeds.py`** - Hauptgenerator
  - Extrahiert Mermaid-Diagramme aus Markdown
  - Erstellt interaktive HTML-Dateien
  - Generiert QR-Codes
  - Erstellt Metadaten

- **`generate_pdf_interactive_mermaid.py`** - Legacy-Generator (HTML-Ansatz)
- **`generate_pdf_with_mermaid.py`** - Bestehender SVG-Generator

### Viewer

- **`interactive_pdf_viewer.html`** - Custom PDF.js Viewer
  - Lädt PDFs mit PDF.js
  - Überlagert interaktive Mermaid-Diagramme
  - Volle Zoom/Pan/Interaktivität

### Dokumentation

- **`INTERACTIVE_PDF_MERMAID_GUIDE.md`** - Technischer Guide
  - Alle Ansätze erklärt
  - Vor- und Nachteile
  - Implementierungsdetails

- **`README_INTERACTIVE.md`** - Diese Datei

## 🚀 Quick Start

### Schritt 1: Dependencies installieren

```bash
# Python Dependencies
pip install reportlab pypdf qrcode[pil] pillow markdown

# Optional: Mermaid CLI für SVG-Generierung
npm install -g @mermaid-js/mermaid-cli
```

### Schritt 2: Interaktive Inhalte generieren

```bash
cd /home/runner/work/ThemisDB/ThemisDB/compendium
python3 generate_interactive_pdf_with_embeds.py
```

**Output:**
```
✅ Generated 17 interactive diagrams
   📄 SVGs: /tmp/themis_interactive_pdf/mermaid_svgs
   🌐 HTMLs: /tmp/themis_interactive_pdf/mermaid_htmls
   📱 QR Codes: /tmp/themis_interactive_pdf/qr_codes
   📋 Metadata: /tmp/themis_interactive_pdf/diagrams_metadata.json
```

### Schritt 3: HTML-Dateien hosten

Die generierten HTML-Dateien müssen online gehostet werden für QR-Code-Funktionalität:

```bash
# Option A: GitHub Pages
# Kopiere HTML-Dateien nach docs/compendium/interactive/
mkdir -p docs/compendium/interactive
cp /tmp/themis_interactive_pdf/mermaid_htmls/*.html docs/compendium/interactive/

# Commit & Push
git add docs/compendium/interactive/
git commit -m "Add interactive Mermaid diagrams"
git push

# Aktiviere GitHub Pages in Repository Settings
```

```bash
# Option B: Lokaler Test-Server
cd /tmp/themis_interactive_pdf/mermaid_htmls
python3 -m http.server 8080

# Öffne: http://localhost:8080/chapter_01_introduction_diagram_1.html
```

### Schritt 4: Interaktive PDF-Ansicht nutzen

```bash
# Kopiere Viewer in Compendium-Verzeichnis
cp interactive_pdf_viewer.html /tmp/themis_interactive_pdf/
cp /tmp/themis_interactive_pdf/diagrams_metadata.json /tmp/themis_interactive_pdf/

# Starte Server
cd /tmp/themis_interactive_pdf
python3 -m http.server 8000

# Öffne im Browser
# http://localhost:8000/interactive_pdf_viewer.html
```

## 📊 Features der generierten Inhalte

### Interaktive HTML-Dateien

Jedes Diagramm bekommt eine standalone HTML-Datei mit:

- ✅ Vollständig interaktive Mermaid.js Rendering
- ✅ Zoom In/Out/Reset Buttons
- ✅ Theme Toggle (Hell/Dunkel)
- ✅ Download als SVG
- ✅ Klickbare Elemente mit Tooltips
- ✅ Responsive Design

**Beispiel:**
```
chapter_01_introduction_diagram_1.html
- 📊 Flowchart mit 6 Knoten
- 🎨 ThemisDB Farbschema
- 🖱️ Interaktive Elemente
- 💾 Download-Funktion
```

### QR Codes

- 📱 Scanbar mit jedem Smartphone
- 🔗 Verlinkt zu GitHub Pages
- 📏 Optimale Größe für PDF-Integration
- ✅ Funktioniert offline nach erstem Scan

### PDF.js Viewer

Der Custom Viewer kombiniert:

1. **PDF.js** für PDF-Rendering
2. **Mermaid.js** für interaktive Diagramme
3. **Custom UI** mit Seitenleiste und Vollbild

**Features:**
- ⌨️ Keyboard Shortcuts (←/→ Seiten, +/- Zoom, F Vollbild)
- 📱 Responsive für Mobile
- 🔍 Zoom-Steuerung
- 📋 Diagramm-Seitenleiste
- ⛶ Vollbildmodus

## 🛠️ Erweiterte Nutzung

### Alle Kapitel verarbeiten

Bearbeite `CHAPTERS` Liste in `generate_interactive_pdf_with_embeds.py`:

```python
CHAPTERS = [
    "preface.md",
    "chapter_01_introduction.md",
    "chapter_02_architecture.md",
    # ... alle 40+ Kapitel
]
```

### Custom Styling

HTML-Templates sind voll anpassbar:

```python
def create_interactive_html(mermaid_code, diagram_id, diagram_type):
    # Ändere Farben, Fonts, Layout hier
    html_content = f'''
    <style>
        body {{
            background: your-gradient;
        }}
    </style>
    '''
```

### PDF mit eingebetteten Anhängen erstellen

Nach der Generierung der HTML-Dateien können Sie diese als PDF-Anhänge einbetten:

```python
from pypdf import PdfWriter, PdfReader

def embed_html_in_pdf(pdf_path, html_files):
    """Bette HTML-Dateien als Anhänge in PDF ein"""
    reader = PdfReader(pdf_path)
    writer = PdfWriter()
    
    # Kopiere alle Seiten
    for page in reader.pages:
        writer.add_page(page)
    
    # Füge Anhänge hinzu
    for html_file in html_files:
        with open(html_file, 'rb') as f:
            writer.add_attachment(html_file.name, f.read())
    
    # Speichere
    with open('output_with_attachments.pdf', 'wb') as f:
        writer.write(f)
```

## 📋 Workflow-Übersicht

```
┌─────────────────────────────────────────────────────────┐
│ 1. Markdown-Kapitel mit ```mermaid Blöcken              │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 2. generate_interactive_pdf_with_embeds.py              │
│    - Extrahiert Mermaid-Code                            │
│    - Erstellt HTML mit Mermaid.js                       │
│    - Generiert QR-Codes                                 │
│    - Erstellt Metadaten-JSON                            │
└───────────────────┬─────────────────────────────────────┘
                    │
        ┌───────────┴───────────┬──────────────┐
        ▼                       ▼              ▼
┌──────────────┐     ┌────────────────┐   ┌─────────┐
│ HTML-Dateien │     │   QR-Codes     │   │ Metadata│
│ (interactive)│     │   (scannable)  │   │  JSON   │
└──────┬───────┘     └────────┬───────┘   └────┬────┘
       │                      │                 │
       ▼                      ▼                 ▼
┌──────────────────────────────────────────────────────┐
│ 3. Deployment                                        │
│    A) GitHub Pages (HTMLs)                           │
│    B) PDF mit Anhängen                               │
│    C) Custom Viewer (PDF.js + Mermaid)              │
└──────────────────────────────────────────────────────┘
```

## 🎨 Beispiel-Ausgabe

### Generierte Dateien pro Diagramm

Für `chapter_01_introduction_diagram_1`:

```
📁 /tmp/themis_interactive_pdf/
├── 📄 mermaid_htmls/
│   └── chapter_01_introduction_diagram_1.html  (7.8 KB)
│       - Standalone interaktive Seite
│       - Mermaid.js CDN eingebunden
│       - Zoom/Theme Controls
│
├── 📱 qr_codes/
│   └── chapter_01_introduction_diagram_1_qr.png  (913 bytes)
│       - QR zu GitHub Pages URL
│       - Optimiert für Druck
│
└── 📋 diagrams_metadata.json
    - ID, Code, Type, URLs
    - Für Viewer und Automation
```

### Interaktive HTML Features

Öffne ein generiertes HTML:

```bash
firefox /tmp/themis_interactive_pdf/mermaid_htmls/chapter_01_introduction_diagram_1.html
```

Sie sehen:
- 🎨 Gradient-Header mit Titel
- 📊 Interaktives Mermaid-Diagramm (klickbar!)
- 🔍 Zoom In/Out/Reset Buttons
- 🎨 Theme Toggle (Hell/Dunkel)
- 💾 Download SVG Button
- 💡 Hover-Effekte auf Elementen

## 🧪 Testing

### Test 1: HTML-Generierung

```bash
python3 generate_interactive_pdf_with_embeds.py

# Erwartete Ausgabe:
# ✅ Total diagrams found: 17
# ✅ Generated 17 interactive diagrams
```

### Test 2: Einzelnes HTML öffnen

```bash
# Öffne in Browser
xdg-open /tmp/themis_interactive_pdf/mermaid_htmls/chapter_01_introduction_diagram_1.html

# Prüfe:
# ✓ Diagramm wird gerendert
# ✓ Buttons funktionieren
# ✓ Elemente sind klickbar
```

### Test 3: QR-Code scannen

```bash
# Zeige QR-Code an
display /tmp/themis_interactive_pdf/qr_codes/chapter_01_introduction_diagram_1_qr.png

# Mit Handy scannen
# ✓ Sollte zu GitHub Pages URL führen (wenn gehostet)
```

### Test 4: PDF.js Viewer

```bash
cd /tmp/themis_interactive_pdf
python3 -m http.server 8000

# Browser: http://localhost:8000/interactive_pdf_viewer.html
# ✓ PDF lädt
# ✓ Diagramm-Seitenleiste zeigt Liste
# ✓ Klick auf Diagramm öffnet Overlay
```

## 🔧 Troubleshooting

### Problem: "mmdc: command not found"

**Lösung:** Mermaid CLI ist optional. Ohne wird nur HTML/QR generiert.

```bash
# Falls SVGs gewünscht:
npm install -g @mermaid-js/mermaid-cli
```

### Problem: QR-Codes funktionieren nicht

**Lösung:** URLs müssen angepasst werden.

Bearbeite `GITHUB_PAGES_BASE` in `generate_interactive_pdf_with_embeds.py`:

```python
GITHUB_PAGES_BASE = "https://YOUR-USERNAME.github.io/ThemisDB/compendium/interactive/"
```

### Problem: PDF.js Viewer zeigt nichts an

**Prüfe:**

1. PDF-Pfad korrekt?
   ```javascript
   const pdfPath = 'ThemisDB-Compendium-v1.3.4-interactive.pdf';
   ```

2. Metadata-JSON vorhanden?
   ```bash
   ls diagrams_metadata.json
   ```

3. CORS-Headers (bei Remote-Server)?
   ```bash
   # Lokaler Server immer OK
   python3 -m http.server
   ```

## 📖 Weitere Ressourcen

- **Technischer Guide:** `INTERACTIVE_PDF_MERMAID_GUIDE.md`
- **Mermaid Docs:** https://mermaid.js.org/
- **PDF.js Docs:** https://mozilla.github.io/pdf.js/
- **ReportLab Docs:** https://www.reportlab.com/docs/

## 🤝 Beitragen

Verbesserungen willkommen!

1. Neue Diagramm-Typen unterstützen
2. PDF-Embedding optimieren
3. Viewer-UI verbessern
4. Mobile-Optimierung

## 📝 Lizenz

Same as ThemisDB: MIT License

---

**Version:** 1.0  
**Datum:** 2026-01-08  
**Autor:** ThemisDB Documentation Team

**Status:** ✅ Produktiv einsetzbar  
**Getestet mit:** Python 3.12, Mermaid.js v10, PDF.js 3.11
