# Changelog

Alle wichtigen Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/),
und dieses Projekt folgt [Semantic Versioning](https://semver.org/lang/de/).

## [1.0.0] - 2026-01-11

### Hinzugefügt
- Initiale Version des ThemisDB Formula Renderer Plugins
- Automatisches Rendering von LaTeX-Formeln mit KaTeX
- Unterstützung für Inline-Formeln (`$...$`)
- Unterstützung für Block-Formeln (`$$...$$`)
- Shortcode-Unterstützung: `[themisdb_formula]`, `[formula]`, `[latex]`, `[math]`
- Admin-Einstellungsseite mit Konfigurationsoptionen
- Anpassbare Delimiters (Trennzeichen)
- Responsive Design mit Dark Mode Unterstützung
- Vollständige LaTeX-Mathematik-Syntax-Unterstützung
- KaTeX 0.16.9 Integration via CDN
- Fehlertolerantes Rendering mit aussagekräftigen Fehlermeldungen
- WordPress Gutenberg und Classic Editor Kompatibilität
- Mehrsprachige Unterstützung (Text Domain: themisdb-formula-renderer)
- Auto-Render für dynamisch geladenen Content (AJAX-Support)
- Performance-optimiert mit CDN-Bereitstellung
- Beispiele und Dokumentation auf der Einstellungsseite
- CSS-Anpassungsmöglichkeiten
- XSS-Schutz und Security Best Practices
- MIT Lizenz

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

[1.0.0]: https://github.com/makr-code/ThemisDB/releases/tag/v1.0.0
