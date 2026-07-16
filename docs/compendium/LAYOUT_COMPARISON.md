# Vergleich: Vorher / Nachher - PDF Layout

## Übersicht der Verbesserungen

Diese Datei zeigt die konkreten Unterschiede zwischen der alten und neuen PDF-Generierung.

## 1. Seitennummerierung

### Vorher (build_pdf_final.py - alt)
```css
@page { 
    size: A4; 
    margin: 2cm; 
    @bottom-center { 
        content: counter(page); 
        font-size: 9pt; 
    } 
}
```
- ✅ Einfache Seitennummerierung
- ❌ Keine römischen Zahlen für Verzeichnisse
- ❌ Keine Running Headers

### Nachher (step2_generate_html.py mit professionellem CSS)
```css
@page {
    size: A4;
    margin: 2.5cm 2cm 2cm 2cm;
    
    @top-center {
        content: "ThemisDB v1.3.4 - Das vollständige Handbuch";
        font-size: 9pt;
        font-style: italic;
        color: #666;
    }
    
    @bottom-center {
        content: counter(page);
        font-size: 10pt;
        font-weight: 500;
    }
}

@page toc {
    @bottom-center {
        content: counter(page, lower-roman);
    }
}
```
- ✅ Running Headers mit Buchtitel
- ✅ Römische Zahlen für TOC (i, ii, iii)
- ✅ Arabische Zahlen für Hauptinhalt (1, 2, 3)
- ✅ Größere, besser lesbare Seitenzahlen

## 2. Widow/Orphan Control

### Vorher
```css
p { 
    margin-bottom: 8pt; 
    text-align: justify; 
    line-height: 1.6; 
}
```
- ❌ Keine Kontrolle über Seitenumbrüche in Absätzen
- ❌ Einzelne Zeilen können am Seitenanfang/ende stehen

### Nachher
```css
html {
    orphans: 3;
    widows: 3;
}

p { 
    margin-bottom: 10pt; 
    text-align: justify; 
    line-height: 1.6;
    orphans: 3;
    widows: 3;
}
```
- ✅ Mindestens 3 Zeilen am Seitenende (orphans) 
- ✅ Mindestens 3 Zeilen am Seitenanfang (widows)
- ✅ Verhindert "Schusterjungen" (orphans) und "Hurenkinder" (widows)

## 3. Überschriften und Seitenumbrüche

### Vorher
```css
h1 { 
    page-break-before: always; 
    font-size: 18pt; 
    /* ... */
}

h2 { 
    font-size: 13pt; 
    /* ... */
}
```
- ✅ H1 beginnt auf neuer Seite
- ❌ Überschriften können vom Text getrennt werden

### Nachher
```css
h1, h2, h3, h4, h5, h6 {
    font-family: 'Helvetica Neue', 'Arial', sans-serif;
    page-break-after: avoid;
    page-break-inside: avoid;
    orphans: 3;
    widows: 3;
}

h1 {
    page-break-before: always;
    string-set: chapter-title content();
    /* ... */
}
```
- ✅ Überschriften bleiben mit folgendem Text zusammen
- ✅ Kapitelüberschriften erscheinen im Header
- ✅ Keine einsamen Überschriften am Seitenende

## 4. Typografie

### Vorher
```css
body { 
    font-family: 'Segoe UI', 'Roboto', 'Helvetica Neue', sans-serif; 
    line-height: 1.5; 
}
```
- ❌ Sans-Serif für Fließtext (nicht optimal für Druck)
- ❌ Keine Silbentrennung

### Nachher
```css
body {
    font-family: 'Georgia', 'Times New Roman', serif;
    line-height: 1.6;
    text-align: justify;
    hyphens: auto;
}

h1, h2, h3, h4, h5, h6 {
    font-family: 'Helvetica Neue', 'Arial', sans-serif;
}
```
- ✅ Serif-Schrift für Fließtext (Standard in Büchern)
- ✅ Sans-Serif für Überschriften (moderner Kontrast)
- ✅ Automatische Silbentrennung
- ✅ Optimale Zeilenhöhe 1.6

## 5. Seitenzahlen im Inhaltsverzeichnis

### Vorher
```html
<li>
    <a href="#c1">1. Introduction</a>
    <span class="toc-page-num">3</span>
</li>
```
- ❌ Statische, manuell eingetragene Seitenzahlen
- ❌ Müssen bei Änderungen manuell aktualisiert werden

### Nachher
```css
.toc-list a::after {
    content: target-counter(attr(href), page);
    float: right;
    color: #666;
    font-weight: 600;
}
```
```html
<li>
    <a href="#c1">1. Introduction</a>
</li>
```
- ✅ Automatische Seitenzahlen via CSS
- ✅ Werden automatisch berechnet
- ✅ Immer aktuell, auch bei Änderungen

## 6. Abbildungen und Tabellen

### Vorher
```css
figure { 
    margin: 15pt 0; 
    page-break-inside: avoid; 
    text-align: center; 
}
```
- ✅ Grundlegende Kontrolle
- ❌ Kein visuelles Highlight

### Nachher
```css
.figure-container {
    margin: 16pt 0;
    padding: 10pt;
    text-align: center;
    page-break-inside: avoid;
    orphans: 3;
    widows: 3;
    border: 1pt solid #e0e0e0;
    background: #fafafa;
}

thead {
    display: table-header-group;
}
```
- ✅ Abbildungen mit Rahmen und Hintergrund
- ✅ Bessere visuelle Abgrenzung
- ✅ Tabellenköpfe auf jeder Seite wiederholt

## 7. Listen und Code-Blöcke

### Vorher
```css
ul, ol { 
    margin-left: 20pt; 
    margin-bottom: 8pt; 
}

pre { 
    page-break-inside: avoid; 
}
```
- ✅ Grundlegende Formatierung
- ❌ Keine Kontrolle über Listen-Umbrüche

### Nachher
```css
ul, ol {
    margin-left: 25pt;
    margin-bottom: 10pt;
    orphans: 3;
    widows: 3;
}

li {
    page-break-inside: avoid;
}

pre {
    page-break-inside: avoid;
    orphans: 4;
    widows: 4;
}
```
- ✅ Listenelemente bleiben zusammen
- ✅ Code-Blöcke mit erweiteter Kontrolle (4 Zeilen)
- ✅ Bessere Lesbarkeit

## 8. Buchbindungs-Ränder

### Vorher
```css
@page { 
    size: A4; 
    margin: 2cm; 
}
```
- ❌ Gleiche Ränder auf allen Seiten
- ❌ Berücksichtigt nicht die Buchbindung

### Nachher
```css
@page :left {
    margin-left: 2.5cm;
    margin-right: 2cm;
}

@page :right {
    margin-left: 2cm;
    margin-right: 2.5cm;
}
```
- ✅ Unterschiedliche Ränder für linke/rechte Seiten
- ✅ Mehr Platz zur Buchmitte (Bindung)
- ✅ Professioneller Buchdruck-Standard

## 9. Absatz-Einzüge

### Vorher
```css
p { 
    margin-bottom: 8pt; 
}
```
- ❌ Keine Absatz-Einzüge
- ❌ Alle Absätze gleich formatiert

### Nachher
```css
h1 + p, h2 + p, h3 + p, h4 + p {
    text-indent: 0;
}

p + p {
    text-indent: 1.5em;
    margin-top: 0;
}
```
- ✅ Erste Absätze nach Überschriften ohne Einzug
- ✅ Folgende Absätze mit 1.5em Einzug
- ✅ Klassischer Buch-Stil

## Zusammenfassung

| Aspekt | Vorher | Nachher |
|--------|--------|---------|
| **Seitennummerierung** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **TOC Seitenzahlen** | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Widow/Orphan** | ⭐ | ⭐⭐⭐⭐⭐ |
| **Page Breaks** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Typografie** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Running Headers** | ⭐ | ⭐⭐⭐⭐⭐ |
| **Buchlayout** | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Professionalität** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

## Verwendung

### step1-5 Pipeline (Empfohlene Production-Version)
```bash
cd compendium
python3 step1_generate_svgs.py
python3 step2_generate_html.py    # ← Mit professionellem Layout
python3 step3_generate_pdf.py
python3 step4_add_bookmarks.py
python3 step5_cleanup.py
```

### Alternative: Legacy build_pdf_final.py
```bash
cd compendium
python3 build_pdf_final.py
weasyprint pdf/ThemisDB-Kompendium-v1.3.4-print.html output.pdf
```

## Hinweis

Die step1-5 Pipeline (`step2_generate_html.py`) bietet jetzt das vollständige professionelle Layout. Die alte Version (`build_pdf_final.py`) wurde ebenfalls mit einigen Verbesserungen aktualisiert (Widow/Orphan Control, Running Headers) und bleibt als Legacy-Alternative verfügbar.
