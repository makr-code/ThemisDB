# ThemisDB Compendium - Build-Anleitung

Dieses Verzeichnis enthält das vollständige ThemisDB Compendium mit allen Markdown-Quellen, Build-Skripten und Outputs.

## ✅ Konsolidierte Struktur

**Alle Compendium-Dateien befinden sich jetzt in `./compendium`:**

```
compendium/
├── *.md                          # Alle Markdown-Kapitel (28 Dateien)
├── generate_pdf_rendered.py      # PDF-Generator mit Mermaid-Rendering
├── mkdocs-compendium.yml         # MkDocs-Konfiguration
├── chrome/                       # Chromium für Mermaid-Rendering
│   └── linux-143.0.7499.169/
├── pdf/                          # ✨ Generierte PDF-Dateien
├── site/                         # Generated HTML-Website (MkDocs)
└── temp/                         # Temporäre Build-Dateien (auto-cleanup)
```

## PDF mit Mermaid-Diagrammen generieren

### Voraussetzungen (WSL/Linux)

```bash
# Python-Umgebung aktivieren
source /mnt/c/VCC/themis/.venv-wsl/bin/activate

# Benötigte Pakete (sollten bereits installiert sein)
pip install markdown weasyprint pygments
```

### PDF generieren (einfachster Weg)

```bash
cd /mnt/c/VCC/themis/compendium
source /mnt/c/VCC/themis/.venv-wsl/bin/activate

# Chrome-Pfad wird automatisch erkannt
python generate_pdf_rendered.py
```

**Ausgabe:** `pdf/ThemisDB-Kompendium-v1.3.4-YYYYMMDD-rendered.pdf`

## Features

### ✅ Mermaid-Diagramme
- **50+ Diagramme** werden als PNG-Bilder gerendert
- Weißer Hintergrund, keine farbigen Rahmen
- Professionelle Abbildungsbeschriftungen: **"Abb. N: Beschreibung"**
- **Automatisches Abbildungsverzeichnis** nach Titelseite
- Diagrammtypen: Flowcharts, Sequenzdiagramme, Gantt, ER-Diagramme, Zustandsautomaten

### ✅ Syntax Highlighting (VS Code Dark+ Theme)
- **Kompakte Darstellung:** 8.5pt Schrift, 1.4 Zeilenhöhe
- **Dunkler Hintergrund:** #1e1e1e mit blauem Rand-Akzent
- **Farbcodierung:**
  - Keywords: `#569cd6` (blau)
  - Strings: `#ce9178` (orange)  
  - Comments: `#6a9955` (grün)
  - Functions: `#dcdcaa` (gelb)
  - Types: `#4ec9b0` (türkis)
- **Sprachen:** Python, JavaScript, SQL, AQL, C++, Bash, YAML, JSON, Markdown
- **Intelligente Seitenumbrüche:** Kleine Code-Blöcke bleiben zusammen, große Blöcke dürfen umbrechen

### ✅ Professionelles Layout
- **Format:** A4 (210 × 297 mm)
- **Ränder:** 25mm oben/unten, 20mm links/rechts
- **Schriften:**
  - Body: Georgia 11pt, Zeilenhöhe 1.65
  - Code: Consolas 8.5pt
  - Überschriften: Helvetica Neue
- **Farben:** Lila-Akzente (#7c4dff) für Überschriften
- **Automatische Kopf-/Fußzeilen:**
  - Kopf: "ThemisDB Compendium v1.3.4"
  - Fuß: "Seite N"

## Statistiken (letzter Build)

- **Kapitel:** 28
- **Mermaid-Diagramme:** 50+ (46 erfolgreich gerendert)
- **PDF-Größe:** ~6-8 MB (mit embedded PNGs)
- **Build-Zeit:** ~2-4 Minuten
- **Seitenzahl:** ~250-300 Seiten (geschätzt)

## Troubleshooting

### ❌ Mermaid-Rendering schlägt fehl
**Problem:** `⚠ mmdc error:` im Log

**Lösung:**
- Node.js prüfen: `node --version` (min. v18)
- Chromium prüfen: `ls compendium/chrome/linux-*/chrome-linux64/chrome`
- Netzwerk: `npx @mermaid-js/mermaid-cli@latest --version`

### ❌ WeasyPrint-Fehler: "cairo not found"
**Problem:** `OSError: cannot load library 'libcairo.so.2'`

**Lösung (WSL):**
```bash
sudo apt update
sudo apt install libpango-1.0-0 libgdk-pixbuf-2.0-0 libcairo2 libnss3 libxss1
```

### ❌ Fehlende Abbildungen im PDF
**Problem:** Mermaid-Diagramme werden nicht angezeigt

**Lösung:**
1. Prüfen: `ls compendium/temp/themis_pdf_*/mermaid_images/*.png`
2. Log nach "✓" oder "✗" durchsuchen
3. Einzelnes Diagramm testen:
   ```bash
   cd compendium/temp
   echo "graph LR; A-->B" > test.mmd
   npx @mermaid-js/mermaid-cli@latest -i test.mmd -o test.png
   ```

### ⚠️ Syntax-Fehler in Mermaid-Diagrammen
**Bekannte Probleme:**
- Kapitel 3: Quadrant Chart mit Sonderzeichen
- Kapitel 7, 8, 13: HTML-Tags in Labels
- Lösung: Markdown-Dateien editieren, HTML entfernen

## Weitere Build-Optionen

### HTML-Site generieren (MkDocs)
```bash
cd /mnt/c/VCC/themis/compendium
mkdocs build -f mkdocs-compendium.yml
# Ausgabe: compendium/site/
```

### Site lokal testen
```bash
mkdocs serve -f mkdocs-compendium.yml
# Browser: http://localhost:8000
```

## Aktualisierungen

**Stand:** 31. Dezember 2025  
**Version:** 1.3.4  
**Letzte Änderungen:**
- ✅ Konsolidierung nach `./compendium`
- ✅ Chrome nach `./compendium/chrome` verschoben
- ✅ VS Code Syntax Highlighting implementiert
- ✅ Abbildungsverzeichnis hinzugefügt
- ✅ 10+ neue Diagramme in Kapiteln 4-17

