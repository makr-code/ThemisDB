# PDF Generation - Quick Reference

## Schnellstart

### Professionelles PDF generieren (step1-5 Pipeline)
```bash
cd compendium

# Vollständige Pipeline
python3 step1_generate_svgs.py      # SVGs aus Mermaid-Diagrammen
python3 step2_generate_html.py      # HTML mit professionellem Layout
python3 step3_generate_pdf.py       # PDF generieren
python3 step4_add_bookmarks.py      # PDF-Lesezeichen
python3 step5_cleanup.py            # Aufräumen
```

**Ausgabe:** `output/ThemisDB-Kompendium-{VERSION}.pdf`

### Was du bekommst

✅ **Durchgehende Seitennummerierung**  
✅ **Seitenzahlen im Inhaltsverzeichnis**  
✅ **Keine abgeschnittenen Kapitel/Absätze**  
✅ **Running Headers mit Buchtitel**  
✅ **Professionelle Typografie**  
✅ **Automatische Silbentrennung**  

## Voraussetzungen

```bash
# Python-Pakete installieren
pip install -r requirements.txt

# WeasyPrint sollte enthalten sein
weasyprint --version
```

Falls WeasyPrint fehlt:
```bash
pip install weasyprint
```

## Pipeline-Schritte

| Script | Beschreibung | Funktion |
|--------|--------------|----------|
| **step1_generate_svgs.py** | Mermaid → SVG | Konvertiert Diagramme zu Vektorgrafiken |
| **step2_generate_html.py** | Markdown → HTML | 🌟 Mit professionellem Buchlayout |
| **step3_generate_pdf.py** | HTML → PDF | WeasyPrint oder wkhtmltopdf |
| **step4_add_bookmarks.py** | PDF-Bookmarks | Navigation im PDF |
| **step5_cleanup.py** | Aufräumen | Temporäre Dateien löschen |

### Alternative: Einzelne Schritte

Für Tests nur step2 + step3:
```bash
python3 step2_generate_html.py
python3 step3_generate_pdf.py
```

## Optionale Features

### Mermaid-Diagramme als SVG rendern

Für beste Qualität der Diagramme:

```bash
# Mermaid CLI installieren
npm install -g @mermaid-js/mermaid-cli

# Oder lokal im Projekt
cd compendium
npm install

# Dann PDF generieren
python3 generate_pdf_book.py
```

**Ohne Mermaid CLI:** Diagramme erscheinen als Code-Blöcke (immer noch lesbar)

## Fehlerbehebung

### Problem: "ModuleNotFoundError: No module named 'weasyprint'"
```bash
pip install weasyprint
```

### Problem: "ModuleNotFoundError: No module named 'markdown'"
```bash
pip install -r requirements.txt
```

### Problem: PDF-Generierung sehr langsam
**Normal:** 2-5 Minuten für 58 Kapitel (4+ MB PDF)

Bei längerer Wartezeit:
- Prüfe CPU-Auslastung
- WeasyPrint nutzt viel CPU für komplexe Layouts
- Warte bis zum Ende - es lohnt sich!

### Problem: Warnungen bei PDF-Generierung
```
WARNING: .notdef glyph rendered for Unicode string...
```
**Normal:** Betrifft nur wenige Zeichen (meist Chinesisch)  
**Lösung:** Kann ignoriert werden oder CJK-Fonts installieren

```
WARNING: Ignored `overflow-x: auto`...
```
**Normal:** WeasyPrint unterstützt einige CSS-Properties nicht  
**Keine Aktion nötig**

## Ausgabe-Dateien

Nach erfolgreicher Generierung findest du:

```
compendium/
├── pdf/
│   ├── ThemisDB-Kompendium-v1.3.4-professional.html  (4.7 MB)
│   └── ThemisDB-Kompendium-v1.3.4-professional.pdf   (4.1 MB)
```

## Weiterführende Dokumentation

- **PDF_LAYOUT_IMPROVEMENTS.md** - Technische Details aller Features
- **LAYOUT_COMPARISON.md** - Vorher/Nachher Vergleich
- **PDF_GENERATION_README.md** - Vollständige Anleitung

## Features im Detail

### 1. Widow/Orphan Control
Verhindert einzelne Zeilen am Seitenanfang/-ende:
- **Minimum 3 Zeilen** am Seitenende (orphans)
- **Minimum 3 Zeilen** am Seitenanfang (widows)

### 2. Intelligente Seitenumbrüche
- Überschriften bleiben mit folgendem Text zusammen
- Code-Blöcke nicht geteilt
- Tabellen nicht geteilt
- Abbildungen mit Beschriftungen zusammen

### 3. Seitennummerierung
- **Römisch** (i, ii, iii) für Verzeichnisse
- **Arabisch** (1, 2, 3) für Hauptinhalt
- **Kopfzeile** mit Buchtitel
- **Fußzeile** mit Seitenzahl

### 4. Professionelle Typografie
- **Body Text:** Georgia (Serif) - optimal für Print
- **Headings:** Helvetica Neue (Sans-Serif) - modern
- **Code:** Consolas/Courier (Monospace)
- **Automatische Silbentrennung**
- **Blocksatz**

## Performance

### Typische Zeiten
- **HTML Generierung:** 5-15 Sekunden
- **PDF Konvertierung:** 2-5 Minuten
- **Gesamt:** ~3-5 Minuten

### Dateigröße
- **HTML:** ~4.7 MB (mit inline SVG/PNG)
- **PDF:** ~4.1 MB (optimiert)

## Best Practices

### 1. Regelmäßige Generierung
Generiere PDF nach größeren Änderungen:
```bash
# Nach Updates mehrerer Kapitel
cd compendium
python3 step2_generate_html.py
python3 step3_generate_pdf.py
```

### 2. Version Control
Committiere nur:
- ✅ Python-Skripte
- ✅ Markdown-Dateien
- ✅ Dokumentation

**Nicht** committen:
- ❌ Große PDF-Dateien (zu `.gitignore` hinzufügen)
- ❌ HTML-Intermediates
- ❌ temp/ Verzeichnis

### 3. Qualitätskontrolle
Prüfe generiertes PDF:
- Seitenzahlen korrekt?
- Inhaltsverzeichnis korrekt?
- Keine komisch geteilten Absätze?
- Diagramme korrekt dargestellt?

## Support

Bei Problemen:
1. Prüfe [Fehlerbehebung](#fehlerbehebung) oben
2. Siehe ausführliche Dokumentation
3. Erstelle Issue auf GitHub

## Beispiel-Workflow

```bash
# 1. In compendium wechseln
cd /path/to/ThemisDB/compendium

# 2. Dependencies checken
pip install -r requirements.txt

# 3. (Optional) Mermaid installieren für beste Diagramme
npm install

# 4. PDF generieren (komplette Pipeline)
python3 step1_generate_svgs.py
python3 step2_generate_html.py
python3 step3_generate_pdf.py
python3 step4_add_bookmarks.py
python3 step5_cleanup.py

# 5. Ergebnis prüfen
ls -lh output/*.pdf

# 6. PDF öffnen (Linux)
xdg-open output/ThemisDB-Kompendium-*.pdf

# Oder (macOS)
open output/ThemisDB-Kompendium-*.pdf

# Oder (Windows)
start output/ThemisDB-Kompendium-*.pdf
```

## Lizenz

Gleiche Lizenz wie ThemisDB: MIT License with Government Clause

---

**Letzte Aktualisierung:** 2025-01-11  
**Version:** 1.3.4  
**Autor:** ThemisDB Documentation Team
