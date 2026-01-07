# WordPress Plugin Packaging Guide

Dieses Dokument beschreibt, wie das ThemisDB Downloads WordPress Plugin für die Verteilung gepackt wird.

## Paket erstellen

### Schritt 1: Plugin-Verzeichnis vorbereiten

Stellen Sie sicher, dass alle Dateien vorhanden sind:

```
themisdb-downloads/
├── themisdb-downloads.php
├── README.md
├── INSTALLATION.md
├── includes/
│   ├── class-github-api.php
│   ├── class-admin.php
│   └── class-shortcodes.php
├── assets/
│   ├── css/
│   │   ├── style.css
│   │   └── admin.css
│   └── js/
│       ├── script.js
│       └── admin.js
└── languages/ (leer, für zukünftige Übersetzungen)
```

### Schritt 2: ZIP-Archiv erstellen

**Linux/macOS:**
```bash
cd wordpress-plugin
zip -r themisdb-downloads-1.0.0.zip themisdb-downloads/ -x "*.git*" -x "*__pycache__*" -x "*.DS_Store"
```

**Windows (PowerShell):**
```powershell
cd wordpress-plugin
Compress-Archive -Path themisdb-downloads -DestinationPath themisdb-downloads-1.0.0.zip
```

### Schritt 3: Paket überprüfen

Entpacken Sie das ZIP-Archiv in ein Test-Verzeichnis und überprüfen Sie:

1. Alle erforderlichen Dateien sind vorhanden
2. Keine unnötigen Dateien (z.B. .git, .DS_Store) sind enthalten
3. Die Verzeichnisstruktur ist korrekt

### Schritt 4: In WordPress testen

1. Installieren Sie das Plugin in einer Test-WordPress-Installation
2. Aktivieren Sie das Plugin
3. Konfigurieren Sie die Einstellungen
4. Erstellen Sie eine Test-Seite mit dem Shortcode
5. Überprüfen Sie alle Funktionen

## Verteilung

### Option 1: GitHub Release

Laden Sie das ZIP-Archiv als Asset zu einem GitHub Release hoch:

```bash
gh release create v1.0.0-plugin \
  themisdb-downloads-1.0.0.zip \
  --title "ThemisDB Downloads WordPress Plugin v1.0.0" \
  --notes "WordPress Plugin für ThemisDB Downloads"
```

### Option 2: Website-Download

Stellen Sie das ZIP-Archiv auf Ihrer Website zum Download bereit:

```html
<a href="/downloads/themisdb-downloads-1.0.0.zip" download>
  WordPress Plugin herunterladen
</a>
```

### Option 3: WordPress.org Repository (zukünftig)

Für die Veröffentlichung im offiziellen WordPress Plugin Repository:

1. Registrieren Sie sich bei wordpress.org
2. Beantragen Sie ein Plugin Repository
3. Folgen Sie den WordPress.org Guidelines
4. Nutzen Sie SVN für Plugin-Updates

## Versionierung

Folgen Sie Semantic Versioning (SemVer):

- **MAJOR.MINOR.PATCH** (z.B. 1.0.0)
- MAJOR: Breaking Changes
- MINOR: Neue Features (rückwärts kompatibel)
- PATCH: Bugfixes

Aktualisieren Sie die Versionsnummer in:
1. `themisdb-downloads.php` (Plugin Header)
2. `define('THEMISDB_DOWNLOADS_VERSION', '1.0.0');`
3. README.md (Changelog)

## Changelog pflegen

Führen Sie ein Changelog in README.md:

```markdown
## Changelog

### Version 1.1.0 (Februar 2026)
- Neue Feature: Docker Image Downloads
- Verbesserung: Schnelleres Caching
- Bugfix: SHA256-Anzeige für spezielle Dateinamen

### Version 1.0.0 (Januar 2026)
- Erste Veröffentlichung
```

## Checksums erstellen

Erstellen Sie SHA256-Checksums für das Plugin-Paket:

```bash
sha256sum themisdb-downloads-1.0.0.zip > themisdb-downloads-1.0.0.zip.sha256
```

Veröffentlichen Sie die Checksum-Datei zusammen mit dem Plugin-Paket.

## Update-Prozess

Für Plugin-Updates:

1. Aktualisieren Sie die Versionsnummer
2. Aktualisieren Sie das Changelog
3. Erstellen Sie ein neues ZIP-Paket
4. Testen Sie das Update in einer Test-Installation
5. Veröffentlichen Sie das neue Paket
6. Informieren Sie Benutzer über das Update

## Lizenz & Credits

Stellen Sie sicher, dass folgende Informationen im Plugin Header vorhanden sind:

```php
/**
 * Plugin Name: ThemisDB Downloads
 * Version: 1.0.0
 * Author: ThemisDB Team
 * License: MIT
 * License URI: https://opensource.org/licenses/MIT
 */
```

## Support-Informationen

Fügen Sie Support-Kontakte hinzu:

- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: Link zur Dokumentation
- E-Mail: support@themisdb.example.com (falls verfügbar)
