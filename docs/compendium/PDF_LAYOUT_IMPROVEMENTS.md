# PDF Layout Improvements - ThemisDB Kompendium

## Übersicht

Die PDF-Generierung wurde mit professionellen Buchlayout-Funktionen verbessert, orientiert an Standards wie Microsoft Word Buchvorlagen und professionellen Verlagspraktiken.

## Implementierte Verbesserungen

### 1. Seitenverwaltung und Nummerierung

#### Durchgehende Seitennummerierung
- **Implementiert mit:** `@page { @bottom-center { content: counter(page); } }`
- Seitenzahlen erscheinen zentriert am Seitenfuß
- Schriftgröße: 10pt, gut lesbar
- Erste Seite (Titelseite) ohne Seitennummer

#### Römische Seitenzahlen für Verzeichnisse
- **Implementiert mit:** `@page toc { @bottom-center { content: counter(page, lower-roman); } }`
- Inhaltsverzeichnis und andere Verzeichnisse nutzen römische Zahlen (i, ii, iii...)
- Hauptinhalt nutzt arabische Zahlen (1, 2, 3...)

#### Running Headers (Kolumnentitel)
- **Implementiert mit:** `@page { @top-center { content: string(chapter-title); } }`
- Kopfzeile zeigt Buchtitel oder Kapiteltitel
- Schriftgröße: 9pt, italic
- Dezent abgetrennt mit Unterstrich

### 2. Professionelle Seitenumbruchskontrolle

#### Widow und Orphan Control
- **Implementiert mit:** `orphans: 3; widows: 3;`
- Verhindert einzelne Zeilen am Seitenanfang (widows) oder -ende (orphans)
- Mindestens 3 Zeilen zusammen für bessere Lesbarkeit
- Gilt für Absätze, Listen, Tabellen

#### Intelligente Seitenumbrüche bei Überschriften
- **Implementiert mit:** `h1, h2, h3 { page-break-after: avoid; }`
- Überschriften werden nie vom folgenden Text getrennt
- H1 (Kapitel) beginnen immer auf neuer Seite
- H2-H6 vermeiden Seitenumbrüche direkt danach

#### Zusammenhalt von Elementen
```css
pre, blockquote, table, figure {
    page-break-inside: avoid;
}
```
- Code-Blöcke werden nicht über Seiten aufgeteilt
- Tabellen bleiben zusammen auf einer Seite
- Abbildungen und Beschriftungen bleiben zusammen
- Blockzitate werden nicht getrennt

### 3. Typografie nach Buchstandards

#### Schriftfamilien
- **Fließtext:** Georgia, Times New Roman (Serif) - Standard für gedruckte Bücher
- **Überschriften:** Helvetica Neue, Arial (Sans-Serif) - modern und klar
- **Code:** Consolas, Courier New (Monospace) - gut lesbar und unterscheidbar

#### Schriftgrößen und Zeilenhöhe
- **Body Text:** 11pt mit Zeilenhöhe 1.6 - optimal für Lesbarkeit
- **H1 (Kapitel):** 20pt - deutliche Hierarchie
- **H2:** 15pt, H3: 13pt, H4: 12pt - klare Abstufung
- **Code:** 9pt - platzsparend aber lesbar
- **Fußnoten/Bildunterschriften:** 9.5pt

#### Textausrichtung
- **Blocksatz mit automatischer Silbentrennung:** `text-align: justify; hyphens: auto;`
- Professioneller Buchsatz
- Verhindert unschöne Lücken im Text

#### Einzüge wie in Büchern
```css
p + p {
    text-indent: 1.5em;
}
```
- Erste Absätze nach Überschriften ohne Einzug
- Folgende Absätze mit 1.5em Einzug (klassischer Buchstil)

### 4. Seitenzahlen im Inhaltsverzeichnis

#### Mit CSS target-counter
```css
.toc-list a::after {
    content: target-counter(attr(href), page);
}
```
- Automatische Seitenzahlen aus Ankern
- Wird von WeasyPrint korrekt gerendert
- Punktelinie zwischen Titel und Seitenzahl

### 5. Seitenränder und Layout

#### A4-Format mit professionellen Rändern
- **Format:** A4 (210mm × 297mm)
- **Ränder:** 
  - Oben: 2.5cm (Platz für Kopfzeile)
  - Unten: 2cm (Platz für Seitenzahl)
  - Links/Rechts: unterschiedlich für Buchdruck

#### Unterschiedliche Ränder für linke/rechte Seiten
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
- Berücksichtigt Buchrücken bei Bindung
- Mehr Rand zur Buchmitte hin
- Standard in professionellen Buchvorlagen

### 6. Abbildungen und Tabellen

#### Abbildungen mit Rahmen und Hintergrund
```css
.figure-container {
    page-break-inside: avoid;
    border: 1pt solid #e0e0e0;
    background: #fafafa;
}
```
- Abbildungen werden hervorgehoben
- Bleiben immer zusammen mit Bildunterschrift
- Dezenter Rahmen und Hintergrund

#### Tabellenköpfe wiederholen
```css
thead {
    display: table-header-group;
}
```
- Bei mehrseitigen Tabellen erscheint Kopfzeile auf jeder Seite
- Wichtig für Referenztabellen

## Verwendete Tools und Features

### WeasyPrint (Version 67.0+)
- Unterstützt CSS Paged Media Level 3
- Implementiert `@page` Regeln vollständig
- Unterstützt `orphans` und `widows`
- Gute Unterstützung für `page-break-*` Eigenschaften

### CSS Paged Media Features
- `@page` - Seitenlayout definieren
- `@top-center`, `@bottom-center` - Running headers/footers
- `counter(page)` - Automatische Seitennummerierung
- `string-set` und `string()` - Kapitelüberschriften in Header
- `target-counter()` - Seitenzahlen im Inhaltsverzeichnis
- `page-break-before`, `page-break-after`, `page-break-inside`
- `orphans` und `widows` - Schusterjungen und Hurenkinder vermeiden

## Verwendung

### step1-5 Pipeline (Production)
```bash
cd compendium
python3 step1_generate_svgs.py      # Mermaid → SVG
python3 step2_generate_html.py      # Markdown → HTML mit professionellem Layout
python3 step3_generate_pdf.py       # HTML → PDF
python3 step4_add_bookmarks.py      # PDF-Lesezeichen
python3 step5_cleanup.py            # Aufräumen
```

**Ausgabe:** `output/ThemisDB-Kompendium-{VERSION}.pdf`

## Vergleich zu vorher

| Feature | Vorher | Jetzt |
|---------|--------|-------|
| Seitennummerierung | Ja, einfach | Ja, mit römischen Zahlen für TOC |
| TOC Seitenzahlen | Statisch/manuell | Automatisch mit target-counter |
| Widow/Orphan Control | Nein | Ja, min. 3 Zeilen |
| Page Break Control | Basis | Erweitert für alle Elemente |
| Running Headers | Nein | Ja, mit Buchtitel |
| Verso/Recto Seiten | Nein | Ja, unterschiedliche Ränder |
| Texteinzüge | Nein | Ja, wie in Büchern |
| Silbentrennung | Nein | Ja, automatisch |
| Typografie | Einfach | Professionell (Serif/Sans-Serif Mix) |

## Best Practices implementiert

### 1. Aus Microsoft Word Buchvorlagen
- Unterschiedliche Kopf-/Fußzeilen für erste Seite
- Römische Zahlen für Vorspann
- Kolumnentitel (Running Headers)
- Kapitel auf neuer Seite beginnen
- Professionelle Schriftgrößen und -familien

### 2. Aus professionellem Verlagswesen
- Widow/Orphan Control (Schusterjungen und Hurenkinder)
- Unterschiedliche Ränder für linke/rechte Seiten
- Blocksatz mit Silbentrennung
- Absatzeinzüge (außer nach Überschriften)
- Hierarchische Typografie (Serif für Text, Sans-Serif für Überschriften)

### 3. Aus Web-to-Print Best Practices
- `page-break-inside: avoid` für zusammenhängende Elemente
- Optimierte Schriftgrößen für Print
- Proper use of CSS Paged Media Modul
- Farboptimierung für Druck

## Bekannte Einschränkungen

1. **Mermaid-Diagramme:** Benötigt `mmdc` (Mermaid CLI) für SVG-Rendering
   - Fallback auf Code-Blöcke wenn nicht verfügbar
   - Installation: `npm install -g @mermaid-js/mermaid-cli`

2. **Schriftarten:** Nutzt Systemschriften
   - Georgia/Times New Roman müssen installiert sein
   - Fallback auf Browser-Standard-Schriften

3. **Chinesische Zeichen:** Warnung bei fehlenden Glyphen
   - Betrifft nur wenige Stellen im Dokument
   - Kann durch Installation von CJK-Fonts behoben werden

## Weiterführende Verbesserungen (optional)

Für zukünftige Versionen könnten folgende Features hinzugefügt werden:

- [ ] Stichwortverzeichnis (Index) mit Seitenzahlen
- [ ] Fußnoten mit automatischer Nummerierung
- [ ] Literaturverzeichnis mit Backlinks
- [ ] Kreuzverweise zwischen Kapiteln
- [ ] Marginalspalten für Notizen
- [ ] Zweispaltiges Layout für bestimmte Abschnitte
- [ ] Farbprofile für professionellen Druck (CMYK)
- [ ] PDF/A-Standard für Archivierung
- [ ] PDF-Lesezeichen (Bookmarks) aus Überschriften

## Referenzen

- **CSS Paged Media Module Level 3:** https://www.w3.org/TR/css-page-3/
- **WeasyPrint Documentation:** https://doc.courtbouillon.org/weasyprint/
- **Professional Book Design Principles:** Robert Bringhurst, "The Elements of Typographic Style"
- **Microsoft Word Book Templates:** Office.com Book Templates

## Autor und Datum

- **Erstellt:** 2025-01-11
- **ThemisDB Version:** v1.3.4
- **Ziel:** Professionelles Buchlayout für technische Dokumentation
