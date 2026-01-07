# MkDocs Themes für ThemisDB Kompendium

## Übersicht

Neben dem aktuell verwendeten **Material for MkDocs** gibt es mehrere alternative Themes, die für technische Dokumentation geeignet sind. Jedes Theme hat spezifische Vor- und Nachteile.

## 🎨 Empfohlene Themes

### 1. Material for MkDocs ⭐ (Aktuell verwendet)

**Website:** https://squidfunk.github.io/mkdocs-material/

**Vorteile:**
- ✅ Modernste Funktionen und aktive Entwicklung
- ✅ Hervorragende Mermaid-Diagramm-Unterstützung
- ✅ Responsive Design (Mobile, Tablet, Desktop)
- ✅ Dark/Light Mode Toggle
- ✅ Suchfunktion mit Highlighting
- ✅ Mehrsprachigkeit (Internationalization)
- ✅ Navigation Tabs und erweiterte Features
- ✅ Code-Highlighting mit Kopierfunktion
- ✅ Große Community und Plugins

**Nachteile:**
- ❌ Größere Bundle-Größe
- ❌ Einige Features nur in "Insiders" Version

**Installation:**
```bash
pip install mkdocs-material
```

**Konfiguration:**
```yaml
theme:
  name: material
  palette:
    - scheme: default
      primary: deep purple
      accent: amber
```

---

### 2. ReadTheDocs Theme

**Website:** https://www.mkdocs.org/user-guide/choosing-your-theme/#readthedocs

**Vorteile:**
- ✅ Klassisches, bewährtes Design
- ✅ Sehr performant (kleines Bundle)
- ✅ Einfache Konfiguration
- ✅ Gute Lesbarkeit
- ✅ In MkDocs integriert (keine Installation nötig)

**Nachteile:**
- ❌ Weniger moderne Features
- ❌ Kein Dark Mode
- ❌ Begrenzte Anpassungsmöglichkeiten

**Installation:**
```bash
# Bereits in MkDocs enthalten
```

**Konfiguration:**
```yaml
theme:
  name: readthedocs
  highlightjs: true
  hljs_languages:
    - python
    - rust
    - sql
```

---

### 3. MkDocs Bootswatch Themes

**Website:** https://mkdocs.github.io/mkdocs-bootswatch/

**Vorteile:**
- ✅ Viele verschiedene Farbschemata (14 Varianten)
- ✅ Bootstrap-basiert
- ✅ Responsive Design
- ✅ Gut für Corporate Design

**Varianten:**
- cerulean, cosmo, cyborg, darkly, flatly, journal
- litera, lumen, lux, materia, minty, pulse
- sandstone, simplex, slate, solar, spacelab, superhero
- united, yeti

**Nachteile:**
- ❌ Nicht mehr aktiv weiterentwickelt
- ❌ Weniger Features als Material

**Installation:**
```bash
pip install mkdocs-bootswatch
```

**Konfiguration:**
```yaml
theme:
  name: flatly  # oder andere Variante
```

---

### 4. GitBook Theme

**Website:** https://gitlab.com/lramage/mkdocs-gitbook-theme

**Vorteile:**
- ✅ GitBook-ähnliches Layout
- ✅ Sidebar-Navigation
- ✅ Sauberes, minimalistisches Design
- ✅ Gut für Dokumentations-Websites

**Nachteile:**
- ❌ Weniger Features als Material
- ❌ Kleinere Community

**Installation:**
```bash
pip install mkdocs-gitbook
```

**Konfiguration:**
```yaml
theme:
  name: gitbook
```

---

### 5. Ivory Theme

**Website:** https://github.com/daizutabi/mkdocs-ivory

**Vorteile:**
- ✅ Minimalistisches, elegantes Design
- ✅ Optimiert für technische Dokumentation
- ✅ Gute Typografie

**Nachteile:**
- ❌ Kleinere Community
- ❌ Weniger Funktionen

**Installation:**
```bash
pip install mkdocs-ivory
```

---

### 6. Windmill Theme

**Website:** https://github.com/gristlabs/mkdocs-windmill

**Vorteile:**
- ✅ Moderne, saubere Optik
- ✅ Gute Navigation
- ✅ Search Highlighting

**Nachteile:**
- ❌ Nicht mehr aktiv entwickelt

**Installation:**
```bash
pip install mkdocs-windmill
```

---

## 🎯 Empfehlung für ThemisDB Kompendium

### Aktuell: Material for MkDocs ⭐⭐⭐⭐⭐

**Begründung:**
1. **Mermaid-Diagramme:** Beste Integration und Rendering-Qualität
2. **PDF-Export:** Gut mit `mkdocs-with-pdf` Plugin
3. **Funktionsumfang:** Navigation Tabs, Suche, Dark Mode
4. **Community:** Große Nutzerbasis, viele Plugins verfügbar
5. **Wartung:** Aktive Entwicklung und regelmäßige Updates

### Alternative für PDF-Focus: ReadTheDocs

Falls PDF-Generierung Priorität hat:
- Kleinere PDF-Dateien
- Schnellere Generation
- Einfacheres Layout (weniger CSS-Konflikte)

**Wechsel zu ReadTheDocs:**
```yaml
theme:
  name: readthedocs
  
# Mermaid bleibt funktionsfähig über markdown_extensions
markdown_extensions:
  - pymdownx.superfences:
      custom_fences:
        - name: mermaid
          class: mermaid
          format: !!python/name:pymdownx.superfences.fence_code_format
```

---

## 🔧 Theme-Wechsel durchführen

### Schritt 1: Theme installieren
```bash
pip install <theme-name>
```

### Schritt 2: mkdocs-compendium.yml anpassen
```yaml
theme:
  name: <theme-name>
  # Theme-spezifische Optionen
```

### Schritt 3: Vorschau testen
```bash
mkdocs serve -f mkdocs-compendium.yml
```

### Schritt 4: PDF neu generieren
```bash
ENABLE_PDF_EXPORT=1 mkdocs build -f mkdocs-compendium.yml
```

---

## 📊 Theme-Vergleich

| Feature | Material | ReadTheDocs | Bootswatch | GitBook |
|---------|----------|-------------|------------|---------|
| Mermaid-Diagramme | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| PDF-Generierung | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| Moderne Features | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| Performance | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| Anpassbarkeit | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| Dokumentation | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| Community | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐ |

---

## 🎨 Custom Themes

### Eigenes Theme erstellen

MkDocs erlaubt auch die Erstellung eigener Themes:

```yaml
theme:
  name: null
  custom_dir: 'custom_theme/'
  
extra_css:
  - stylesheets/extra.css
```

**Vorteile:**
- Vollständige Kontrolle über Design
- Optimiert für spezifische Anforderungen
- Keine Abhängigkeiten von Drittanbieter-Themes

**Nachteile:**
- Höherer Wartungsaufwand
- Keine automatischen Updates
- Mehr Entwicklungszeit

---

## 💡 Fazit

**Für ThemisDB Kompendium empfohlen:**

1. **Online-Dokumentation:** Material for MkDocs (aktuell)
   - Beste User Experience
   - Mermaid-Diagramme rendern perfekt
   - Modern und wartungsfreundlich

2. **PDF-Generierung:** WeasyPrint-Script (aktuell verwendet)
   - Schneller als mkdocs-with-pdf
   - Bessere Kontrolle über Layout
   - Kleinere Dateigrößen

3. **Alternative:** ReadTheDocs Theme
   - Wenn Einfachheit Priorität hat
   - Für schnellere Build-Zeiten
   - Für klassisches Look & Feel

---

**Erstellt:** 2025-12-31  
**Version:** 1.0  
**Autor:** ThemisDB Documentation Team
