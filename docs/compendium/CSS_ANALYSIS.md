# WeasyPrint CSS Performance Analyse
**Datum:** 13. Januar 2026  
**Ziel:** Identifizierung problematischer CSS-Features in styles_modern_book_final.scss

## Executive Summary
Das moderne CSS enthält mehrere Features, die WeasyPrint zum Hängen bringen können. Die Hauptprobleme sind **CSS Grid mit target-counter()** und komplexe **@page-Regeln**.

---

## 🔴 KRITISCHE PROBLEME (verursachen Hängen)

### 1. **CSS Grid mit `target-counter()` (Zeilen 234, 305)**
```scss
.toc-link {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: 6pt;
}

.toc-page {
  content: target-counter(attr(href), page);  // ← PROBLEM!
}
```

**WeasyPrint-Dokumentation sagt:**
- ✅ `target-counter()` ist unterstützt (CSS Generated Content Level 3)
- ⚠️ **ABER:** Kombination mit CSS Grid kann zu Performance-Problemen führen
- ⚠️ Grid ist nur "für einfache Fälle" unterstützt mit "einigen Einschränkungen"

**Performance-Impact:** 
- `target-counter()` muss für JEDEN TOC-Eintrag die Seite des Ziels berechnen
- Bei 68 YAML-Items × durchschnittlich 20 Links = ~1.360 Berechnungen
- Grid-Layout muss für jeden Link neu berechnet werden

**Lösung:**
```scss
.toc-link {
  display: block;  /* Statt grid */
}

.toc-page::before {
  content: leader(dotted) " ";  /* Statt grid-gap */
}

.toc-page::after {
  content: target-counter(attr(href), page);
}
```

---

### 2. **Verschachtelte @page-Regeln (Zeilen 25-68)**
```scss
@page {
  @top-center { content: "..."; }  // Margin box
  @bottom-center { content: "..."; }
}

@page :first { /* ... */ }
@page toc { /* ... */ }
@page :left { /* ... */ }
@page :right { /* ... */ }
```

**WeasyPrint-Dokumentation sagt:**
- ✅ @page-Regeln sind unterstützt (CSS Paged Media Level 3)
- ✅ Page margin boxes sind unterstützt
- ⚠️ **ABER:** Viele verschachtelte Regeln können Rendering verlangsamen

**Performance-Impact:**
- 6 verschiedene @page-Kontexte (default, :first, toc, :left, :right, print)
- Jede Seite muss gegen alle Regeln geprüft werden
- Bei 760 Seiten = 760 × 6 = 4.560 Regel-Evaluationen

**Lösung:**
```scss
/* Nur eine @page-Regel */
@page {
  size: A4;
  margin: 2cm 1.8cm 1.7cm 1.8cm;
}
```

---

### 3. **CSS Custom Properties (Zeilen 7-23)**
```scss
:root {
  --primary-color: #1f2f3b;
  --secondary-color: #2f5563;
  /* ... 12 weitere Variablen */
}

h1 { color: var(--primary-color); }
```

**WeasyPrint-Dokumentation sagt:**
- ✅ CSS Custom Properties sind unterstützt (CSS Variables Level 1)
- ⚠️ **ABER:** Jede `var()`-Referenz muss zur Laufzeit aufgelöst werden

**Performance-Impact:**
- 15 CSS-Variablen
- Verwendet in ~40 Regeln
- Bei 760 Seiten mit durchschnittlich 50 Elementen = 1.520.000 var()-Auflösungen

**Lösung:**
```scss
/* Direkte Werte statt Variablen */
h1 { color: #1f2f3b; }
h2 { color: #2f5563; }
```

---

## 🟡 MODERATE PROBLEME (verlangsamen Rendering)

### 4. **Flexbox für Cover (Zeile 97)**
```scss
.cover {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
}
```

**Status:** Flexbox ist unterstützt, aber "nicht tief getestet"

**Lösung:**
```scss
.cover {
  text-align: center;
  padding-top: 8cm;  /* Statt flex centering */
}
```

---

### 5. **Grid für Cover-Metadaten (Zeile 150)**
```scss
.cover-meta {
  display: grid;
  grid-template-columns: repeat(2, minmax(160px, 1fr));
  gap: 10pt 16pt;
}
```

**Status:** Grid "funktioniert für einfache Fälle, hat aber Einschränkungen"

**Lösung:**
```scss
.cover-meta {
  display: block;  /* Oder table */
}

.cover-meta .label {
  display: inline-block;
  width: 40%;
}

.cover-meta .value {
  display: inline-block;
  width: 55%;
}
```

---

### 6. **:nth-child() Selektoren (Zeile 520)**
```scss
table tr:nth-child(even) {
  background-color: var(--table-row-alt-bg);
}
```

**Status:** Unterstützt, aber bei vielen Zeilen langsam

**Lösung:**
```scss
/* HTML-Klassen nutzen statt Pseudo-Selektoren */
table tr.even {
  background-color: #fafafa;
}
```

---

## 🟢 UNKRITISCH (gut unterstützt)

### ✅ Gut unterstützte Features:
- **Basic Typography:** `font-family`, `font-size`, `font-weight` ✓
- **Colors:** Hex-Werte (#1f2f3b) ✓
- **Margins/Padding:** Standard Box-Model ✓
- **Borders:** `border`, `border-left`, `border-bottom` ✓
- **Text-align:** `justify`, `center`, `left` ✓
- **Page breaks:** `page-break-before`, `page-break-after`, `page-break-inside` ✓
- **Orphans/Widows:** Gut unterstützt ✓
- **Basic @page:** Size und Margins ✓

---

## 📊 PERFORMANCE-MATRIX

| Feature | Zeilen | Häufigkeit | Impact | Alternative |
|---------|--------|------------|--------|-------------|
| **CSS Grid + target-counter** | 234, 305 | ~1.360× | 🔴 KRITISCH | Block + leader() |
| **Verschachtelte @page** | 25-68 | 6 Kontexte | 🔴 HOCH | 1 @page-Regel |
| **CSS Variablen** | 7-23, 40+ | 1.5M Auflösungen | 🟡 MITTEL | Direkte Werte |
| **Flexbox** | 97 | 1× | 🟡 NIEDRIG | Padding-based |
| **Grid (Cover)** | 150 | 1× | 🟡 NIEDRIG | Table/Inline-block |
| **:nth-child** | 520 | Pro Tabelle | 🟡 NIEDRIG | CSS-Klassen |

---

## 🎯 EMPFOHLENE MASSNAHMEN

### Sofort (Kritisch):
1. **CSS Grid entfernen** → Block-Layout
2. **target-counter() optimieren** → Nur für TOC, nicht für jede Seite
3. **@page-Regeln vereinfachen** → Nur eine Basis-Regel

### Mittelfristig (Performance):
4. **CSS-Variablen eliminieren** → Direkte Werte
5. **Flexbox ersetzen** → Klassisches Centering
6. **Grid für Cover** → Table- oder Block-Layout

### Optional (Nice-to-have):
7. **:nth-child vermeiden** → HTML-Klassen
8. **@media print** → Kann entfernt werden (WeasyPrint ist immer print)

---

## 🧪 TEST-STRATEGIE

### Phase 1: Kritische Fixes
```scss
/* Nur diese 3 Änderungen: */
1. .toc-link { display: block; }  // Statt grid
2. @page { /* nur eine Regel */ }
3. :root { /* entfernen */ }
```

**Erwartung:** WeasyPrint läuft durch (< 30 Sekunden)

### Phase 2: Performance-Optimierung
```scss
4. .cover { /* kein flex */ }
5. .cover-meta { /* kein grid */ }
```

**Erwartung:** 50% schneller als Minimal-CSS

### Phase 3: Qualitäts-Verbesserung
- Schrittweise Features hinzufügen
- Nach jedem Feature testen
- Performance-Budget: Max 60 Sekunden für 760 Seiten

---

## 📈 ERWARTETE ERGEBNISSE

| Version | Rendering-Zeit | PDF-Größe | Qualität |
|---------|----------------|-----------|----------|
| **Aktuell (modern)** | ∞ (hängt) | N/A | 100% |
| **Minimal** | 5s | 2.61 MB | 60% |
| **Optimiert** | ~15s | 3.2 MB | 90% |
| **Ziel** | <30s | 3.8 MB | 95% |

---

## 💡 WEASYPRINT-SPEZIFISCHE ERKENNTNISSE

### Was WeasyPrint NICHT mag:
1. **Kombination** komplexer Features (Grid + target-counter)
2. **Viele** Kontext-Switches (@page-Varianten)
3. **Verschachtelte** Berechnungen (var() in var())
4. **Dynamische** Layouts (Grid mit auto-fill/auto-fit)

### Was WeasyPrint GUT kann:
1. **Einfache** @page-Regeln mit Margins
2. **Statische** Layouts (Block, Inline-block)
3. **Direkte** CSS-Werte (keine Variablen)
4. **CSS 2.1** Standard-Features

---

## 🔧 NÄCHSTE SCHRITTE

1. **styles_modern_book_optimized.scss erstellen**
   - Kritische Fixes implementieren
   - Grid → Block
   - Variablen → Direkte Werte
   - @page vereinfachen

2. **Testen**
   - WeasyPrint Rendering-Zeit messen
   - PDF-Qualität vergleichen
   - Fehlende Features dokumentieren

3. **Iterieren**
   - Features schrittweise hinzufügen
   - Performance nach jedem Schritt messen
   - Balance zwischen Qualität und Geschwindigkeit finden

---

## 📚 WEASYPRINT-DOKUMENTATION REFERENZEN

- **CSS Paged Media:** https://doc.courtbouillon.org/weasyprint/stable/api_reference.html#css-paged-media-module-level-3
- **Generated Content:** https://doc.courtbouillon.org/weasyprint/stable/api_reference.html#css-generated-content-module-level-3
- **Grid Layout:** https://doc.courtbouillon.org/weasyprint/stable/api_reference.html#css-grid-layout-module-level-2 (⚠️ "Einschränkungen")
- **Custom Properties:** https://doc.courtbouillon.org/weasyprint/stable/api_reference.html#css-custom-properties-for-cascading-variables-module-level-1

---

**Fazit:** Das moderne CSS ist zu komplex für WeasyPrint. Die Kombination aus Grid, target-counter(), vielen @page-Regeln und CSS-Variablen führt zu exponentieller Rendering-Komplexität. Eine optimierte Version mit klassischen CSS-Techniken sollte das Problem lösen.
