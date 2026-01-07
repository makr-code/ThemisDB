# ThemisDB WordPress Theme - Installation & Usage Guide

## Installationsanleitung (German)

### Voraussetzungen

- WordPress 6.0 oder höher
- PHP 7.4 oder höher
- Empfohlene Plugins:
  - Gutenberg/Block Editor (integriert)
  - Contact Form 7 (für Kontaktformulare)
  - Yoast SEO (für erweiterte SEO-Funktionen)

### Installation

#### Methode 1: Upload über WordPress Admin

1. Laden Sie das Theme-Paket herunter
2. Gehen Sie zu WordPress Admin > Design > Themes
3. Klicken Sie auf "Theme hinzufügen" > "Theme hochladen"
4. Wählen Sie die ZIP-Datei des Themes aus
5. Klicken Sie auf "Jetzt installieren"
6. Aktivieren Sie das Theme nach der Installation

#### Methode 2: Manuelle Installation

1. Entpacken Sie die Theme-Dateien
2. Laden Sie den Ordner `themisdb` per FTP in `/wp-content/themes/` hoch
3. Gehen Sie zu WordPress Admin > Design > Themes
4. Aktivieren Sie das ThemisDB Theme

### Erste Schritte nach der Installation

#### 1. Menüs einrichten

1. Gehen Sie zu **Design > Menüs**
2. Erstellen Sie ein neues Menü für die Hauptnavigation
3. Fügen Sie Seiten, Kategorien oder benutzerdefinierte Links hinzu
4. Weisen Sie das Menü dem Bereich "Primary Menu" zu
5. Optional: Erstellen Sie ein Footer-Menü für den Fußbereich

#### 2. Widgets konfigurieren

Das Theme bietet folgende Widget-Bereiche:

- **Sidebar**: Hauptseitenleiste (rechts)
- **Footer Widget 1, 2, 3**: Drei Spalten im Footer

So fügen Sie Widgets hinzu:
1. Gehen Sie zu **Design > Widgets**
2. Ziehen Sie Widgets in die gewünschten Bereiche
3. Empfohlene Widgets:
   - Sidebar: Suche, Kategorien, Letzte Beiträge, Tags
   - Footer: Über uns, Kontaktinformationen, Social Media Links

#### 3. Theme-Farben anpassen

1. Gehen Sie zu **Design > Customizer**
2. Navigieren Sie zu **Theme Colors**
3. Passen Sie die Farben an:
   - **Primärfarbe** (Primary Color): Standard #2c3e50
   - **Sekundärfarbe** (Secondary Color): Standard #3498db
   - **Akzentfarbe** (Accent Color): Standard #7c4dff

#### 4. Logo hochladen

1. Gehen Sie zu **Design > Customizer > Website-Identität**
2. Klicken Sie auf "Logo auswählen"
3. Laden Sie Ihr Logo hoch (empfohlene Größe: 200x60px)
4. Optional: Laden Sie auch ein Site-Icon (Favicon) hoch

#### 5. Beitragsbilder verwenden

Das Theme unterstützt Beitragsbilder (Featured Images):
1. Bearbeiten Sie einen Beitrag oder eine Seite
2. Klicken Sie rechts auf "Beitragsbild festlegen"
3. Wählen Sie ein Bild aus oder laden Sie ein neues hoch
4. Empfohlene Größe: 1200x675px

### Theme-Funktionen

#### Vollbreiten-Template

Für Seiten ohne Sidebar:
1. Bearbeiten Sie eine Seite
2. Wählen Sie rechts unter "Seitenattribute" das Template "Full Width"
3. Speichern Sie die Seite

#### Custom Post Types Support

Das Theme unterstützt alle Standard-WordPress-Post-Types und Custom Post Types.

#### Block Editor (Gutenberg) Integration

Das Theme ist vollständig mit dem Gutenberg Editor kompatibel und bietet:
- Custom Farbpalette mit Themis-Farben
- Optimierte Styles für alle Standard-Blöcke
- Wide und Full Width Alignment Support
- Editor-Styles für WYSIWYG-Erlebnis

#### Code-Blöcke für technische Inhalte

Das Theme ist optimiert für technische Dokumentation:
- Syntax-Highlighting-freundliche Styles
- Optimierte Code-Block-Darstellung
- Monospace-Schrift für Inline-Code
- Dunkler Hintergrund für Pre-Blöcke

### Anpassung und Erweiterung

#### Child Theme erstellen

Um das Theme sicher anzupassen:

1. Erstellen Sie einen neuen Ordner: `/wp-content/themes/themisdb-child/`

2. Erstellen Sie `style.css`:
```css
/*
 Theme Name:   ThemisDB Child
 Description:  Child theme for ThemisDB
 Author:       Ihr Name
 Template:     themisdb
 Version:      1.0.0
*/

/* Ihre benutzerdefinierten Styles hier */
```

3. Erstellen Sie `functions.php`:
```php
<?php
function themisdb_child_enqueue_styles() {
    wp_enqueue_style( 'parent-style', get_template_directory_uri() . '/style.css' );
    wp_enqueue_style( 'child-style', get_stylesheet_uri(), array('parent-style') );
}
add_action( 'wp_enqueue_scripts', 'themisdb_child_enqueue_styles' );
```

4. Aktivieren Sie das Child Theme in WordPress Admin

#### CSS-Variablen überschreiben

In Ihrem Child Theme können Sie CSS-Variablen überschreiben:

```css
:root {
    --primary-color: #your-color;
    --secondary-color: #your-color;
    --accent-purple: #your-color;
}
```

### Fehlerbehebung

#### Problem: Menü wird nicht angezeigt
**Lösung**: Stellen Sie sicher, dass Sie ein Menü erstellt und dem Bereich "Primary Menu" zugewiesen haben.

#### Problem: Featured Images werden nicht angezeigt
**Lösung**: Aktivieren Sie Post Thumbnails Support in `functions.php` (bereits aktiviert im Theme).

#### Problem: Sidebar erscheint nicht
**Lösung**: Fügen Sie Widgets zur Sidebar hinzu unter Design > Widgets.

#### Problem: Mobile-Menü funktioniert nicht
**Lösung**: Stellen Sie sicher, dass JavaScript im Browser aktiviert ist.

### Performance-Optimierung

#### Empfohlene Plugins
- **WP Super Cache** oder **W3 Total Cache**: Caching
- **Smush** oder **ShortPixel**: Bildoptimierung
- **Autoptimize**: CSS/JS-Optimierung

#### Empfohlene Einstellungen
- Verwenden Sie ein CDN für statische Ressourcen
- Aktivieren Sie GZIP-Komprimierung
- Optimieren Sie Bilder vor dem Upload
- Verwenden Sie Lazy Loading für Bilder

### SEO-Best Practices

1. **Permalinks**: Verwenden Sie SEO-freundliche URLs (Einstellungen > Permalinks)
2. **Meta-Descriptions**: Nutzen Sie ein SEO-Plugin wie Yoast SEO
3. **Alt-Text**: Fügen Sie beschreibenden Alt-Text zu allen Bildern hinzu
4. **Überschriften-Hierarchie**: Verwenden Sie H1-H6 korrekt
5. **Interne Verlinkung**: Verlinken Sie verwandte Beiträge

### Barrierefreiheit

Das Theme folgt WCAG 2.1 Level AA Standards:
- Tastaturnavigation unterstützt
- Screen-Reader-freundlich
- Ausreichender Farbkontrast
- ARIA-Landmarks implementiert
- Skip-to-Content-Link vorhanden

### Support und Updates

- **GitHub Repository**: https://github.com/makr-code/ThemisDB
- **Dokumentation**: Siehe Repository-Docs
- **Issues**: Erstellen Sie ein GitHub Issue für Fehlerberichte

### Lizenz

Das Theme ist unter der MIT-Lizenz veröffentlicht.

---

## Installation Guide (English)

### Requirements

- WordPress 6.0 or higher
- PHP 7.4 or higher
- Recommended plugins:
  - Gutenberg/Block Editor (built-in)
  - Contact Form 7 (for contact forms)
  - Yoast SEO (for enhanced SEO features)

### Installation Steps

#### Method 1: Upload via WordPress Admin

1. Download the theme package
2. Go to WordPress Admin > Appearance > Themes
3. Click "Add New" > "Upload Theme"
4. Select the theme ZIP file
5. Click "Install Now"
6. Activate the theme after installation

#### Method 2: Manual Installation

1. Unzip the theme files
2. Upload the `themisdb` folder to `/wp-content/themes/` via FTP
3. Go to WordPress Admin > Appearance > Themes
4. Activate the ThemisDB theme

### Quick Start Guide

#### 1. Set Up Menus

1. Go to **Appearance > Menus**
2. Create a new menu for main navigation
3. Add pages, categories, or custom links
4. Assign the menu to "Primary Menu" location
5. Optional: Create a footer menu

#### 2. Configure Widgets

Available widget areas:
- **Sidebar**: Main sidebar (right side)
- **Footer Widget 1, 2, 3**: Three footer columns

To add widgets:
1. Go to **Appearance > Widgets**
2. Drag widgets to desired areas
3. Recommended widgets:
   - Sidebar: Search, Categories, Recent Posts, Tags
   - Footer: About, Contact Info, Social Links

#### 3. Customize Colors

1. Go to **Appearance > Customize**
2. Navigate to **Theme Colors**
3. Adjust colors:
   - **Primary Color**: Default #2c3e50
   - **Secondary Color**: Default #3498db
   - **Accent Color**: Default #7c4dff

#### 4. Upload Logo

1. Go to **Appearance > Customize > Site Identity**
2. Click "Select Logo"
3. Upload your logo (recommended size: 200x60px)
4. Optional: Upload a Site Icon (Favicon)

#### 5. Use Featured Images

1. Edit a post or page
2. Click "Set featured image" in the right sidebar
3. Select or upload an image
4. Recommended size: 1200x675px

### Theme Features

#### Full Width Template

For pages without sidebar:
1. Edit a page
2. Select "Full Width" template under Page Attributes
3. Save the page

#### Block Editor Support

Fully compatible with Gutenberg:
- Custom color palette with Themis colors
- Optimized styles for all blocks
- Wide and full-width alignment support
- Editor styles for WYSIWYG experience

#### Code Blocks for Technical Content

Optimized for technical documentation:
- Syntax highlighting friendly styles
- Optimized code block display
- Monospace font for inline code
- Dark background for pre blocks

### Support

- **GitHub**: https://github.com/makr-code/ThemisDB
- **Documentation**: See repository docs folder
- **Issues**: Create a GitHub issue for bug reports

### License

Licensed under the MIT License.
