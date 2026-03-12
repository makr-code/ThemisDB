# Changelog

Alle wesentlichen Änderungen an diesem Plugin werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/),
und dieses Projekt folgt [Semantic Versioning](https://semver.org/lang/de/).

## [1.0.0] - 2026-01-15

### Hinzugefügt
- Initiales Release des ThemisDB Compendium Downloads Plugins
- Shortcode `[themisdb_compendium_downloads]` für Seiten und Beiträge
- Widget für Sidebar-Integration
- Admin-Einstellungsseite unter "Einstellungen → Kompendium Downloads"
- Automatisches Abrufen von GitHub Release-Daten
- Caching-Mechanismus für optimale Performance
- Download-Tracking und Statistiken
- Responsive Design mit Mobile-First-Ansatz
- Dark Mode Unterstützung
- Drei Stil-Optionen: Modern, Klassisch, Minimal
- Drei Layout-Optionen: Cards, List, Compact
- Anzeige von Print- und Professional-PDF-Versionen
- Dateigröße-Anzeige mit automatischer Formatierung
- Versions- und Datumsanzeige
- Cache-Verwaltung im Admin-Bereich
- Download-Statistiken im Admin-Bereich
- Deutsche und englische Übersetzungen
- Vollständige Dokumentation (README, INSTALLATION, CHANGELOG)
- Packaging-Script für Distribution

### Sicherheit
- XSS-Schutz durch Eingabevalidierung und Output-Escaping
- CSRF-Schutz mit WordPress Nonces
- Sichere API-Aufrufe mit wp_remote_get()
- Sanitization aller Benutzereingaben

### Performance
- Transient-Cache für GitHub API-Daten (Standard: 1 Stunde)
- Lazy Loading von Assets
- Optimierte CSS- und JavaScript-Dateien

[1.0.0]: https://github.com/makr-code/wordpressPlugins/releases/tag/v1.0.0
