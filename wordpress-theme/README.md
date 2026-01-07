# ThemisDB WordPress Theme

Ein modernes, professionelles WordPress-Theme mit Themis-Markenfarben.

## 🎨 Design

Das Theme folgt Best Practices und State-of-the-Art-Standards, inspiriert vom midnight-blogger Theme:

- **Moderne, saubere Gestaltung**
- **Themis-Markenfarben** (Primary: #2c3e50, Secondary: #3498db, Accent: #7c4dff)
- **Responsive Design** (Mobile-First)
- **Barrierefreiheit** (WCAG 2.1 AA)
- **Optimiert für technische Inhalte**

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
- **[themisdb/README.md](themisdb/README.md)** - Theme-spezifische Dokumentation

## ✨ Hauptmerkmale

### Design & Layout
- ✅ Modernes Card-basiertes Layout
- ✅ Gradient-Header mit Themis-Farben
- ✅ Responsive auf allen Geräten
- ✅ Mobile Navigation mit Hamburger-Menü
- ✅ Sidebar + 3 Footer Widget-Bereiche

### WordPress-Integration
- ✅ Gutenberg Block Editor Support
- ✅ Custom Color Palette
- ✅ Editor Styles (WYSIWYG)
- ✅ Post Thumbnails
- ✅ Navigation Menus
- ✅ Widget Areas
- ✅ Custom Logo & Background

### Performance & SEO
- ✅ Minimales CSS/JS (~17KB)
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

## 🎨 Themis-Farben

```css
--primary-color: #2c3e50;      /* Dunkles Blau-Grau */
--secondary-color: #3498db;    /* Helles Blau */
--accent-purple: #7c4dff;      /* Lila */
--success-color: #27ae60;      /* Grün */
--warning-color: #f39c12;      /* Orange */
```

## 📁 Theme-Struktur

```
themisdb/
├── style.css                  # Haupt-Stylesheet (16.697 Zeilen)
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
│   └── navigation.js          # Navigation & Interaktivität
└── template-parts/
    ├── content.php            # Post-Template
    ├── content-single.php     # Einzelpost-Template
    ├── content-page.php       # Seiten-Template
    ├── content-search.php     # Such-Template
    └── content-none.php       # Keine Ergebnisse
```

**Gesamt**: 24 Dateien, ~2.200 Zeilen Code

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

## 📋 Requirements

- WordPress 6.0+
- PHP 7.4+
- Moderne Browser (Chrome, Firefox, Safari, Edge)

## 🔌 Empfohlene Plugins

- **Contact Form 7** - Kontaktformulare
- **Yoast SEO** - SEO-Optimierung
- **WP Super Cache** - Performance
- **Smush** - Bildoptimierung

## 🌐 Browser-Support

- Chrome (neueste 2)
- Firefox (neueste 2)
- Safari (neueste 2)
- Edge (neueste 2)
- Mobile Browsers

## 📝 Lizenz

MIT License - Copyright (c) 2024 ThemisDB Team

## 📞 Support

- **GitHub**: https://github.com/makr-code/ThemisDB
- **Issues**: Erstelle ein GitHub Issue

## 🎯 Status

✅ **Production Ready** (nach Screenshot-Erstellung)

- [x] Alle WordPress-Standards erfüllt
- [x] Themis-Markenfarben integriert
- [x] Modern & professionell gestaltet
- [x] Barrierefrei (WCAG 2.1 AA)
- [x] SEO-freundlich
- [x] Performance-optimiert
- [x] Vollständig dokumentiert
- [ ] Screenshot.png (1200x900px)

## 📚 Weitere Dokumentation

- [Installation & Nutzung (DE/EN)](INSTALLATION_GUIDE.md)
- [Projekt-Zusammenfassung](THEME_SUMMARY.md)
- [Theme README](themisdb/README.md)
