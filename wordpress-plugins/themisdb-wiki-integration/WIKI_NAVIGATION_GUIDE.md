# ThemisDB Wiki Navigation - Verwendungsbeispiele

**Version:** 1.1.0  
**Feature:** Wiki-Navigation aus _Sidebar.md  
**Datum:** 07. Januar 2026

---

## Übersicht

Die neue Wiki-Navigation verwendet die `_Sidebar.md` Datei aus dem GitHub-Repository als Quelle für eine strukturierte Dokumentations-Navigation in WordPress.

---

## Navigationsstile

### 1. Sidebar-Stil (Standard)

**Verwendung:**
```php
[themisdb_wiki_nav lang="de" style="sidebar"]
```

**Aussehen:**
```
┌─────────────────────────────────────┐
│ 📚 Dokumentation                     │
├─────────────────────────────────────┤
│                                     │
│ ### 📋 Schnellstart                 │
│ • Übersicht                         │
│ • Home                              │
│ • Dokumentations-Index              │
│                                     │
│ ### 🚀 v1.4.0-alpha Release         │
│ • Release Notes                     │
│ • Changelog                         │
│ • Kompendium Update                 │
│                                     │
│ ### 📝 Neue Features                │
│ • Grammatik-gesteuerte Generierung  │
│ • RoPE Scaling                      │
│ • Vision Support                    │
│ • Flash Attention                   │
│                                     │
│ ### 📚 Dokumentation                │
│ • Sachstandsbericht 2025            │
│ • Features                          │
│ • Roadmap                           │
│                                     │
│ ... (weitere Sections)              │
└─────────────────────────────────────┘
```

**Eigenschaften:**
- Klassische Seitenleisten-Darstellung
- Alle Sections sichtbar
- Ideal für breite Sidebars
- Übersichtlich für viele Links

---

### 2. Accordion-Stil

**Verwendung:**
```php
[themisdb_wiki_nav lang="de" style="accordion"]
```

**Aussehen:**
```
┌─────────────────────────────────────┐
│ 📚 Dokumentation                     │
├─────────────────────────────────────┤
│                                     │
│ ### 📋 Schnellstart             ▼   │
│ • Übersicht                         │
│ • Home                              │
│ • Dokumentations-Index              │
│                                     │
│ ### 🚀 v1.4.0-alpha Release     ▶   │
│ (eingeklappt)                       │
│                                     │
│ ### 📝 Neue Features            ▼   │
│ • Grammatik-gesteuerte Generierung  │
│ • RoPE Scaling                      │
│ • Vision Support                    │
│                                     │
│ ### 📚 Dokumentation            ▶   │
│ (eingeklappt)                       │
│                                     │
│ ... (weitere Sections)              │
└─────────────────────────────────────┘
```

**Eigenschaften:**
- Einklappbare Sections
- Platzsparen bei vielen Kategorien
- Click auf Titel zum Ein-/Ausklappen
- Smooth Animations
- Aktuelle Section automatisch ausgeklappt

---

### 3. Horizontal-Stil

**Verwendung:**
```php
[themisdb_wiki_nav lang="de" style="horizontal"]
```

**Aussehen:**
```
┌──────────────────────────────────────────────────────────┐
│ Schnellstart:  Übersicht | Home | Index                  │
│ Release:       Notes | Changelog | Update                │
│ Features:      Grammatik | RoPE | Vision | Flash         │
│ Docs:          Bericht | Features | Roadmap              │
└──────────────────────────────────────────────────────────┘
```

**Eigenschaften:**
- Kompakte horizontale Darstellung
- Sections nebeneinander
- Ideal für Header oder Footer
- Responsive für Mobile

---

## Integration in WordPress

### Option 1: Shortcode in Seite

```php
<!-- In WordPress Seite/Post Editor -->
<div class="documentation-layout">
    <aside class="sidebar">
        [themisdb_wiki_nav lang="de" style="accordion"]
    </aside>
    
    <main class="content">
        [themisdb_wiki file="README.md" lang="de" show_toc="yes"]
    </main>
</div>
```

### Option 2: Widget in Seitenleiste

**Schritte:**
1. WordPress Admin → **Design** → **Widgets**
2. Widget "ThemisDB Wiki Navigation" suchen
3. In gewünschten Widget-Bereich ziehen
4. Konfigurieren:
   - Titel: "Dokumentation"
   - Sprache: Deutsch (DE)
   - Stil: Accordion
5. **Speichern**

### Option 3: Im Theme Template

```php
<?php
// In sidebar.php oder page-documentation.php
if (function_exists('do_shortcode')) {
    echo do_shortcode('[themisdb_wiki_nav lang="de" style="sidebar"]');
}
?>
```

---

## Verwendungsszenarien

### Szenario 1: Vollständige Dokumentationsseite

**WordPress-Seite:** `/dokumentation/`

```php
<!-- Zwei-Spalten-Layout -->
<div class="doc-container" style="display: grid; grid-template-columns: 300px 1fr; gap: 2rem;">
    <!-- Navigation (links) -->
    <aside class="doc-sidebar">
        [themisdb_wiki_nav lang="de" style="accordion"]
    </aside>
    
    <!-- Inhalt (rechts) -->
    <main class="doc-content">
        [themisdb_wiki file="README.md" lang="de" show_toc="yes"]
    </main>
</div>
```

### Szenario 2: Dokumentations-Hub

**WordPress-Seite:** `/docs/`

```php
<!-- Zentrale Übersicht -->
<h1>ThemisDB Dokumentation</h1>
<p>Willkommen zur ThemisDB-Dokumentation. Wählen Sie einen Bereich:</p>

<!-- Navigation als Grid -->
[themisdb_wiki_nav lang="de" style="sidebar"]

<!-- Oder Dokumentenliste -->
[themisdb_docs lang="de" layout="grid"]
```

### Szenario 3: Mehrsprachige Site

```php
<?php
// Sprache aus WordPress-Locale ableiten
$lang = (strpos(get_locale(), 'de') === 0) ? 'de' : 'en';
?>

<!-- Dynamische Sprachwahl -->
[themisdb_wiki_nav lang="<?php echo $lang; ?>" style="sidebar"]
[themisdb_wiki file="README.md" lang="<?php echo $lang; ?>"]
```

---

## Anpassung per CSS

### Farben ändern

```css
/* Navigation-Container */
.themisdb-wiki-nav {
    background: #ffffff; /* Hintergrund */
    border-color: #cccccc; /* Rahmen */
}

/* Section-Titel */
.themisdb-nav-section-title {
    color: #333333;
    border-bottom-color: #0969da; /* Unterstrich */
}

/* Links */
.themisdb-nav-item a {
    color: #0969da; /* Link-Farbe */
}

.themisdb-nav-item a:hover {
    background: #f0f0f0; /* Hover-Hintergrund */
    color: #0550ae; /* Hover-Farbe */
}

/* Aktive Seite */
.themisdb-nav-item a.current-page {
    background: #0969da; /* Highlight-Hintergrund */
    color: #ffffff; /* Highlight-Text */
}
```

### Abstände anpassen

```css
/* Kompaktere Darstellung */
.themisdb-wiki-nav {
    padding: 1rem; /* Weniger Padding */
}

.themisdb-nav-section {
    margin-bottom: 1rem; /* Weniger Abstand zwischen Sections */
}

.themisdb-nav-item a {
    padding: 0.25rem 0.5rem; /* Kleinere Links */
    font-size: 0.875rem; /* Kleinere Schrift */
}
```

### Custom Theme-Integration

```css
/* Anpassung an WordPress-Theme */
.themisdb-wiki-nav {
    /* Übernimm Theme-Farben */
    background: var(--theme-sidebar-bg, #f6f8fa);
    border-color: var(--theme-border-color, #d0d7de);
}

/* Mobile Responsiveness */
@media (max-width: 768px) {
    .themisdb-wiki-nav-accordion .themisdb-nav-section-title {
        font-size: 0.9rem;
    }
    
    .themisdb-nav-item a {
        padding: 0.5rem;
    }
}
```

---

## Technische Details

### Parsing von _Sidebar.md

Das Plugin liest die `_Sidebar.md` Datei und parsed sie wie folgt:

**Markdown-Struktur:**
```markdown
### 📋 Schnellstart
- [Übersicht](index.md)
- [Home](home.md)

#### Erweitert
- [API Docs](api/README.md)

### 📚 Dokumentation
- [Features](features.md)
```

**Generiertes HTML:**
```html
<nav class="themisdb-wiki-nav">
    <div class="themisdb-nav-section">
        <h3 class="themisdb-nav-section-title">📋 Schnellstart</h3>
        <ul class="themisdb-nav-section-items">
            <li class="themisdb-nav-item">
                <a href="?doc=index">Übersicht</a>
            </li>
            <li class="themisdb-nav-item">
                <a href="?doc=home">Home</a>
            </li>
            <li class="themisdb-nav-subsection">
                <span class="themisdb-nav-subsection-title">Erweitert</span>
                <ul class="themisdb-nav-subsection-items">
                    <li class="themisdb-nav-item">
                        <a href="?doc=api/README">API Docs</a>
                    </li>
                </ul>
            </li>
        </ul>
    </div>
    
    <div class="themisdb-nav-section">
        <h3 class="themisdb-nav-section-title">📚 Dokumentation</h3>
        <ul class="themisdb-nav-section-items">
            <li class="themisdb-nav-item">
                <a href="?doc=features">Features</a>
            </li>
        </ul>
    </div>
</nav>
```

### URL-Generierung

Links werden automatisch WordPress-kompatibel gemacht:

**GitHub-Pfad:** `docs/en/features/FEATURES.md`  
**WordPress-URL:** `?doc=en/features/FEATURES`

Für saubere URLs kann ein Custom Rewrite verwendet werden.

---

## Erweiterte Konfiguration

### Custom URL-Struktur

```php
// In functions.php des Themes
add_filter('themisdb_wiki_nav_url', 'custom_doc_url', 10, 2);

function custom_doc_url($url, $github_path) {
    // Erzeuge saubere URLs
    $slug = str_replace('.md', '', $github_path);
    return home_url('/docs/' . $slug);
}
```

### Navigation filtern

```php
// In functions.php
add_filter('themisdb_wiki_nav_sections', 'filter_nav_sections', 10, 1);

function filter_nav_sections($sections) {
    // Nur bestimmte Sections anzeigen
    $allowed = ['Schnellstart', 'Dokumentation', 'API'];
    // ... Filter-Logik
    return $sections;
}
```

---

## Troubleshooting

### Problem: Navigation wird nicht angezeigt

**Ursache:** _Sidebar.md nicht gefunden

**Lösung:**
1. Prüfen Sie die Plugin-Einstellungen:
   - Repository: `makr-code/wordpressPlugins` ✓
   - Branch: `main` ✓
   - Docs Path: `docs` ✓
2. Manuell testen: https://github.com/makr-code/wordpressPlugins/blob/main/docs/_Sidebar.md
3. "Sync Now" im Admin-Panel klicken

### Problem: Links funktionieren nicht

**Ursache:** WordPress-Seiten für Dokumente fehlen

**Lösung:**
Erstellen Sie WordPress-Seiten mit entsprechendem Shortcode:
```php
<!-- Seite: /docs/features -->
[themisdb_wiki file="features/FEATURES.md" lang="de"]
```

### Problem: Styling passt nicht zum Theme

**Lösung:**
Custom CSS in **Design → Customizer → Zusätzliches CSS**:
```css
.themisdb-wiki-nav {
    background: inherit;
    border: none;
}
```

---

## Best Practices

### 1. Navigation persistent in Sidebar

**Widgets verwenden:**
- Widget "ThemisDB Wiki Navigation" in Sidebar platzieren
- Auf allen Dokumentations-Seiten sichtbar
- Einheitliche Navigation

### 2. Accordion für viele Kategorien

**Bei > 10 Sections:**
```php
[themisdb_wiki_nav style="accordion"]
```
- Spart Platz
- Übersichtlicher
- Bessere UX

### 3. Breadcrumb-Navigation ergänzen

```php
<!-- Breadcrumb -->
<nav class="breadcrumb">
    <a href="/">Home</a> » 
    <a href="/docs/">Dokumentation</a> » 
    <span>Features</span>
</nav>

<!-- Wiki-Navigation -->
[themisdb_wiki_nav lang="de" style="sidebar"]
```

---

## Zusammenfassung

Die neue Wiki-Navigation-Funktion macht die `_Sidebar.md` zum **zentralen Ausgangspunkt** für die Dokumentations-Navigation in WordPress. Sie:

✅ Liest automatisch die Struktur aus GitHub  
✅ Bietet drei flexible Anzeigestile  
✅ Integriert sich nahtlos als Shortcode oder Widget  
✅ Highlightet die aktuelle Seite  
✅ Ist responsive und theme-kompatibel  
✅ Cached für Performance  

**Empfehlung:** Verwenden Sie den Accordion-Stil in der Sidebar für die beste Balance zwischen Übersichtlichkeit und Funktionalität.

---

**Version:** 1.1.0  
**Autor:** ThemisDB Team  
**Datum:** 07. Januar 2026
