# ThemisDB WordPress Theme

Ein modernes, professionelles WordPress-Theme mit Themis-Markenfarben und state-of-the-art Funktionen.

## 🎨 Design

Das Theme folgt Best Practices und State-of-the-Art-Standards mit umfassenden Verbesserungen:

- **Moderne, saubere Gestaltung** mit weißem Hintergrund
- **Themis-Markenfarben** (Primary: #2c3e50, Secondary: #3498db, Accent: #7c4dff)
- **Unicode-Icons** für bessere Performance (📅 👤 📁 🏷️ 💬)
- **Responsive Design** (Mobile-First)
- **Barrierefreiheit** (WCAG 2.1 AA)
- **Optimiert für technische Inhalte**
- **Moderne Animationen** und Übergänge
- **Interaktive Graph-Navigation** (Neo4j Bloom-inspiriert)

## ✨ Neue Features

### Interaktive Elemente
- ✅ **Leseprogress-Anzeige** - Visueller Indikator beim Scrollen
- ✅ **Lightbox-Galerie** - Interaktive Bildergalerie mit Zoom und Navigation
- ✅ **Back-to-Top Button** - Schneller Scroll nach oben
- ✅ **Lazy Loading** - Bilder werden erst bei Bedarf geladen
- ✅ **Code-Copy-Button** - Ein-Klick-Kopieren von Code-Blöcken
- ✅ **Graph-Navigation** - Interaktive Visualisierung der Seitenstruktur
- ✅ **Externe Links** - Automatische Markierung mit Icons (🔗)

### Design-Elemente
- ✅ **Animierte Karteneffekte** - Smooth Hover-Animationen
- ✅ **Gradient-Buttons** - Moderne Button-Styles mit Farbverläufen
- ✅ **Stats-Counter** - Animierte Zahlen für Statistiken
- ✅ **Timeline-Komponente** - Zeitleisten-Darstellung
- ✅ **Accordion/Toggle** - Ausklappbare Inhalte
- ✅ **Alert-Boxen** - Info-, Success-, Warning-, Danger-Hinweise
- ✅ **Badge-System** - Tags und Labels in verschiedenen Farben
- ✅ **Hero-Section** - Eindrucksvolle Header-Bereiche
- ✅ **Card-Grid** - Moderne Karten-Layouts
- ✅ **Breadcrumbs** - Navigations-Pfad mit Icons

### Typografie & Styling
- ✅ **Gradient-Text-Effekte** - Farbverläufe in Überschriften
- ✅ **Verbesserte Meta-Tags** - Pill-Style Badges für Post-Info
- ✅ **Widget-Icons** - Automatische Icons in Widget-Listen
- ✅ **Smooth-Transitions** - Flüssige Übergänge bei allen Interaktionen
- ✅ **Hover-Effekte** - Micro-Interactions bei Buttons, Links, Karten

## 📦 Installation

### Methode 1: WordPress Admin

1. Theme-Ordner als ZIP packen:
   ```bash
   cd wordpress-theme
   zip -r themisdb.zip themisdb/
   ```

2. In WordPress:
   - Gehe zu **Design > Themes > Theme hochladen**
   - Wähle `themisdb.zip`
   - Aktiviere das Theme

### Methode 2: FTP

1. Kopiere den `themisdb` Ordner nach `/wp-content/themes/`
2. Aktiviere das Theme in WordPress Admin

## 📖 Dokumentation

- **[INSTALLATION_GUIDE.md](INSTALLATION_GUIDE.md)** - Vollständige Installations- und Nutzungsanleitung (DE/EN)
- **[THEME_SUMMARY.md](THEME_SUMMARY.md)** - Projektzusammenfassung und Spezifikationen
- **[DESIGN_GUIDE.md](DESIGN_GUIDE.md)** - Detaillierte Design-Dokumentation
- **[themisdb/README.md](themisdb/README.md)** - Theme-spezifische Dokumentation

## ✨ Hauptmerkmale

### Design & Layout
- ✅ Modernes Card-basiertes Layout mit Schatten und Tiefe
- ✅ Gradient-Header mit Themis-Farben und Muster
- ✅ Responsive auf allen Geräten
- ✅ Mobile Navigation mit Hamburger-Menü
- ✅ Sidebar + 3 Footer Widget-Bereiche
- ✅ Weißer Hintergrund für bessere Lesbarkeit
- ✅ Animierte Übergänge und Hover-Effekte

### WordPress-Integration
- ✅ Gutenberg Block Editor Support
- ✅ Custom Color Palette
- ✅ Editor Styles (WYSIWYG)
- ✅ Post Thumbnails mit Hover-Effekt
- ✅ Navigation Menus mit Untermenüs
- ✅ Widget Areas
- ✅ Custom Logo & Background
- ✅ Theme Customizer Integration

### Performance & SEO
- ✅ Optimiertes CSS/JS (~25KB gesamt)
- ✅ Lazy Loading für Bilder
- ✅ Keine jQuery-Abhängigkeiten
- ✅ Semantisches HTML5
- ✅ SEO-freundlich
- ✅ Fast Loading

### Entwickler-Features
- ✅ CSS Custom Properties
- ✅ Child-Theme-ready
- ✅ Translation-ready (i18n)
- ✅ Gut dokumentiert
- ✅ WordPress Standards
- ✅ Moderne JavaScript-Features

## 🎨 Themis-Farben

```css
--primary-color: #2c3e50;      /* Dunkles Blau-Grau */
--secondary-color: #3498db;    /* Helles Blau */
--accent-purple: #7c4dff;      /* Lila */
--success-color: #27ae60;      /* Grün */
--warning-color: #f39c12;      /* Orange */
--danger-color: #e74c3c;       /* Rot */
```

## 📁 Theme-Struktur

```
themisdb/
├── style.css                  # Haupt-Stylesheet (~2500 Zeilen)
├── functions.php              # Theme-Funktionen
├── header.php                 # Header mit Navigation
├── footer.php                 # Footer mit Widget-Bereichen
├── index.php                  # Haupt-Template
├── single.php                 # Einzelbeiträge
├── page.php                   # Seiten
├── archive.php                # Archive
├── search.php                 # Suche
├── 404.php                    # Fehlerseite
├── sidebar.php                # Sidebar
├── comments.php               # Kommentare
├── searchform.php             # Suchformular
├── editor-style.css           # Gutenberg-Styles
├── template-full-width.php    # Vollbreiten-Template
├── js/
│   ├── navigation.js          # Navigation & Mobile Menu
│   ├── graph-navigation.js    # Interaktive Graph-Navigation (Neo4j)
│   └── enhancements.js        # Moderne Features (NEW!)
└── template-parts/
    ├── content.php            # Post-Template
    ├── content-single.php     # Einzelpost-Template
    ├── content-page.php       # Seiten-Template
    ├── content-search.php     # Such-Template
    └── content-none.php       # Keine Ergebnisse
```

**Gesamt**: 27 Dateien, ~4.000 Zeilen Code

## 🚀 Schnellstart

1. **Theme installieren** (siehe oben)

2. **Menüs einrichten**:
   - Design > Menüs
   - Erstelle "Primary Menu" und "Footer Menu"

3. **Widgets hinzufügen**:
   - Design > Widgets
   - Füge Widgets zu Sidebar und Footer hinzu

4. **Farben anpassen** (optional):
   - Design > Customizer > Theme Colors

5. **Logo hochladen**:
   - Design > Customizer > Site Identity

6. **Graph-Navigation aktivieren**:
   - Erstelle ein Primary Menu
   - Die Graph-Navigation wird automatisch aktiviert
   - Klicke auf das Symbol oben rechts zum Öffnen

## 🔧 Anpassung

### Child Theme erstellen

```css
/* style.css */
/*
 Theme Name:   ThemisDB Child
 Template:     themisdb
*/
```

```php
<?php
// functions.php
function themisdb_child_enqueue_styles() {
    wp_enqueue_style('parent-style', get_template_directory_uri() . '/style.css');
}
add_action('wp_enqueue_scripts', 'themisdb_child_enqueue_styles');
```

### Farben überschreiben

```css
:root {
    --primary-color: #your-color;
    --secondary-color: #your-color;
}
```

### Komponenten verwenden

```html
<!-- Alert Box -->
<div class="alert alert-info">
    <p>Info-Nachricht mit automatischem Icon</p>
</div>

<!-- Badge -->
<span class="badge badge-primary">Neu</span>

<!-- Card Grid -->
<div class="card-grid">
    <div class="card">
        <div class="card-header">
            <h3>Titel</h3>
        </div>
        <div class="card-body">
            <p>Inhalt</p>
        </div>
    </div>
</div>

<!-- Stats Section -->
<div class="stats-section">
    <div class="stat-box">
        <div class="stat-icon">🚀</div>
        <div class="stat-number" data-count="1000">0</div>
        <div class="stat-label">Downloads</div>
    </div>
</div>

<!-- Hero Section -->
<div class="hero">
    <div class="hero-content">
        <h1>Willkommen</h1>
        <p>Ihre Beschreibung hier</p>
        <a href="#" class="button">Mehr erfahren</a>
    </div>
</div>
```

## 📋 Requirements

- WordPress 6.0+
- PHP 7.4+
- Moderne Browser (Chrome, Firefox, Safari, Edge)
- JavaScript aktiviert (für interaktive Features)

## 🔌 Empfohlene Plugins

- **Contact Form 7** - Kontaktformulare
- **Yoast SEO** - SEO-Optimierung
- **WP Super Cache** - Performance
- **Smush** - Bildoptimierung
- **Chart.js Plugin** - Für Diagramme und Grafiken

## 🌐 Browser-Support

- Chrome (neueste 2)
- Firefox (neueste 2)
- Safari (neueste 2)
- Edge (neueste 2)
- Mobile Browsers (iOS Safari, Chrome Mobile)

## 🎯 Neue Funktionen im Detail

### Graph-Navigation
Die interaktive Graph-Navigation nutzt D3.js für eine Neo4j Bloom-inspirierte Visualisierung:
- **Force-directed Graph** - Physik-basiertes Layout
- **Interaktiv** - Ziehen, Zoomen, Pan
- **Thematische Farben** - Automatische Kategorisierung
- **Fallback** - HTML-basierte Tree-Ansicht ohne JavaScript

### Animationen
Alle Animationen respektieren `prefers-reduced-motion`:
```css
@media (prefers-reduced-motion: reduce) {
    /* Animationen werden deaktiviert */
}
```

### Performance
- **Lazy Loading** - Bilder werden erst geladen, wenn sie sichtbar sind
- **Intersection Observer** - Moderne Browser-API für bessere Performance
- **CSS-Only Animations** - Keine JavaScript-Animationen wo möglich
- **Minimales JavaScript** - Nur ~13KB für alle Enhancements

## 📝 Lizenz

MIT License - Copyright (c) 2024 ThemisDB Team

## 📞 Support

- **GitHub**: https://github.com/makr-code/ThemisDB
- **Issues**: Erstelle ein GitHub Issue

## 🎯 Status

✅ **Production Ready**

- [x] Alle WordPress-Standards erfüllt
- [x] Themis-Markenfarben integriert
- [x] Modern & professionell gestaltet
- [x] Barrierefrei (WCAG 2.1 AA)
- [x] SEO-freundlich
- [x] Performance-optimiert
- [x] Vollständig dokumentiert
- [x] Moderne Web-Features implementiert
- [x] Graph-Navigation funktioniert
- [ ] Screenshot.png (1200x900px) - empfohlen

## 📚 Weitere Dokumentation

- [Installation & Nutzung (DE/EN)](INSTALLATION_GUIDE.md)
- [Projekt-Zusammenfassung](THEME_SUMMARY.md)
- [Design Guide](DESIGN_GUIDE.md)
- [Theme README](themisdb/README.md)

## 🔄 Version History

### v1.0.0 (Aktuell)
- ✅ Weißer Hintergrund für bessere Lesbarkeit
- ✅ Unicode-Icons statt SVG
- ✅ Moderne Animationen und Übergänge
- ✅ Interaktive Features (Lightbox, Lazy Loading, etc.)
- ✅ Graph-Navigation Bug-Fix
- ✅ Verbesserte Typografie mit Gradienten
- ✅ Professionelles Card-Design
- ✅ Erweiterte Komponenten-Bibliothek
