# WordPress Plugin für ThemisDB Downloads

## Übersicht

Dieses WordPress Plugin ermöglicht die automatische Anzeige von ThemisDB Release-Downloads auf WordPress-Seiten. Es ruft die neuesten Packages vom GitHub Repository ab und zeigt sie mit den entsprechenden SHA256-Checksums zur Verifikation an.

## Funktionen

- ✅ Automatisches Abrufen der neuesten Releases vom GitHub Repository
- ✅ Anzeige von Download-Links für alle Plattformen (Windows, Linux, Docker, QNAP, ARM)
- ✅ SHA256-Checksums für jede Datei
- ✅ Browser-basierte Download-Verifizierung
- ✅ Mehrere Anzeigestile (Standard, Kompakt, Tabelle)
- ✅ Plattform-Filter
- ✅ Cache-System zur Reduzierung von API-Aufrufen
- ✅ Responsive Design
- ✅ Einfache Integration per Shortcode

## Speicherort im Repository

```
ThemisDB/
└── wordpress-plugin/
    ├── PACKAGING.md                    # Packaging-Anleitung
    └── themisdb-downloads/             # Plugin-Verzeichnis
        ├── themisdb-downloads.php      # Haupt-Plugin-Datei
        ├── README.md                   # Plugin-Dokumentation
        ├── INSTALLATION.md             # Installationsanleitung
        ├── includes/                   # PHP-Klassen
        │   ├── class-github-api.php    # GitHub API Handler
        │   ├── class-admin.php         # Admin Panel
        │   └── class-shortcodes.php    # Shortcode Handler
        └── assets/                     # Frontend-Ressourcen
            ├── css/
            │   ├── style.css           # Frontend Styles
            │   └── admin.css           # Admin Styles
            └── js/
                ├── script.js           # Frontend JavaScript
                └── admin.js            # Admin JavaScript
```

## Integration mit ThemisDB Release-Strategie

Das Plugin integriert sich nahtlos in die ThemisDB Release-Strategie (siehe `docs/de/deployment/deployment_strategy.md` und `docs/de/deployment/v1.3.5_RELEASE_BUILD_STRATEGY.md`):

### Unterstützte Release-Formate

Das Plugin erkennt und zeigt automatisch folgende Release-Assets an:

| Plattform | Dateiformat | Beispiel |
|-----------|-------------|----------|
| **Windows x64** | `.zip`, `.exe` | `themisdb-1.4.0-windows-x64.zip` |
| **Linux x64** | `.tar.gz`, `.deb`, `.rpm` | `themisdb-1.4.0-linux-x64.tar.gz` |
| **Linux ARM64** | `.tar.gz` | `themisdb-1.4.0-arm64.tar.gz` |
| **Docker** | Docker Hub Link | `themisdb/themisdb:1.4.0` |
| **QNAP NAS** | `.zip` | `themisdb-1.4.0-qnap-x64.zip` |

### SHA256-Checksums

Das Plugin liest automatisch SHA256SUMS-Dateien aus GitHub Releases:

**Unterstützte Formate:**
- `SHA256SUMS.txt`
- `SHA256SUMS_v1.4.0.txt`
- `SHA256SUMS_production.txt`
- `themisdb-1.4.0.sha256`

**Format der Checksum-Datei:**
```
a1b2c3d4...  themisdb-1.4.0-windows-x64.zip
e5f6g7h8...  themisdb-1.4.0-linux-x64.tar.gz
```

### Release-Editions

Das Plugin unterstützt alle drei ThemisDB Editions:

1. **Community Edition** (öffentlich auf GitHub)
   - Automatisch über GitHub API abrufbar
   - Keine Authentifizierung erforderlich

2. **Enterprise Edition** (privates Repository/Release)
   - Benötigt GitHub Personal Access Token mit Zugriff auf privates Repository
   - Konfigurierbar in Plugin-Einstellungen

3. **Hyperscaler Edition** (OEM-direkt)
   - Kann über Custom GitHub Repository konfiguriert werden
   - Token mit entsprechenden Berechtigungen erforderlich

## Installation

Siehe [INSTALLATION.md](themisdb-downloads/INSTALLATION.md) für detaillierte Installationsanweisungen.

### Schnellstart

1. Plugin herunterladen oder als ZIP verpacken:
   ```bash
   cd wordpress-plugin
   zip -r themisdb-downloads.zip themisdb-downloads/
   ```

2. In WordPress hochladen:
   - WordPress Admin → Plugins → Installieren
   - "Plugin hochladen" → ZIP auswählen
   - Plugin aktivieren

3. Konfigurieren:
   - Einstellungen → ThemisDB Downloads
   - GitHub Repository: `makr-code/ThemisDB`
   - Optional: GitHub Token für höhere API-Limits

4. Shortcode verwenden:
   ```
   [themisdb_downloads]
   ```

## Verwendung

### Shortcodes

```php
// Neueste Version anzeigen
[themisdb_downloads]

// Alle Releases anzeigen
[themisdb_downloads show="all"]

// Nur Windows-Downloads
[themisdb_downloads platform="windows"]

// Kompakte Ansicht
[themisdb_downloads style="compact"]

// Tabellen-Ansicht
[themisdb_downloads style="table"]

// Verifizierungs-Tool
[themisdb_verify]

// Nur Versionsnummer
[themisdb_latest]
```

### Beispiel-Seite

Erstellen Sie eine WordPress-Seite mit folgendem Inhalt:

```html
<h1>ThemisDB Downloads</h1>

<p>Aktuelle Version: [themisdb_latest]</p>

<h2>Neueste Version herunterladen</h2>
[themisdb_downloads]

<h2>Download verifizieren</h2>
[themisdb_verify]

<h2>Alle Versionen</h2>
[themisdb_downloads show="all" style="table" limit="5"]
```

## Konfiguration

### GitHub API Token

Für höhere API-Limits (5000 statt 60 Anfragen/Stunde):

1. Gehen Sie zu: https://github.com/settings/tokens
2. Erstellen Sie einen neuen Token (classic)
3. Name: "WordPress ThemisDB Plugin"
4. Scopes: Keine (für öffentliche Repos)
5. Token kopieren und in Plugin-Einstellungen einfügen

### Cache-Einstellungen

**Empfohlene Werte:**
- Kleine Websites (< 1000 Besucher/Tag): `3600` Sekunden (1 Stunde)
- Mittlere Websites (1000-10000 Besucher/Tag): `7200` Sekunden (2 Stunden)
- Große Websites (> 10000 Besucher/Tag): `14400` Sekunden (4 Stunden)

## Release-Workflow

### Für ThemisDB Releases

Wenn ein neues ThemisDB Release veröffentlicht wird:

1. **GitHub Release erstellen** (wie in `v1.3.5_RELEASE_BUILD_STRATEGY.md` beschrieben)
2. **Assets hochladen:**
   - Windows ZIP: `themisdb-1.4.0-windows-x64.zip`
   - Linux TAR.GZ: `themisdb-1.4.0-linux-x64.tar.gz`
   - SHA256SUMS: `SHA256SUMS.txt`
3. **Plugin erkennt automatisch:**
   - Neues Release wird nach Cache-Ablauf angezeigt
   - Oder manuell Cache leeren in WordPress Admin

### SHA256SUMS generieren

Bei der Erstellung eines Releases (siehe `deployment_strategy.md`):

```bash
# Windows (PowerShell)
Get-FileHash -Algorithm SHA256 *.zip | Format-List | Out-File SHA256SUMS.txt

# Linux/macOS
sha256sum *.zip *.tar.gz > SHA256SUMS.txt

# Zum Release hinzufügen
gh release upload v1.4.0 SHA256SUMS.txt
```

## Deployment auf Produktions-Website

### Voraussetzungen

- WordPress 5.0+
- PHP 7.2+
- HTTPS (empfohlen für API-Aufrufe)

### Deployment-Schritte

1. **Plugin installieren** (siehe INSTALLATION.md)
2. **Seite erstellen:**
   - Neue Seite: "Downloads"
   - Shortcode einfügen: `[themisdb_downloads]`
   - Veröffentlichen
3. **Navigation aktualisieren:**
   - Menü → Downloads-Seite hinzufügen
4. **Testen:**
   - Seite aufrufen
   - Download-Links überprüfen
   - SHA256-Checksums überprüfen

### Performance-Optimierung

1. **WordPress-Cache aktivieren:**
   - Plugin: WP Super Cache oder W3 Total Cache
   - Cache-Zeit: mindestens 1 Stunde

2. **CDN verwenden:**
   - CloudFlare, AWS CloudFront, etc.
   - Reduziert Ladezeiten für Assets

3. **GitHub Token verwenden:**
   - Erhöht API-Limit auf 5000/Stunde
   - Verhindert Rate Limit Errors

## Monitoring & Wartung

### Plugin-Status überwachen

Überprüfen Sie regelmäßig:
- WordPress Admin → Einstellungen → ThemisDB Downloads
- API-Verbindungsstatus (oben auf der Seite)
- Cache-Status

### Cache manuell leeren

Bei Problemen oder nach neuem Release:
1. WordPress Admin → Einstellungen → ThemisDB Downloads
2. Button "Cache leeren" klicken

### Logs überprüfen

WordPress Debug Log aktivieren (in `wp-config.php`):
```php
define('WP_DEBUG', true);
define('WP_DEBUG_LOG', true);
```

Logs finden Sie in: `wp-content/debug.log`

## Troubleshooting

Siehe [README.md](themisdb-downloads/README.md) Abschnitt "Fehlerbehebung" für häufige Probleme und Lösungen.

## Support & Weiterentwicklung

- **GitHub Issues:** https://github.com/makr-code/ThemisDB/issues
- **Dokumentation:** [docs/de/deployment/](../../docs/de/deployment/)
- **Plugin README:** [themisdb-downloads/README.md](themisdb-downloads/README.md)

## Lizenz

MIT License - Siehe [../../LICENSE](../../LICENSE)

## Verwandte Dokumentation

- [Deployment Strategy](../../docs/de/deployment/deployment_strategy.md)
- [v1.3.5 Release Build Strategy](../../docs/de/deployment/v1.3.5_RELEASE_BUILD_STRATEGY.md)
- [GitHub Release Process](../../docs/de/deployment/README.md)
