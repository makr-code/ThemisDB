# PDF Generation Instructions for v1.4.0-alpha

**Version:** v1.4.0-alpha  
**Date:** 6. Januar 2026

---

## Übersicht

Diese Anleitung beschreibt die Schritte zur Generierung des ThemisDB-Kompendium-PDFs für Version 1.4.0-alpha.

---

## Voraussetzungen

Stellen Sie sicher, dass folgende Software installiert ist:

```bash
# Python 3.8+
python3 --version

# Erforderliche Python-Pakete
pip install -r requirements.txt  # Falls vorhanden

# Für Mermaid-Diagramm-Rendering
npm install -g @mermaid-js/mermaid-cli
```

---

## Schritt 1: Vorbereitung

### 1.1 Markdown-Dateien überprüfen

```bash
cd /home/runner/work/ThemisDB/ThemisDB/compendium

# Prüfe alle Kapitel-Dateien
ls -lh chapter_*.md

# Prüfe Anhänge
ls -lh appendix_*.md
```

### 1.2 Mermaid-Diagramme testen

Stelle sicher, dass alle Mermaid-Diagramme korrekt formatiert sind:

```bash
# Teste Mermaid-Rendering (optional)
mmdc -i chapter_02_architecture.md -o test_output.pdf
```

### 1.3 Inhaltsverzeichnis prüfen

Überprüfe `index.md` und `mkdocs-compendium.yml`:

```bash
cat index.md
cat mkdocs-compendium.yml
```

---

## Schritt 2: PDF-Generierung

### 2.1 PDF generieren

```bash
cd /home/runner/work/ThemisDB/ThemisDB/compendium

# Führe PDF-Generierungsskript aus
python generate_pdf_with_mermaid.py
```

**Erwartete Ausgabe:**
```
Generating PDF from Markdown files...
Processing chapter_01_introduction.md...
Processing chapter_02_architecture.md...
...
Rendering Mermaid diagrams...
Generating table of contents...
Creating PDF: ThemisDB-Kompendium.pdf
Done! PDF saved to: pdf/ThemisDB-Kompendium.pdf
```

### 2.2 Alternative: Andere Generierungsskripte

Falls `generate_pdf_with_mermaid.py` Probleme macht, versuche:

```bash
# Variante 1: Mit WeasyPrint
python generate_pdf_weasyprint.py

# Variante 2: Mit SVG-Rendering
python generate_pdf_svg.py

# Variante 3: Themed PDFs
python generate_themed_pdfs.py
```

---

## Schritt 3: Qualitätssicherung

### 3.1 PDF öffnen und prüfen

```bash
# PDF im Standardviewer öffnen
xdg-open pdf/ThemisDB-Kompendium.pdf

# Oder mit spezifischem Viewer
evince pdf/ThemisDB-Kompendium.pdf
```

### 3.2 Checkliste

Prüfe folgende Punkte:

- [ ] **Inhaltsverzeichnis**: Alle Kapitel vorhanden und verlinkt
- [ ] **Seitenzahlen**: Korrekt und konsistent
- [ ] **Mermaid-Diagramme**: Alle korrekt gerendert
- [ ] **Code-Blöcke**: Gut lesbar und formatiert
- [ ] **Querverweise**: Links funktionieren
- [ ] **Formatierung**: Konsistent über alle Kapitel
- [ ] **Dateigröße**: Angemessen (typisch: 5-15 MB)

### 3.3 Seitenzahlen zählen

```bash
cd /home/runner/work/ThemisDB/ThemisDB/compendium

# Zähle Seiten im generierten PDF
python count_pages.py pdf/ThemisDB-Kompendium.pdf
```

---

## Schritt 4: Finalisierung

### 4.1 PDF umbenennen

```bash
cd /home/runner/work/ThemisDB/ThemisDB/compendium/pdf

# Benenne PDF mit Versionsnummer
mv ThemisDB-Kompendium.pdf ThemisDB-Kompendium-v1.4.0-alpha.pdf
```

### 4.2 PDF ins Wurzelverzeichnis kopieren

```bash
cd /home/runner/work/ThemisDB/ThemisDB

# Kopiere PDF ins Hauptverzeichnis
cp compendium/pdf/ThemisDB-Kompendium-v1.4.0-alpha.pdf .

# Prüfe, dass Datei vorhanden ist
ls -lh ThemisDB-Kompendium-v1.4.0-alpha.pdf
```

### 4.3 Alte PDF archivieren (optional)

```bash
# Falls alte PDF vorhanden, archiviere sie
if [ -f ThemisDB-Kompendium-v1.3.4.pdf ]; then
  mkdir -p archive/pdfs
  mv ThemisDB-Kompendium-v1.3.4.pdf archive/pdfs/
  echo "Old PDF archived"
fi
```

### 4.4 README.md aktualisieren

Aktualisiere die README.md mit dem Link zur neuen PDF:

```markdown
## Documentation

📚 **[ThemisDB Compendium v1.4.0-alpha (PDF)](ThemisDB-Kompendium-v1.4.0-alpha.pdf)** - Complete documentation (Updated: Jan 6, 2026)
```

---

## Schritt 5: Git Commit

### 5.1 PDF zum Repository hinzufügen

```bash
cd /home/runner/work/ThemisDB/ThemisDB

# Füge PDF zum Git-Repository hinzu
git add ThemisDB-Kompendium-v1.4.0-alpha.pdf
git add README.md  # Falls aktualisiert

# Commit mit aussagekräftiger Nachricht
git commit -m "Add ThemisDB Compendium PDF v1.4.0-alpha

- Generated from updated markdown documentation
- Includes all v1.4.0-alpha features:
  * 6 LLM features
  * 3 performance optimizations
  * HA features (Hot Spare, WAL Replication)
  * Enhanced monitoring metrics
  * PostgreSQL protocol enhancements
- Updated appendices
- Total: ~XXX pages"

# Push zum Repository
git push origin <your-branch-name>
```

---

## Troubleshooting

### Problem: Mermaid-Diagramme werden nicht gerendert

**Lösung:**
```bash
# Installiere mermaid-cli
npm install -g @mermaid-js/mermaid-cli

# Teste mermaid
mmdc --version
```

### Problem: PDF ist zu groß (>50 MB)

**Lösung:**
```bash
# Komprimiere PDF mit Ghostscript
gs -sDEVICE=pdfwrite \
   -dCompatibilityLevel=1.4 \
   -dPDFSETTINGS=/ebook \
   -dNOPAUSE -dQUIET -dBATCH \
   -sOutputFile=ThemisDB-Kompendium-v1.4.0-alpha-compressed.pdf \
   ThemisDB-Kompendium-v1.4.0-alpha.pdf
```

### Problem: Schrift nicht lesbar

**Lösung:**
Editiere `generate_pdf_with_mermaid.py` und erhöhe Font-Größe:

```python
# Suche nach font_size Parameter und erhöhe auf 11 oder 12
font_size = 11  # Standard: 10
```

### Problem: Python-Fehler beim Generieren

**Lösung:**
```bash
# Installiere fehlende Abhängigkeiten
pip install markdown weasyprint pymupdf pillow

# Oder verwende requirements.txt falls vorhanden
pip install -r requirements.txt
```

---

## Erwartetes Ergebnis

Nach erfolgreicher Generierung solltest du haben:

```
ThemisDB/
├── ThemisDB-Kompendium-v1.4.0-alpha.pdf    (Hauptverzeichnis)
├── compendium/
│   └── pdf/
│       └── ThemisDB-Kompendium-v1.4.0-alpha.pdf
└── README.md (aktualisiert mit PDF-Link)
```

**Dateigröße:** Typischerweise 5-15 MB  
**Seitenanzahl:** Geschätzt 250-350 Seiten (abhängig von Formatierung)

---

## Hinweis zur Dokumentationsqualität

Das generierte PDF sollte folgende v1.4.0-alpha Inhalte enthalten:

- ✅ Kapitel 17: 6 neue LLM-Features (~675 Zeilen)
- ✅ Kapitel 21: 3 Performance-Optimierungen (~419 Zeilen)
- ✅ Kapitel 16: Hot Spare & WAL Replication (~454 Zeilen)
- ✅ Kapitel 19: Enhanced Prometheus Metrics (~470 Zeilen)
- ✅ Kapitel 31: PostgreSQL Protocol Enhancements (~567 Zeilen)
- ✅ Appendix D: Feature Status Matrix (aktualisiert)
- ✅ Appendix H: Glossary mit 8 neuen Begriffen

**Gesamt:** ~3.000 Zeilen neue Dokumentation

---

## Support

Bei Problemen mit der PDF-Generierung:

1. Prüfe die Logs in `compendium/pdf/generation.log` (falls vorhanden)
2. Teste alternative Generierungsskripte
3. Öffne ein Issue auf GitHub mit:
   - Fehlermeldung
   - Python-Version (`python --version`)
   - Betriebssystem
   - Verwendetes Generierungsskript

---

**Viel Erfolg bei der PDF-Generierung!** 📄✨
