# PDF-Generierungsstrategie: HTML+SVG+CSS → PDF (Text + Vektoren)

## Problem
Chrome `--print-to-pdf` erzeugt Pixeloutput (Rasterisierung) → große Dateigröße (~46 MB)

## Ziel
PDF mit echtem Text + SVG-Vektoren (nicht pixeliert) + CSS-Styling

---

## Optionen & Bewertung

### 1. **WeasyPrint** ✅ IDEAL (aber problematisch auf Windows)
- **Erzeugt:** Native PDF mit echtem Text + Vektor-SVGs
- **Pros:**
  - Erzeugt echte PDF-Text (nicht rasterisiert)
  - SVG-Support natürlich eingebettet
  - CSS/SCSS vollständig unterstützt
  - Kleine Ausgabe-Dateigröße (~5-15 MB erwartet)
- **Cons:**
  - GTK3-Abhängigkeiten schwierig auf Windows/WSL
  - Performance-Probleme bei sehr großen Dokumenten (aber beherrschbar mit richtigem HTML)
- **Für WSL:** Sollte mit korrekten GTK-Libraries funktionieren

### 2. **wkhtmltopdf** ✅ GUTE ALTERNATIVE
- **Erzeugt:** Native PDF mit echtem Text + SVG-Vektoren
- **Pros:**
  - Qt-basiert, zuverlässig
  - Gutes CSS-Support
  - SVG-Einbettung funktioniert gut
  - Kleine Dateigröße
- **Cons:**
  - Benötigt X11/Display-Server (in WSL konfigurierbar)
  - Weniger CSS3-Support als WeasyPrint
- **Für WSL:** Mit xvfb-run einsetzbar

### 3. **LibreOffice headless** ✅ BRAUCHBAR
- **Erzeugt:** Native PDF mit echtem Text
- **Pros:**
  - Auf den meisten Linux-Systemen vorinstalliert
  - Zuverlässig
  - CSS-Support durch Writer-Engine
- **Cons:**
  - Braucht HTML → ODT Konvertierung
  - SVG-Rendering kann unterschiedlich ausfallen
  - Langsamere Verarbeitung
- **Für WSL:** Einfache Installation

### 4. **Chrome --print-to-pdf** ❌ NICHT IDEAL
- **Erzeugt:** Pixeloutput (Rasterisierung)
- **Nachteile:**
  - Text wird zu Bildern
  - Sehr große Dateigröße
  - Nicht durchsuchbar
  - Schlechte Qualität beim Vergrößern

### 5. **Playwright/Puppeteer** ❌ NICHT IDEAL
- **Problem:** Gleich wie Chrome (Pixeloutput)

---

## EMPFOHLENE STRATEGIE

### **Primary: WeasyPrint in WSL**
```bash
# In WSL Ubuntu:
sudo apt-get install -y \
    python3-dev \
    libcairo2-dev \
    libffi-dev \
    libpango-1.0-0 \
    libpango-cairo-1.0-0 \
    libgdk-pixbuf2.0-0

pip3 install weasyprint
```

**Warum:**
- Erzeugt echte native PDFs (nicht pixeliert)
- SVG-Support ist native
- CSS vollständig unterstützt
- Kompakte Dateigröße (~8-15 MB erwartet statt 46 MB)
- Text durchsuchbar und kopierbar

### **Fallback: wkhtmltopdf**
```bash
sudo apt-get install -y wkhtmltopdf xvfb
```

**Verwendung:**
```bash
xvfb-run -a wkhtmltopdf input.html output.pdf
```

---

## HTML-Optimierung für Kleinere PDFs

1. **Inline SVGs statt externe Bilder**
   - SVGs direkt in HTML (bereits gemacht ✓)
   - Vermeidung zusätzlicher Dateien

2. **CSS-Optimierung**
   - Entfernung unnötiger Gradienten
   - Vereinfachte Farben
   - Minimale Box-Shadows

3. **SVG-Optimierung VOR Einbettung**
   ```python
   # Entfernung von:
   # - Metadata
   # - Style-Attributen (nicht nötig)
   # - Whitespace
   # - Dezimal-Precision (auf 2-3 Stellen reduzieren)
   ```

4. **HTML-Vereinfachung**
   - Minimale Bilder/Dekoratoren
   - Keine Doppel-Rendering

---

## Erwartete Dateigröße-Reduktion

| Methode | Größe | Format |
|---------|-------|--------|
| Chrome (aktuell) | 46 MB | Pixeloutput |
| WeasyPrint (optimiert) | 8-12 MB | Native PDF |
| wkhtmltopdf (optimiert) | 10-15 MB | Native PDF |

**Reduktion: 70-80% möglich**

---

## Empfehlung: Schritt-für-Schritt Vorgehen

1. ✅ **SVGs optimieren** (`optimize_svgs.py` - bereits erstellt)
   - Metadata entfernen
   - Whitespace minimieren
   - Dezimalstellen reduzieren

2. ✅ **HTML optimieren**
   - CSS vereinfachen
   - Unnötige Styling entfernen
   - Cover-Page minimal halten

3. **WeasyPrint in WSL verwenden**
   - Korrektes GTK-Setup
   - Native PDF-Generierung
   - ~10x Reduktion der Dateigröße

4. **Optional: Zusätzliche Ghostscript-Kompression**
   - Nach WeasyPrint PDF erzeugt ist
   - Maximale Kompression: `-dPDFSETTINGS=/screen` (minimal)

---

## Nächste Schritte

1. SVGs optimieren (`python3 optimize_svgs.py`)
2. HTML vereinfachen (CSS-Cleaning)
3. step3 auf WeasyPrint umstellen (mit korrektem Setup)
4. Test-Build durchführen
5. Dateigrößen vergleichen
