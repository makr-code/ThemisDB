# ThemisDB - Cross-Reference: Tools, Examples & WordPress Plugins

**Version:** 1.0.0  
**Datum:** 8. Januar 2026  
**Zweck:** Vermeidung von Duplikationen zwischen tools/, examples/ und wordpress-plugin/

---

## 📊 Übersicht

Dieses Dokument mappt die Beziehungen zwischen:
1. **Standalone Tools** (`/tools/`)
2. **WordPress-Plugins** (`/wordpress-plugin/`)
3. **Examples** (`/examples/`)

### Prinzip: Standalone + WordPress Variante

Analog zum TCO Calculator Modell:
- **Standalone Version** in `/tools/` - Kann direkt genutzt werden (HTML/JS/Python/etc.)
- **WordPress Variante** in `/wordpress-plugin/` - WordPress-Integration mit Shortcodes

---

## ✅ Bestehende Mappings (Standalone → WordPress)

### 1. TCO Calculator
- **Standalone:** `/tools/tco-calculator/` (HTML/JS/CSS)
- **WordPress:** `/wordpress-plugin/tco-calculator-wordpress/`
- **Status:** ✅ Beide vollständig implementiert
- **Pattern:** Standalone kann direkt genutzt werden, WordPress-Variante nutzt gleiche Logik

### 2. Benchmark Visualizer
- **Standalone:** Daten in `/benchmarks/` (JSON/CSV)
- **WordPress:** `/wordpress-plugin/benchmark-visualizer-wordpress/`
- **Status:** ✅ WordPress-Plugin implementiert
- **Datenquelle:** Benchmark-Daten aus Repository

### 3. Feature Matrix
- **Standalone:** Feature-Daten in `/docs/features/`
- **WordPress:** `/wordpress-plugin/feature-matrix-wordpress/`
- **Status:** ✅ WordPress-Plugin implementiert
- **Datenquelle:** Feature-Dokumentation

### 4. Architecture Diagrams
- **Standalone:** Mermaid-Diagramme in `/docs/architecture/`
- **WordPress:** `/wordpress-plugin/architecture-diagrams-wordpress/`
- **Status:** ✅ WordPress-Plugin implementiert
- **Datenquelle:** Architektur-Dokumentation

---

## 🏗️ Teilweise implementiert / In Entwicklung

### 5. Query Playground
- **Standalone:** Könnte in `/tools/query-playground/` implementiert werden
- **WordPress:** `/wordpress-plugin/query-playground-wordpress/` (in Entwicklung)
- **Status:** 🏗️ Nur WordPress-Variante
- **Empfehlung:** Standalone-Version für lokale Tests erstellen

### 6. Downloads Manager
- **Standalone:** Nutzt GitHub API direkt
- **WordPress:** `/wordpress-plugin/themisdb-downloads/`
- **Status:** 🏗️ Nur WordPress-Variante
- **Empfehlung:** Standalone-HTML-Version für Release-Pages

### 7. Gallery (Bildsuche)
- **Standalone:** Könnte in `/tools/image-gallery/` sein
- **WordPress:** `/wordpress-plugin/themisdb-gallery/`
- **Status:** 🏗️ Nur WordPress-Variante
- **Empfehlung:** Nicht kritisch, spezifisch für CMS

### 8. Wiki Integration
- **Standalone:** `/tools/publish_wiki.py` (existiert!)
- **WordPress:** `/wordpress-plugin/themisdb-wiki-integration/`
- **Status:** ⚠️ **DUPLIKATION GEFUNDEN**
- **Empfehlung:** Koordinieren - Python-Tool vs. WordPress-Plugin

### 9. Release Timeline
- **Standalone:** Daten aus CHANGELOG.md + GitHub API
- **WordPress:** `/wordpress-plugin/release-timeline-wordpress/` (nur Struktur)
- **Status:** 🏗️ Nur Struktur
- **Empfehlung:** Standalone-HTML-Version erstellen

### 10. Test Dashboard
- **Standalone:** Grafana/Prometheus (existiert bereits!)
- **WordPress:** `/wordpress-plugin/test-dashboard-wordpress/` (nur Struktur)
- **Status:** ⚠️ **POTENTIELLE DUPLIKATION**
- **Empfehlung:** WordPress zeigt Daten aus Grafana/GitHub Actions

---

## 💡 Vorgeschlagene Neue Plugins - Duplikations-Check

### Phase 4 Vorschläge

#### 11. Tutorial Builder Plugin
- **Standalone:** ❌ NICHT vorhanden
- **Examples:** Tutorials existieren in `/docs/` und `/examples/*/HOW_TO.md`
- **Duplikation:** ❌ Keine - Neues interaktives Format
- **Empfehlung:** ✅ **UMSETZEN** - Interaktive Version der HOW_TO Guides

#### 12. Use Case Showcase Plugin
- **Standalone:** ❌ NICHT vorhanden
- **Examples:** `/examples/` enthält 22+ Use Cases
- **Duplikation:** ⚠️ **POTENTIELLE ÜBERSCHNEIDUNG**
- **Empfehlung:** ✅ **UMSETZEN MIT ANPASSUNG**
  - WordPress-Plugin als **interaktiver Showcase** der `/examples/` Use Cases
  - Verlinkt zu den vollständigen Beispielen in `/examples/`
  - Zeigt Screenshots, Metriken, Architektur-Diagramme
  - **NICHT** Code duplizieren, sondern **referenzieren**

### Phase 5 Vorschläge

#### 13. Interactive Demo Builder
- **Standalone:** ❌ NICHT vorhanden
- **Examples:** Screenshots/Demos existieren verstreut
- **Duplikation:** ❌ Keine
- **Empfehlung:** ✅ **UMSETZEN**

#### 14. SDK & Integration Examples
- **Standalone:** ❌ NICHT als Tool vorhanden
- **Examples:** Code-Beispiele in `/examples/`, `/clients/`, `/sdks/`
- **Duplikation:** ⚠️ **POTENTIELLE ÜBERSCHNEIDUNG**
- **Empfehlung:** ✅ **UMSETZEN MIT ANPASSUNG**
  - WordPress-Plugin als **Code-Browser** für bestehende Beispiele
  - **Aggregiert** Code aus `/examples/`, `/clients/`, `/sdks/`
  - **NICHT** Code duplizieren, sondern **einbetten/verlinken**

#### 15. Migration Assistant
- **Standalone:** ❌ NICHT vorhanden
- **Examples:** Migration-Guides in `/docs/guides/`
- **Duplikation:** ❌ Keine
- **Empfehlung:** ✅ **UMSETZEN**

#### 16. Customer Success Stories
- **Standalone:** ❌ NICHT vorhanden
- **Examples:** Keine Testimonials/Case Studies vorhanden
- **Duplikation:** ❌ Keine
- **Empfehlung:** ✅ **UMSETZEN**

### Phase 6 Vorschläge

#### 17. Glossary & Term Explainer
- **Standalone:** ❌ NICHT als Tool vorhanden
- **Docs:** Glossar könnte in `/docs/glossary.md` sein
- **Duplikation:** ❌ Keine
- **Empfehlung:** ✅ **UMSETZEN**

#### 18. Interactive Performance Calculator
- **Standalone:** `/tools/compare_hyperscaler.py` existiert!
- **WordPress:** Vorgeschlagenes Plugin
- **Duplikation:** ⚠️ **DUPLIKATION GEFUNDEN**
- **Empfehlung:** ✅ **UMSETZEN ALS WORDPRESS-VARIANTE**
  - Analog zu TCO Calculator Pattern
  - Standalone Python-Tool in `/tools/`
  - WordPress-Plugin als interaktive Web-Version

#### 19. CDC Demo Plugin
- **Standalone:** ❌ NICHT vorhanden
- **Examples:** CDC-Beispiele in Code, aber nicht als Demo
- **Duplikation:** ❌ Keine
- **Empfehlung:** ✅ **UMSETZEN**

#### 20. News & Blog Aggregator
- **Standalone:** ❌ NICHT vorhanden
- **Examples:** `/tools/publish_wiki.py` für Wiki, aber nicht für News
- **Duplikation:** ❌ Keine
- **Empfehlung:** ✅ **UMSETZEN**

---

## 🔍 Gefundene Duplikationen & Empfehlungen

### ⚠️ 1. Wiki Integration
**Problem:**
- `/tools/publish_wiki.py` - Python-Skript für GitHub Wiki → MkDocs
- `/wordpress-plugin/themisdb-wiki-integration/` - WordPress GitHub Wiki Integration

**Lösung:**
- **Behalten:** Beide haben unterschiedliche Zwecke
- `publish_wiki.py` - Automatisiertes Publishing in CI/CD
- WordPress-Plugin - Content-Darstellung auf Website

### ⚠️ 2. Test Dashboard
**Problem:**
- Grafana/Prometheus Dashboards existieren bereits
- WordPress-Plugin würde gleiche Metriken zeigen

**Lösung:**
- **WordPress-Plugin:** Zeigt eingebettete Grafana-Dashboards oder GitHub Actions Status
- **NICHT** eigene Metrik-Erfassung implementieren
- Nutzt APIs: GitHub Actions, Grafana

### ⚠️ 3. Performance Calculator vs. compare_hyperscaler.py
**Problem:**
- `/tools/compare_hyperscaler.py` - Hyperscaler-Kostenvergleich
- Vorgeschlagenes Plugin - Performance Calculator

**Lösung:**
- **Analog zu TCO Calculator Pattern:**
  - `/tools/performance-calculator/` - Standalone HTML/JS Version
  - `/wordpress-plugin/performance-calculator-wordpress/` - WordPress-Variante
- `compare_hyperscaler.py` bleibt für CLI/Scripting

### ⚠️ 4. Use Case Showcase vs. /examples/
**Problem:**
- `/examples/` enthält 22+ vollständige Use Case Implementierungen
- Vorgeschlagenes Plugin würde Use Cases showcasen

**Lösung:**
- **WordPress-Plugin als Aggregator/Showcase:**
  ```
  Use Case Showcase Plugin
  ├── Listet alle /examples/*
  ├── Zeigt README.md Zusammenfassungen
  ├── Verlinkt zu vollständigem Code auf GitHub
  ├── Zeigt Screenshots/Diagramme
  └── Performance-Metriken (falls vorhanden)
  ```
- **NICHT** Code aus `/examples/` duplizieren
- **NUR** Metadaten und Links

### ⚠️ 5. SDK Examples vs. /clients/ und /examples/
**Problem:**
- `/clients/` - SDK-Implementierungen (Python, Java, Go, etc.)
- `/examples/` - Code-Beispiele mit verschiedenen SDKs
- Vorgeschlagenes Plugin - SDK Examples

**Lösung:**
- **WordPress-Plugin als Code-Browser:**
  ```
  SDK Examples Plugin
  ├── Scannt /clients/ und /examples/
  ├── Gruppiert nach Sprache/Framework
  ├── Syntax-Highlighting
  ├── Copy-to-Clipboard
  └── Links zu GitHub-Code
  ```
- **NICHT** Code duplizieren
- Könnte Code via GitHub API oder als embed/iframe anzeigen

---

## 📋 Aktualisierte Empfehlungen

### Sofort umzusetzen (Phase 4)

#### ✅ Tutorial Builder
- **Standalone:** Erstelle `/tools/tutorial-builder/` (HTML/JS)
- **WordPress:** Implementiere WordPress-Variante
- **Datenquelle:** Nutzt HOW_TO.md aus `/examples/*/HOW_TO.md`
- **Keine Duplikation**

#### ✅ Use Case Showcase (ANGEPASST)
- **Standalone:** Erstelle `/tools/use-case-showcase/` (HTML/JS)
- **WordPress:** Implementiere WordPress-Variante
- **Datenquelle:** Aggregiert Metadaten aus `/examples/*/README.md`
- **Vermeidet Duplikation:** Verlinkt zu vollständigem Code, dupliziert ihn nicht

### Mittelfristig (Phase 5)

#### ✅ Interactive Demo Builder
- Keine Duplikation - umsetzen wie geplant

#### ✅ SDK Examples (ANGEPASST)
- **Standalone:** Erstelle `/tools/sdk-examples-browser/` (HTML/JS)
- **WordPress:** Implementiere WordPress-Variante
- **Datenquelle:** Scannt `/clients/` und `/examples/`
- **Vermeidet Duplikation:** Zeigt Code via embed, dupliziert ihn nicht

#### ✅ Migration Assistant
- Keine Duplikation - umsetzen wie geplant

#### ✅ Customer Success Stories
- Keine Duplikation - umsetzen wie geplant

### Langfristig (Phase 6)

#### ✅ Glossary
- Keine Duplikation - umsetzen wie geplant

#### ✅ Performance Calculator (ANGEPASST)
- **Standalone:** Erstelle `/tools/performance-calculator/` (HTML/JS)
- **WordPress:** Implementiere WordPress-Variante
- **Existing:** `compare_hyperscaler.py` bleibt für CLI-Nutzung
- **Analog zu:** TCO Calculator Pattern

#### ✅ CDC Demo
- Keine Duplikation - umsetzen wie geplant

#### ✅ News Aggregator
- Keine Duplikation - umsetzen wie geplant

---

## 🏗️ Implementierungs-Pattern

### Standard-Pattern: Standalone + WordPress

Für jedes Tool/Plugin:

```
1. Standalone Version in /tools/<tool-name>/
   ├── index.html
   ├── style.css
   ├── app.js
   └── README.md

2. WordPress Variante in /wordpress-plugin/<tool-name>-wordpress/
   ├── themisdb-<tool-name>.php
   ├── assets/
   ├── templates/
   └── README.md
```

### Datenquellen-Hierarchie

1. **Primary Source:** Daten bleiben in Original-Location
   - `/examples/` für Use Cases
   - `/clients/` und `/sdks/` für SDK-Code
   - `/benchmarks/` für Performance-Daten
   - `/docs/` für Dokumentation

2. **Aggregation:** Tools/Plugins aggregieren via:
   - File-System-Scans (Standalone)
   - GitHub API (WordPress)
   - Direkte Einbettung (iframes, embeds)

3. **NO DUPLICATION:** Code wird nicht kopiert, sondern referenziert

---

## 📊 Zusammenfassung

### Duplikationen vermieden

| Plugin | Duplikation? | Lösung |
|--------|-------------|--------|
| Tutorial Builder | ❌ Keine | Neu - nutzt HOW_TO.md als Quelle |
| Use Case Showcase | ⚠️ Potentiell | Aggregator - verlinkt zu /examples/ |
| Interactive Demo | ❌ Keine | Neu |
| SDK Examples | ⚠️ Potentiell | Code-Browser - zeigt /clients/ + /examples/ |
| Migration Assistant | ❌ Keine | Neu |
| Success Stories | ❌ Keine | Neu |
| Glossary | ❌ Keine | Neu |
| Performance Calculator | ⚠️ Ja | Wie TCO: Standalone + WordPress, CLI bleibt |
| CDC Demo | ❌ Keine | Neu |
| News Aggregator | ❌ Keine | Neu |

### Neue Standalone Tools zu erstellen

Für konsistentes Pattern (analog TCO Calculator):

1. `/tools/tutorial-builder/` - Standalone HTML/JS
2. `/tools/use-case-showcase/` - Standalone HTML/JS
3. `/tools/sdk-examples-browser/` - Standalone HTML/JS
4. `/tools/performance-calculator/` - Standalone HTML/JS
5. `/tools/migration-assistant/` - Standalone HTML/JS (optional)

---

## ✅ Nächste Schritte

1. **Phase 4 Umsetzung:**
   - Tutorial Builder: Standalone + WordPress
   - Use Case Showcase: Standalone + WordPress (als Aggregator)

2. **Dokumentation aktualisieren:**
   - Dieses Dokument in Vorschläge einarbeiten
   - Klarstellen: Plugins = Aggregatoren, keine Code-Duplikation

3. **Pattern etablieren:**
   - Jedes Tool: Standalone + WordPress-Variante
   - Daten bleiben in Original-Locations
   - Aggregation statt Duplikation

---

**Dokument-Status:** ✅ Cross-Reference abgeschlossen  
**Nächste Schritte:** Aktualisierung der Vorschläge  
**Maintainer:** ThemisDB Development Team  
**Version:** 1.0.0  
**Datum:** 8. Januar 2026
