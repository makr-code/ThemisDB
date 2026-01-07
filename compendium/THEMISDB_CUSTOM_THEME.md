# ThemisDB Custom Theme

## Übersicht

Das **ThemisDB Corporate Theme** ist ein custom Material Design-basiertes Theme, das speziell für das ThemisDB Kompendium entwickelt wurde. Es kombiniert die ThemisDB-Markenfarben mit professioneller Buch-Typografie.

## Theme-Eigenschaften

### Farbschema

**Primärfarben (aus ThemisDB-Branding):**
- **Primary:** `#1a4d2e` (Dark Green) - Hauptfarbe für Überschriften, Hervorhebungen
- **Secondary:** `#0f3d5c` (Dark Blue) - Akzentfarbe für Unterüberschriften
- **Accent:** `#2a7f62` (Medium Green) - Zusätzliche Akzente, Links

**Textfarben:**
- **Text:** `#2c3e50` (Dunkelgrau) - Optimale Lesbarkeit
- **Background:** `#ffffff` (Weiß) - Sauberer Hintergrund
- **Code Background:** `#f0f7f4` (Heller Grün-Ton) - Subtile Hervorhebung

### Typografie

**Schriftarten:**
- **Überschriften:** Helvetica Neue, Arial, sans-serif
- **Fließtext:** Georgia, Times New Roman, serif (optimal für Buchdruck)
- **Code:** Fira Code, Courier New, monospace

**Größen:**
- H1: 28pt (Chapter titles)
- H2: 20pt (Sections)
- H3: 16pt (Subsections)
- Body: 11pt
- Code: 9.5-10pt

### Design-Merkmale

1. **Gradient Headers:**
   - Überschriften mit 3px Unterstrich in Primary-Farbe
   - H2 mit 2px Unterstrich in Accent-Farbe
   - Professionelle Hierarchie

2. **Mermaid-Diagramme:**
   - Gradient-Header (Dark Green → Dark Blue)
   - Heller grüner Hintergrund
   - Abgerundete Ecken (8px border-radius)
   - Box-Shadow für Tiefe

3. **Code-Blöcke:**
   - Grün-getönter Hintergrund
   - 4px linker Border in Primary-Farbe (Dark Green)
   - Border-radius für moderne Optik

4. **Tabellen:**
   - Gradient-Header (Dark Green → Dark Blue)
   - Abwechselnde Zeilenfarben
   - Professionelles Layout

## Vergleich: ThemisDB vs. Material Design

| Eigenschaft | ThemisDB Corporate | Material Standard |
|-------------|-------------------|-------------------|
| **Primärfarbe** | `#1a4d2e` (Dark Green) | `#7c4dff` (Purple) |
| **Sekundärfarbe** | `#0f3d5c` (Dark Blue) | `#ff4081` (Pink) |
| **Akzentfarbe** | `#2a7f62` (Medium Green) | `#536dfe` (Indigo) |
| **Code-Hintergrund** | `#f0f7f4` (Grün-Ton) | `#f5f5f5` (Grau) |
| **Markenidentität** | ✅ ThemisDB Dark Green/Blue | ❌ Generisch |
| **Farbharmonie** | ✅ Green-Blue Harmonie | ⚠️ Purple-Pink Kontrast |

## Verwendung

### PDF generieren

```bash
cd docs/compendium
python3 generate_themed_pdfs.py

# Nur ThemisDB Theme:
python3 -c "from generate_themed_pdfs import generate_themed_pdf; generate_themed_pdf('themisdb')"
```

### Ausgabe

**Datei:** `pdf_output/themes/ThemisDB-Kompendium-themisdb-YYYYMMDD.pdf`

**Größe:** ~1.5 MB

**Inhalt:**
- 44 Mermaid-Diagramme in ThemisDB-Farbschema
- 18 Kapitel mit konsistenter Typografie
- Professionelles Layout für Druck und Digital

## Anpassung

### Farben ändern

In `generate_themed_pdfs.py`:

```python
"themisdb": {
    "name": "ThemisDB Corporate",
    "primary": "#7c4dff",    # Ihre Primary-Farbe
    "secondary": "#43e97b",  # Ihre Secondary-Farbe
    "accent": "#4facfe",     # Ihre Accent-Farbe
    "text": "#2c3e50",
    "background": "#ffffff",
    "code_bg": "#f5f3ff",
    # ...
}
```

### Schriftarten ändern

```python
"themisdb": {
    # ...
    "heading_font": '"Your Heading Font", Arial, sans-serif',
    "body_font": '"Your Body Font", Georgia, serif',
    "code_font": '"Your Code Font", monospace'
}
```

### Weitere Anpassungen

**Abstände:**
- Seitenränder: 25mm oben/unten, 20mm links/rechts (in `@page`)
- Zeilenhöhe: 1.65 für optimale Lesbarkeit

**Seitenelemente:**
- Header: Theme-Name und Version
- Footer: Seitenzahlen

## Best Practices

### Für Druck

- ✅ Verwenden Sie Georgia für Fließtext (Serifenschrift, bessere Lesbarkeit)
- ✅ 11pt Schriftgröße für A4-Format
- ✅ 1.65 Zeilenhöhe für angenehmes Lesen
- ✅ Ausreichende Ränder (25mm) für Bindung

### Für Digital

- ✅ PDF-Lesezeichen für Navigation
- ✅ Interne Links funktionsfähig
- ✅ Code-Blöcke mit Syntax-Highlighting
- ✅ Mermaid-Diagramme klar lesbar

### Für Markenkonformität

- ✅ ThemisDB-Farbpalette durchgehend verwendet
- ✅ Logo-Platzierung auf Titelseite (implementierbar)
- ✅ Konsistente Typografie
- ✅ Professionelles Erscheinungsbild

## Vergleich mit anderen Book-Themes

| Theme | Primärfarbe | Best For | Stil |
|-------|-------------|----------|------|
| **ThemisDB** | Purple | **ThemisDB Branding** | **Modern Corporate** |
| O'Reilly | Orange-Red | Tech Books | Classic Tech |
| Springer | Dark Blue | Academic | Scientific |
| Manning | Red | Programming | Traditional |
| Pragmatic | Orange | Agile/Dev | Contemporary |

## Beispiele

### Kapitel-Überschrift
```
# Chapter 1: Introduction    ← 28pt, Purple, 3px Unterstrich
```

### Mermaid-Diagramm
```
┌─────────────────────────────────────┐
│ 📊 Flowchart #1                     │ ← Gradient Header (Purple→Blue)
├─────────────────────────────────────┤
│                                     │
│   graph TB                          │ ← Code auf lila Hintergrund
│       A --> B                       │
│                                     │
├─────────────────────────────────────┤
│ Hinweis: Online interaktiv         │ ← Footer
└─────────────────────────────────────┘
```

### Code-Block
```python
def example():
    return "Hello ThemisDB"
```
← Lila Hintergrund (#f5f3ff), linker Purple Border

## Roadmap

**Zukünftige Erweiterungen:**

- [ ] Logo-Integration auf Titelseite
- [ ] Kapitel-Start mit Farb-Seitenbalken
- [ ] Seitennummerierung mit Theme-Farbe
- [ ] Inhaltsverzeichnis mit Hyperlinks
- [ ] Index am Ende
- [ ] Glossar-Sektion mit Theme-Styling

## Technische Details

**Generierung:**
- Engine: WeasyPrint 
- Format: A4 (210mm × 297mm)
- Auflösung: 96 DPI
- Farbprofil: sRGB
- Kompression: Standard PDF

**Kompatibilität:**
- Adobe Acrobat Reader: ✅
- Browser (Chrome, Firefox, Safari): ✅
- E-Reader (Kindle, Kobo): ✅
- Mobile (iOS, Android): ✅

---

**Version:** 1.0  
**Erstellt:** 2025-12-31  
**Autor:** ThemisDB Documentation Team
