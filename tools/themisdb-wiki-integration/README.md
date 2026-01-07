# ThemisDB Wiki Integration WordPress Plugin

**Version:** 1.0.0  
**Author:** ThemisDB Team  
**License:** MIT  

## Beschreibung / Description

**Deutsch:**
Dieses WordPress-Plugin ermöglicht die automatische Integration der ThemisDB-Dokumentation (Wiki) aus GitHub in Ihre WordPress-Website. Das Plugin ruft Markdown-Dateien direkt aus dem GitHub-Repository ab und zeigt sie formatiert in WordPress an.

**English:**
This WordPress plugin enables automatic integration of ThemisDB documentation (wiki) from GitHub into your WordPress website. The plugin fetches markdown files directly from the GitHub repository and displays them formatted in WordPress.

## Features

- ✅ Automatisches Abrufen von Markdown-Dokumentation aus GitHub
- ✅ Unterstützung für mehrere Sprachen (DE, EN, FR)
- ✅ Caching-Mechanismus für bessere Performance
- ✅ Automatische Synchronisierung (stündlich)
- ✅ Shortcodes für einfache Integration
- ✅ Inhaltsverzeichnis-Generierung
- ✅ Responsive Design
- ✅ Dark Mode Support
- ✅ Admin-Panel zur Konfiguration

## Installation

### Methode 1: Manueller Upload

1. Laden Sie den Ordner `themisdb-wiki-integration` in das Verzeichnis `/wp-content/plugins/` hoch
2. Aktivieren Sie das Plugin über das 'Plugins'-Menü in WordPress
3. Gehen Sie zu Einstellungen → ThemisDB Wiki, um das Plugin zu konfigurieren

### Methode 2: Von GitHub

```bash
cd /path/to/wordpress/wp-content/plugins/
git clone https://github.com/makr-code/ThemisDB.git themisdb-repo
cp -r themisdb-repo/tools/themisdb-wiki-integration ./
rm -rf themisdb-repo
```

## Konfiguration

Nach der Aktivierung gehen Sie zu **Einstellungen → ThemisDB Wiki** und konfigurieren Sie:

- **GitHub Repository**: `makr-code/ThemisDB` (Standard)
- **Branch**: `main` oder `develop`
- **Dokumentationspfad**: `docs` (Standard)
- **Standard-Sprache**: `de`, `en`, oder `fr`
- **GitHub Token** (Optional): Für private Repos oder höhere Rate Limits
- **Auto-Sync**: Aktivieren für automatische stündliche Synchronisierung

## Verwendung / Usage

### Shortcode: Dokumentation anzeigen

```php
[themisdb_wiki file="README.md" lang="de" show_toc="yes"]
```

**Parameter:**
- `file`: Markdown-Datei zum Anzeigen (z.B. `README.md`, `features/FEATURES.md`)
- `lang`: Sprache (`de`, `en`, `fr`) - Standard: Plugin-Einstellung
- `show_toc`: Inhaltsverzeichnis anzeigen (`yes`/`no`)

### Shortcode: Dokumentationsliste anzeigen

```php
[themisdb_docs lang="de" layout="grid"]
```

**Parameter:**
- `lang`: Sprache (`de`, `en`, `fr`)
- `layout`: Anzeigelayout (`list`, `grid`)

## Beispiele / Examples

### Beispiel 1: Deutsche README anzeigen
```php
[themisdb_wiki file="README.md" lang="de"]
```

### Beispiel 2: Englische Architektur-Dokumentation mit Inhaltsverzeichnis
```php
[themisdb_wiki file="architecture/ARCHITECTURE.md" lang="en" show_toc="yes"]
```

### Beispiel 3: Liste aller verfügbaren deutschen Dokumente
```php
[themisdb_docs lang="de" layout="grid"]
```

### Beispiel 4: Feature-Übersicht
```php
[themisdb_wiki file="features/FEATURES.md" lang="de" show_toc="yes"]
```

### Beispiel 5: Installation Guide
```php
[themisdb_wiki file="guides/INSTALLATION.md" lang="en"]
```

## Verzeichnisstruktur

```
themisdb-wiki-integration/
├── themisdb-wiki-integration.php   # Haupt-Plugin-Datei
├── assets/
│   ├── css/
│   │   └── wiki-integration.css    # Styling
│   └── js/
│       └── wiki-integration.js     # JavaScript-Funktionalität
├── templates/
│   └── admin-settings.php          # Admin-Panel-Template
├── README.md                       # Diese Datei
├── INSTALLATION.md                 # Installationsanleitung
├── LICENSE                         # MIT Lizenz
└── readme.txt                      # WordPress.org readme
```

## Funktionsweise

1. **GitHub API Integration**: Das Plugin nutzt die GitHub Contents API, um Markdown-Dateien abzurufen
2. **Caching**: Abgerufene Inhalte werden für 1 Stunde im WordPress-Transient-Cache gespeichert
3. **Markdown-Parsing**: Einfache Markdown-zu-HTML-Konvertierung (für Produktion empfohlen: Parsedown-Library)
4. **Shortcode-Rendering**: WordPress Shortcodes ermöglichen flexible Integration in Seiten/Posts
5. **Auto-Sync**: Optionale stündliche Aktualisierung der Dokumentation

## Technische Details

### API Rate Limits

- **GitHub API**: 60 Requests/Stunde (nicht authentifiziert)
- **Mit Token**: 5.000 Requests/Stunde
- **Empfehlung**: GitHub Personal Access Token für höhere Limits

### Caching

- **Transient Cache**: 1 Stunde Standardwert
- **Manuelles Löschen**: Admin-Panel → "Sync Now" Button
- **Automatisches Löschen**: Bei stündlicher Auto-Sync

### Performance

- Lazy Loading: Assets nur bei Shortcode-Verwendung laden
- Minified CSS/JS für Produktion
- CDN-kompatibel
- Responsive Images

## Sicherheit

- ✅ Nonce-Verification für AJAX-Requests
- ✅ Capability Checks (`manage_options`)
- ✅ Input Sanitization
- ✅ Output Escaping
- ✅ Sichere GitHub Token-Speicherung

## Kompatibilität

- **WordPress**: 5.0+
- **PHP**: 7.4+
- **Themes**: Kompatibel mit allen Standard-WordPress-Themes
- **Plugins**: Keine bekannten Konflikte

## Troubleshooting

### Problem: "GitHub API returned status code 404"
**Lösung**: Überprüfen Sie Repository-Name, Branch und Dokumentationspfad in den Einstellungen

### Problem: "Rate limit exceeded"
**Lösung**: Fügen Sie ein GitHub Personal Access Token in den Einstellungen hinzu

### Problem: Dokumentation wird nicht aktualisiert
**Lösung**: Klicken Sie auf "Sync Now" im Admin-Panel, um den Cache manuell zu löschen

### Problem: Styling-Probleme
**Lösung**: Überprüfen Sie Theme-CSS-Konflikte und passen Sie `wiki-integration.css` an

## Erweiterungsmöglichkeiten

### Parsedown-Integration (empfohlen für Produktion)

```php
// In themisdb-wiki-integration.php
require_once 'vendor/parsedown/Parsedown.php';

private function markdown_to_html($markdown) {
    $Parsedown = new Parsedown();
    return $Parsedown->text($markdown);
}
```

### Syntax Highlighting

Integration mit **Prism.js** oder **Highlight.js** für Code-Syntax-Highlighting:

```php
wp_enqueue_style('prism-css', 'https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/themes/prism.min.css');
wp_enqueue_script('prism-js', 'https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/prism.min.js');
```

### Mermaid.js für Diagramme

```php
wp_enqueue_script('mermaid-js', 'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js');
```

## Support & Beiträge

- **Repository**: https://github.com/makr-code/ThemisDB
- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Dokumentation**: https://github.com/makr-code/ThemisDB/tree/main/docs

## Roadmap

### Version 1.1
- [ ] Parsedown-Library für besseres Markdown-Parsing
- [ ] Syntax Highlighting mit Prism.js
- [ ] Mermaid.js für Diagramm-Rendering
- [ ] Suche in Dokumentation

### Version 1.2
- [ ] PDF-Export von Dokumentation
- [ ] Versionsvergleich
- [ ] Mehrsprachige Navigation
- [ ] Custom CSS pro Seite

### Version 2.0
- [ ] Gutenberg-Block für Dokumentation
- [ ] REST API Endpoints
- [ ] Webhook-Integration für automatische Updates
- [ ] ThemisDB-Instanz als Backend für Suche

## Lizenz

Dieses Plugin ist unter der MIT-Lizenz lizenziert. Siehe [LICENSE](LICENSE) für Details.

## Autoren

- **ThemisDB Team** - https://github.com/makr-code

## Danksagungen

- Inspiriert vom TCO Calculator Plugin
- Basierend auf GitHub Contents API
- WordPress Plugin-Boilerplate

## Changelog

### Version 1.0.0 (Januar 2026)
- ✅ Initiale Veröffentlichung
- ✅ GitHub API Integration
- ✅ Markdown-zu-HTML-Konvertierung
- ✅ Mehrsprachige Unterstützung (DE, EN, FR)
- ✅ Caching-Mechanismus
- ✅ Auto-Sync-Funktion
- ✅ Admin-Panel
- ✅ Responsive Design
- ✅ Dark Mode Support

---

**Viel Erfolg mit ThemisDB Wiki Integration!** 🚀
