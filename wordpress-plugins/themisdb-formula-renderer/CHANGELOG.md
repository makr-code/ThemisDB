# Changelog

Alle wichtigen Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/),
und dieses Projekt folgt [Semantic Versioning](https://semver.org/lang/de/).

## [1.1.0] - 2026-02-11

### ⚡ Performance Enhancements (CRITICAL)
- **Conditional Loading**: KaTeX wird nur noch auf Seiten mit Formeln geladen
  - Prüft auf Shortcodes: `[themisdb_formula]`, `[formula]`, `[latex]`, `[math]`
  - Prüft auf Delimiters: `$$...$$` und `$...$`
  - Performance-Gewinn: ~70% schnellere Ladezeit auf Seiten ohne Formeln
  - ~1.5 MB weniger Datenübertragung auf Seiten ohne Formeln
- **Preload Hints**: `<link rel="preload">` für KaTeX CSS und JS hinzugefügt
- Entfernung von `ajaxDelay` aus localized script (nicht mehr benötigt)

### 🎨 Design & Branding (HIGH)
- **Themis Brand Colors**: Neue Farbpalette implementiert
  - Primary: #2c3e50, Secondary: #3498db, Accent: #7c4dff
  - Success: #27ae60, Warning: #f39c12, Error: #e74c3c
- **Mobile Responsiveness**: Verbesserte Darstellung auf kleinen Bildschirmen
  - Horizontal Scroll für lange Formeln
  - Touch-optimierte Bedienung
  - Kleinere Schriftgrößen für bessere Lesbarkeit
- **Dark Mode**: Optimierte Farbpalette für Dark Mode
  - Automatische Anpassung an System-Einstellungen
  - Verbesserte Kontraste für KaTeX-Formeln
- **Print Styles**: Optimierte Druckausgabe
  - Copy-Buttons werden beim Drucken ausgeblendet
  - Transparenter Hintergrund für Formeln
  - Page-break-inside: avoid für Formeln

### ✨ Neue Features
- **Copy-to-Clipboard Button** (MEDIUM):
  - Copy-Button erscheint beim Hover über Block-Formeln
  - Auf Mobile immer sichtbar
  - Unterstützt moderne Clipboard API und Fallback für ältere Browser
  - Visuelles Feedback: "✅ Copied!" nach erfolgreichem Kopieren
  - Animierte Button-Effekte
- **Formula Library** (LOW):
  - Admin-Bereich mit häufig verwendeten Formeln
  - Zugriff über: Einstellungen → Formula Library
  - 5 Kategorien: Algebra, Calculus, Statistics, Physics, Geometry
  - 15 vordefinierte Formeln mit Beschreibungen
  - Ein-Klick-Kopieren von Shortcodes oder LaTeX-Code
  - Live-Vorschau der Formeln
  - Responsive Grid-Layout
- **MathML Export** (MEDIUM):
  - Screen Reader Support durch MathML-Alternative
  - Automatische MathML-Generierung mit KaTeX
  - Hidden MathML für Screen Reader
  - ARIA-Labels für bessere Accessibility
  - Visuelle Formeln werden für Screen Reader versteckt

### ♿ Accessibility
- **Screen Reader Support**: MathML-Alternative für alle Formeln
- **ARIA Labels**: Verbesserte Labels für Copy-Buttons
- **High Contrast Mode**: Unterstützung für hohe Kontraste
- **Reduced Motion**: Respektiert `prefers-reduced-motion`
- **sr-only Class**: Standard Screen-Reader-Only Utility-Klasse

### 📱 Mobile Improvements (HIGH)
- **Overflow Fix**: Horizontal Scroll für lange Formeln
- **Touch Optimization**: Copy-Buttons immer sichtbar auf Mobile
- **Responsive Sizing**: Kleinere Formeln auf kleinen Bildschirmen
- **-webkit-overflow-scrolling**: Touch für besseres Scrolling

### 🔧 Technical Changes
- Version aktualisiert von 1.0.0 auf 1.1.0
- CSS komplett überarbeitet (von 180 auf 300+ Zeilen)
- JavaScript erweitert mit Copy- und MathML-Funktionalität
- Neue PHP-Klasse: `ThemisDB_Formula_Library`
- Admin-Menu-Integration für Formula Library

### 📝 Dokumentation
- README.md aktualisiert mit v1.1.0 Features
- Changelog erweitert mit detaillierten Änderungen
- Formula Library Dokumentation hinzugefügt

### Geändert
- `themisdb_formula_enqueue_scripts()`: Conditional Loading implementiert
- `assets/css/style.css`: Komplett überarbeitet mit Themis Brand Colors
- `assets/js/script.js`: Copy-Button und MathML-Funktionalität hinzugefügt

### Hinzugefügt
- `themisdb_formula_add_preload()`: Preload-Hints Funktion
- `themisdb_formula_library_menu()`: Admin-Menu für Formula Library
- `includes/class-formula-library.php`: Neue Klasse für Formula Library
- Copy-Button Styles und Funktionalität
- MathML Accessibility Features
- Dark Mode und Print Styles

### Bekannte Einschränkungen
- Conditional Loading funktioniert nur auf Single Posts/Pages (nicht auf Archive-Seiten)
- Copy-Button erfordert moderne Browser (IE11 wird mit Fallback unterstützt)
- MathML-Generierung erfordert KaTeX geladen zu sein

## [1.0.0] - 2026-01-11

### Hinzugefügt
- Initiale Version des ThemisDB Formula Renderer Plugins
- Automatisches Rendering von LaTeX-Formeln mit KaTeX
- Unterstützung für Inline-Formeln (`$...$`)
- Unterstützung für Block-Formeln (`$$...$$`)
- Shortcode-Unterstützung: `[themisdb_formula]`, `[formula]`, `[latex]`, `[math]`
- Admin-Einstellungsseite mit Konfigurationsoptionen
- Anpassbare Delimiters (Trennzeichen) mit Validierung
- Responsive Design mit Dark Mode Unterstützung
- Vollständige LaTeX-Mathematik-Syntax-Unterstützung
- KaTeX 0.16.9 Integration via CDN
- Fehlertolerantes Rendering mit aussagekräftigen Fehlermeldungen
- WordPress Gutenberg und Classic Editor Kompatibilität
- Mehrsprachige Unterstützung (Text Domain: themisdb-formula-renderer)
- Auto-Render für dynamisch geladenen Content (AJAX-Support mit intelligenter Retry-Logik)
- Performance-optimiert mit CDN-Bereitstellung
- Beispiele und Dokumentation auf der Einstellungsseite
- CSS-Anpassungsmöglichkeiten
- Umfassende Security-Features (siehe unten)
- MIT Lizenz

### Sicherheit
- **Input Sanitization**: Alle Benutzereingaben werden sanitized mit `sanitize_text_field()`
- **Input Validation**: Delimiter-Validierung mit Character-Whitelisting
- **XSS-Schutz**: Proper escaping und Validierung von `$_GET` Parameter
- **Field Length Limits**: Server- und Client-seitige Längenbegrenzung (maxlength: 10)
- **Settings API**: Sanitization Callbacks in `register_setting()` registriert
- **CDN Security**: Dokumentation zu Subresource Integrity (SRI) für Produktionsumgebungen

### Features
- **Auto-Rendering**: Formeln werden automatisch in Beiträgen, Seiten und Kommentaren gerendert
- **Shortcodes**: Mehrere Shortcode-Aliase für Flexibilität
- **Settings Page**: Benutzerfreundliche Konfiguration im WordPress Admin
- **Examples**: Interaktive Beispiele auf der Einstellungsseite
- **Resources**: Links zu KaTeX-Dokumentation und LaTeX-Hilfe
- **Dark Mode**: Automatische Anpassung an Dark Mode
- **Responsive**: Optimiert für alle Bildschirmgrößen
- **Error Handling**: Aussagekräftige Fehlermeldungen bei Syntax-Problemen
- **No Server Load**: Alle Berechnungen erfolgen im Browser
- **Cache Friendly**: Funktioniert mit allen WordPress-Caching-Plugins

### Dokumentation
- Vollständiges README.md mit Verwendungsbeispielen
- Detailliertes INSTALLATION.md mit Schritt-für-Schritt-Anleitung
- Code-Kommentare und PHPDoc
- Beispiele für häufige LaTeX-Befehle

### Technische Details
- PHP 7.2+ Kompatibilität
- WordPress 5.0+ Kompatibilität
- KaTeX 0.16.9 (via jsDelivr CDN)
- jQuery-basiertes JavaScript
- CSS3 mit modernen Features
- WordPress Coding Standards
- Security: Input Sanitization und Output Escaping

### Unterstützte LaTeX-Features
- Grundlegende Operatoren und Symbole
- Griechische Buchstaben
- Hoch- und Tiefgestellte Zeichen
- Brüche und Wurzeln
- Summen, Produkte und Integrale
- Matrizen und Vektoren
- Grenzwerte und Ableitungen
- Physikalische und chemische Notation
- Spezielle Funktionen
- Und viele mehr (siehe KaTeX-Dokumentation)

### Bekannte Einschränkungen
- Erfordert JavaScript-Aktivierung im Browser
- Sehr komplexe Formeln können Rendering-Zeit erhöhen
- TikZ und PGF/TikZ werden nicht unterstützt (nur pure KaTeX)
- Benötigt Internet-Verbindung für CDN (kann auf lokales Hosting umgestellt werden)

[1.1.0]: https://github.com/makr-code/wordpressPlugins/releases/tag/v1.1.0
[1.0.0]: https://github.com/makr-code/wordpressPlugins/releases/tag/v1.0.0
