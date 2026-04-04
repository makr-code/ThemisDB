# Pandoc + LaTeX PDF Generation Guide

## Warum Pandoc + LaTeX statt WeasyPrint?

### Vorteile von LaTeX

1. **Professionelle Typografie**
   - Automatische Silbentrennung
   - Optimaler Zeilenumbruch (line breaking algorithm)
   - Ligatures und Kerning
   - Microtype-Optimierungen

2. **Native Unterstützung für akademische Features**
   - Automatisches Inhaltsverzeichnis mit korrekten Seitenzahlen
   - Abbildungsverzeichnis (List of Figures)
   - Tabellenverzeichnis (List of Tables)
   - Stichwortverzeichnis (Index)
   - Bibliografien und Zitate

3. **Keine Performance-Probleme**
   - LaTeX ist für große Dokumente optimiert
   - Keine Grid/Flexbox/CSS-Variable-Probleme
   - Rendert 760-Seiten-Dokumente in wenigen Minuten

4. **PDF/A Unterstützung**
   - Langzeitarchivierung
   - Tagged PDFs für Barrierefreiheit

### Nachteile

1. **Installation größer**
   - TeX Live: ~1 GB
   - Aber nur einmalige Installation

2. **Weniger CSS-Kontrolle**
   - Layout wird durch LaTeX-Templates gesteuert
   - Aber: Templates sind anpassbar

## Installation

### Windows (WSL)

```bash
# In WSL Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y pandoc texlive-xetex texlive-lang-german texlive-fonts-recommended
```

### macOS

```bash
brew install pandoc
brew install --cask basictex
# Nach Installation:
sudo tlmgr update --self
sudo tlmgr install collection-xetex collection-langgerman
```

### Linux

```bash
# Ubuntu/Debian
sudo apt-get install pandoc texlive-xetex texlive-lang-german

# Fedora
sudo dnf install pandoc texlive-xetex texlive-babel-german

# Arch
sudo pacman -S pandoc texlive-core texlive-latexextra
```

## Verwendung

### Einfach: Mit Skript

```bash
python step3_generate_pdf_pandoc.py
```

Das Skript:
1. Kombiniert alle Markdown-Dateien in korrekter Reihenfolge
2. Fügt YAML-Metadaten hinzu (Titel, Autor, TOC-Konfiguration)
3. Ruft Pandoc mit XeLaTeX auf
4. Generiert PDF mit Inhaltsverzeichnis und Abbildungsverzeichnis

### Manuell: Direkt mit Pandoc

```bash
pandoc input.md -o output.pdf \
  --pdf-engine=xelatex \
  --toc \
  --toc-depth=3 \
  --list-of-figures \
  --number-sections \
  -V documentclass=book \
  -V papersize=a4 \
  -V fontsize=11pt \
  -V lang=de-DE
```

## PDF-Engines

Pandoc unterstützt mehrere LaTeX-Engines:

### 1. **pdflatex** (Standard)
```bash
--pdf-engine=pdflatex
```
- Schnellste Option
- Aber: Schlechte Unicode-Unterstützung
- Nur für rein ASCII-Dokumente geeignet

### 2. **XeLaTeX** (Empfohlen)
```bash
--pdf-engine=xelatex
```
- ✅ **Vollständige Unicode-Unterstützung**
- ✅ System-Fonts verwendbar
- ✅ Deutsche Umlaute kein Problem
- Etwas langsamer als pdflatex
- **Beste Wahl für das Kompendium**

### 3. **LuaLaTeX** (Modern)
```bash
--pdf-engine=lualatex
```
- Modernste Engine
- Lua-Skripting möglich
- Ähnlich wie XeLaTeX
- Manchmal langsamer

## YAML-Metadaten für PDF

Im Markdown-File oder als separate metadata.yaml:

```yaml
---
title: ThemisDB Kompendium
subtitle: Multi-Model Datenbank
author:
  - ThemisDB Team
  - Dr. Example Name
date: Januar 2026
keywords: [Datenbank, Multi-Model, AQL]

# Dokumentklasse
documentclass: book        # oder: report, article
papersize: a4
fontsize: 11pt            # oder: 10pt, 12pt

# Margins
geometry:
  - top=20mm
  - bottom=17mm
  - left=18mm
  - right=18mm

# Inhaltsverzeichnis
toc: true
toc-depth: 3              # Ebenen 1-3
lof: true                 # List of Figures
lot: false                # List of Tables (deaktiviert)

# Nummerierung
numbersections: true      # Kapitelnummerierung

# Links
colorlinks: true
linkcolor: blue
urlcolor: blue
citecolor: green

# Sprache
lang: de-DE               # Deutsche Silbentrennung

# Fonts (nur bei XeLaTeX/LuaLaTeX)
mainfont: DejaVu Serif
sansfont: DejaVu Sans
monofont: DejaVu Sans Mono

# Header/Footer
header-includes: |
  \usepackage{fancyhdr}
  \pagestyle{fancy}
  \fancyhead[LE,RO]{\thepage}
  \fancyhead[LO]{\rightmark}
  \fancyhead[RE]{\leftmark}
---
```

## Template-Anpassungen

### Eigenes LaTeX-Template

```bash
# Standard-Template exportieren
pandoc -D latex > custom-template.latex

# Template anpassen und verwenden
pandoc input.md -o output.pdf \
  --template=custom-template.latex \
  --pdf-engine=xelatex
```

### Häufige Anpassungen

**1. Chapter-Seiten immer rechts beginnen:**
```latex
\documentclass[openright]{book}
```

**2. Zwei-Seiten-Layout mit unterschiedlichen Margins:**
```latex
\usepackage[
  twoside,
  inner=25mm,
  outer=15mm,
  top=20mm,
  bottom=17mm
]{geometry}
```

**3. Fancy Headers:**
```latex
\usepackage{fancyhdr}
\pagestyle{fancy}
\fancyhf{}
\fancyhead[LE,RO]{\thepage}
\fancyhead[LO]{\nouppercase{\rightmark}}
\fancyhead[RE]{\nouppercase{\leftmark}}
```

**4. Custom Chapter-Titel:**
```latex
\usepackage{titlesec}
\titleformat{\chapter}[display]
  {\normalfont\huge\bfseries}{\chaptertitlename\ \thechapter}{20pt}{\Huge}
```

## Vergleich: WeasyPrint vs Pandoc+LaTeX

| Feature | WeasyPrint | Pandoc+LaTeX |
|---------|------------|--------------|
| **Performance** | ⚠️ Hängt bei komplexem CSS | ✅ Schnell (2-5 Min) |
| **Seitenzahlen in TOC** | ⚠️ target-counter() langsam | ✅ Native LaTeX-Features |
| **Typografie** | ⚠️ Browser-basiert | ✅ Professionell |
| **CSS Kontrolle** | ✅ Volle CSS3-Kontrolle | ⚠️ LaTeX-Templates |
| **Installation** | ✅ Klein (~50 MB) | ⚠️ Groß (~1 GB) |
| **Figuren/Tabellen** | ⚠️ Manuell mit CSS | ✅ Automatisch |
| **Mehrsprachig** | ✅ HTML lang-Attribut | ✅ Babel-Pakete |
| **PDF/A** | ✅ Experimentell | ✅ Production-ready |

## Best Practices

### 1. Markdown strukturieren

```markdown
# Chapter 1: Introduction {#intro}

Some text with reference to [Chapter 2](#data-models).

## Section 1.1

### Subsection 1.1.1
```

### 2. Bilder einbinden

```markdown
![Caption für Bild](images/architecture.png){width=80%}

<!-- Mit Referenz: -->
![Architektur Übersicht](images/arch.png){#fig:arch width=70%}

Siehe Abbildung @fig:arch für Details.
```

### 3. Tabellen

```markdown
| Feature | Status |
|---------|--------|
| Multi-Model | ✅ |
| ACID | ✅ |

: Übersicht der Features {#tbl:features}
```

### 4. Code-Blöcke

````markdown
```javascript
// Code mit Syntax-Highlighting
const db = require('arangojs');
```
````

### 5. Mathematische Formeln

```markdown
Inline: $E = mc^2$

Display:
$$
\sum_{i=1}^{n} x_i = \frac{n(n+1)}{2}
$$
```

## Troubleshooting

### Problem: "! LaTeX Error: File 'xyz.sty' not found"

**Lösung:** Fehlende LaTeX-Pakete installieren

```bash
# Ubuntu/Debian
sudo apt-get install texlive-latex-extra texlive-fonts-extra

# Oder manuell:
sudo tlmgr install xyz
```

### Problem: "Unicode char not set up for use with LaTeX"

**Lösung:** XeLaTeX statt pdfLaTeX verwenden

```bash
pandoc input.md -o output.pdf --pdf-engine=xelatex
```

### Problem: Bilder werden nicht gefunden

**Lösung:** Absolute Pfade oder --resource-path

```bash
pandoc input.md -o output.pdf \
  --resource-path=.:images:../images
```

### Problem: Deutsche Silbentrennung funktioniert nicht

**Lösung:** Lang-Variable setzen

```yaml
---
lang: de-DE
---
```

Oder:

```bash
pandoc input.md -o output.pdf -V lang=de-DE
```

## Performance-Tipps

1. **Bilder optimieren**
   - PNG für Screenshots (mit pngcrush komprimieren)
   - JPEG für Fotos
   - SVG für Diagramme → PDF konvertieren

2. **Große Dokumente splitten**
   - Jedes Kapitel als separate Datei
   - Pandoc kombiniert automatisch

3. **Cache nutzen**
   - LaTeX cached `.aux`, `.toc` Dateien
   - Zweiter Lauf ist schneller

4. **Fonts reduzieren**
   - Nur notwendige Fonts einbinden
   - Standard-Fonts sind schneller

## Nächste Schritte

1. ✅ Pandoc + LaTeX installieren (läuft...)
2. ⏳ Test-PDF generieren
3. Template anpassen (optional)
4. Bookmarks hinzufügen (step4)
5. Finales PDF erstellen
