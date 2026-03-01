# Automatisches Update-System für ThemisDB WordPress Plugins

**Version:** 1.0.0  
**Datum:** Februar 2026  
**Status:** Implementiert

---

## Übersicht

Alle ThemisDB WordPress Plugins sind jetzt mit einem automatischen Update-System ausgestattet, das das native WordPress Update-System nutzt. Updates werden direkt vom GitHub Repository (makr-code/ThemisDB) bereitgestellt.

---

## Features

### ✅ Kernfunktionalität

- **Automatische Update-Prüfung**: Integriert in WordPress Update-System
- **GitHub-Integration**: Updates werden direkt von GitHub Releases bezogen
- **Version-Management**: Automatische Version-Vergleiche
- **Metadata-Support**: Vollständige Plugin-Informationen in Update-Dialog
- **Caching**: 12-Stunden Cache zur Performance-Optimierung
- **Fehlertoleranz**: Graceful Degradation bei API-Fehlern

### 🔒 Sicherheit

- **Sichere API-Aufrufe**: Verwendet WordPress HTTP API
- **Transient Caching**: Minimiert externe Anfragen
- **Capability Checks**: Nur autorisierte Nutzer können Updates durchführen
- **HTTPS-Only**: Alle Verbindungen über sichere Kanäle

### 🚀 Performance

- **Lazy Loading**: Updates werden nur geprüft wenn nötig
- **Intelligentes Caching**: Reduziert GitHub API-Aufrufe
- **Asynchrone Prüfung**: Keine Blockierung des Admin-Bereichs
- **Minimale Overhead**: Nur aktive Plugins werden geprüft

---

## Implementierung

### Updater-Klasse

Die zentrale Update-Funktionalität ist in einer gemeinsamen Klasse implementiert:

```
wordpress-plugin/includes/class-themisdb-plugin-updater.php
```

Diese Klasse bietet:
- GitHub API-Integration
- WordPress Update Hook-Integration
- Version-Vergleich und Update-Logik
- Fehlerbehandlung und Logging

### Plugin-Integration

Jedes Plugin integriert den Updater mit minimalem Code:

```php
// Load updater class
require_once dirname(PLUGIN_DIR) . '/includes/class-themisdb-plugin-updater.php';

// Initialize automatic updates
if (class_exists('ThemisDB_Plugin_Updater')) {
    new ThemisDB_Plugin_Updater(
        PLUGIN_FILE,           // Main plugin file
        'plugin-slug',         // Plugin directory name
        PLUGIN_VERSION         // Current version
    );
}
```

### Update-Metadata

Jedes Plugin hat eine `update-info.json` Datei im Plugin-Verzeichnis:

```json
{
  "name": "Plugin Name",
  "version": "1.0.0",
  "homepage": "https://github.com/makr-code/ThemisDB",
  "description": "Plugin description",
  "author": "ThemisDB Team",
  "author_uri": "https://github.com/makr-code/ThemisDB",
  "requires": "5.8",
  "tested": "6.4",
  "requires_php": "7.4"
}
```

---

## Update-Workflow

### 1. Update-Prüfung

WordPress prüft periodisch (standardmäßig zweimal täglich) auf Updates:

1. Updater-Klasse wird für jedes Plugin initialisiert
2. GitHub API wird nach neuesten Releases abgefragt
3. `update-info.json` wird vom Repository geladen
4. Version-Vergleich wird durchgeführt
5. Update-Information wird im WordPress Transient gespeichert

### 2. Update-Benachrichtigung

Wenn ein Update verfügbar ist:

1. WordPress zeigt Update-Benachrichtigung im Admin-Bereich
2. "Details anzeigen"-Link zeigt Changelog und Informationen
3. "Jetzt aktualisieren"-Button steht zur Verfügung

### 3. Update-Installation

Bei Klick auf "Jetzt aktualisieren":

1. WordPress lädt Plugin-ZIP von GitHub
2. Backup des aktuellen Plugins wird erstellt
3. Neues Plugin wird installiert
4. Plugin wird automatisch reaktiviert

### 4. Update-Verifizierung

Nach der Installation:

1. WordPress prüft Plugin-Kompatibilität
2. Plugin-Aktivierung wird getestet
3. Erfolgsmeldung wird angezeigt

---

## Release-Prozess

### Für Plugin-Entwickler

#### 1. Version aktualisieren

In der Haupt-Plugin-Datei:

```php
/**
 * Version: 1.1.0
 */
define('PLUGIN_VERSION', '1.1.0');
```

In `update-info.json`:

```json
{
  "version": "1.1.0",
  ...
}
```

#### 2. Changelog erstellen

Im Plugin-Verzeichnis oder im Release:

```markdown
## [1.1.0] - 2026-02-17

### Added
- Neue Feature XYZ
- Verbesserte Performance

### Fixed
- Bugfix ABC
```

#### 3. GitHub Release erstellen

```bash
# Commit und Push
git add .
git commit -m "Release v1.1.0"
git push

# Tag erstellen
git tag v1.1.0
git push --tags

# GitHub Release erstellen (via GitHub UI oder CLI)
gh release create v1.1.0 \
  --title "Version 1.1.0" \
  --notes-file CHANGELOG.md
```

#### 4. Plugin-ZIP als Asset hochladen (Optional)

Für optimale Performance kann ein fertig gepacktes Plugin-ZIP als Release Asset hochgeladen werden:

```bash
# Plugin-ZIP erstellen
cd wordpress-plugin
zip -r themisdb-plugin-name.zip themisdb-plugin-name/

# Als Release Asset hochladen
gh release upload v1.1.0 themisdb-plugin-name.zip
```

---

## Cache-Management

### Automatisches Caching

- Updates werden 12 Stunden gecacht
- Cache-Key: `themisdb_update_{plugin_slug}`
- Verwendet WordPress Transient API

### Manuelles Cache-Löschen

Cache wird automatisch gelöscht bei:
- Manueller Update-Prüfung im WordPress Admin
- Nach erfolgreicher Plugin-Installation
- Beim Deaktivieren des Plugins

Manuell löschen:

```php
delete_transient('themisdb_update_plugin-slug');
```

Oder im WordPress Admin:
1. Dashboard → Aktualisierungen
2. "Erneut prüfen" klicken

---

## Troubleshooting

### Problem: Updates werden nicht angezeigt

**Lösung:**

1. Cache leeren:
   - Im Admin: Dashboard → Aktualisierungen → "Erneut prüfen"
   - Per Plugin: Transient manuell löschen

2. GitHub API prüfen:
   ```bash
   curl https://api.github.com/repos/makr-code/ThemisDB/releases/latest
   ```

3. `update-info.json` prüfen:
   ```bash
   curl https://raw.githubusercontent.com/makr-code/ThemisDB/main/wordpress-plugin/{plugin-slug}/update-info.json
   ```

### Problem: Update schlägt fehl

**Lösung:**

1. Plugin manuell aktualisieren:
   - Plugin deaktivieren
   - Alte Version löschen
   - Neue Version hochladen
   - Plugin aktivieren

2. Berechtigungen prüfen:
   - WordPress muss Schreibrechte im Plugins-Verzeichnis haben
   - PHP `allow_url_fopen` muss aktiviert sein

3. Fehler-Logs prüfen:
   - WordPress Debug-Log aktivieren
   - PHP Error-Log prüfen

### Problem: "Details anzeigen" zeigt keine Informationen

**Lösung:**

1. `update-info.json` validieren:
   - JSON-Syntax prüfen
   - Alle erforderlichen Felder vorhanden?

2. GitHub API Rate Limit prüfen:
   ```bash
   curl -I https://api.github.com/rate_limit
   ```

---

## API-Referenz

### ThemisDB_Plugin_Updater

**Konstruktor:**

```php
new ThemisDB_Plugin_Updater(
    string $plugin_file,     // Main plugin file path
    string $plugin_slug,     // Plugin directory name
    string $version,         // Current version
    string $username = 'makr-code',    // GitHub username
    string $repository = 'ThemisDB'    // GitHub repository
)
```

**Wichtige Methoden:**

```php
// Update prüfen
public function check_for_update($transient)

// Plugin-Info abrufen
public function plugin_info($result, $action, $args)

// Nach Installation
public function after_install($response, $hook_extra, $result)

// Cache löschen
public function maybe_clear_cache()
```

---

## Unterstützte Plugins

Alle 15 ThemisDB WordPress Plugins unterstützen automatische Updates:

1. ✅ themisdb-architecture-diagrams (v1.1.0)
2. ✅ themisdb-benchmark-visualizer (v1.0.0)
3. ✅ themisdb-compendium-downloads (v1.0.0)
4. ✅ themisdb-docker-downloads (v1.0.0)
5. ✅ themisdb-downloads (v1.2.0)
6. ✅ themisdb-feature-matrix (v1.0.0)
7. ✅ themisdb-formula-renderer (v1.1.0)
8. ✅ themisdb-gallery (v1.0.1)
9. ✅ themisdb-order-request (v1.0.0)
10. ✅ themisdb-query-playground (v1.0.0)
11. ✅ themisdb-release-timeline (v1.0.0)
12. ✅ themisdb-taxonomy-manager (v1.0.0)
13. ✅ themisdb-tco-calculator (v1.0.0)
14. ✅ themisdb-test-dashboard (v1.0.0)
15. ✅ themisdb-wiki-integration (v1.0.1)

---

## Best Practices

### Für Entwickler

1. **Semantic Versioning**: Verwenden Sie SemVer (MAJOR.MINOR.PATCH)
2. **Changelog pflegen**: Dokumentieren Sie alle Änderungen
3. **Testing**: Testen Sie Updates vor dem Release
4. **Backward Compatibility**: Vermeiden Sie Breaking Changes
5. **Kommunikation**: Informieren Sie Nutzer über wichtige Updates

### Für Administratoren

1. **Backup erstellen**: Vor jedem Update ein Backup anlegen
2. **Staging-Umgebung**: Updates zuerst auf Staging testen
3. **Update-Zeitpunkt**: Updates außerhalb der Stoßzeiten durchführen
4. **Monitoring**: System nach Update überwachen
5. **Rollback-Plan**: Bereithalten für den Fall von Problemen

---

## Technische Details

### GitHub API Endpoints

```
# Latest Release
GET https://api.github.com/repos/makr-code/ThemisDB/releases/latest

# Update Metadata
GET https://raw.githubusercontent.com/makr-code/ThemisDB/main/wordpress-plugin/{plugin-slug}/update-info.json
```

### WordPress Hooks

```php
// Update-Prüfung
add_filter('pre_set_site_transient_update_plugins', ...)

// Plugin-Informationen
add_filter('plugins_api', ...)

// Nach Installation
add_filter('upgrader_post_install', ...)

// Cache-Management
add_action('admin_init', ...)
```

### Transient Keys

```
themisdb_update_{plugin_slug}
```

Cache-Dauer: 43200 Sekunden (12 Stunden)

---

## Sicherheitshinweise

### Empfehlungen

1. **HTTPS verwenden**: Alle Verbindungen über sichere Kanäle
2. **Berechtigungen prüfen**: Nur Administratoren können Updates durchführen
3. **Input Validierung**: Alle externen Daten werden validiert
4. **Output Escaping**: Alle Ausgaben werden escaped
5. **Rate Limiting**: GitHub API Rate Limits beachten

### GitHub Token (Optional)

Für höhere API Rate Limits kann ein GitHub Token konfiguriert werden:

```php
define('THEMISDB_GITHUB_TOKEN', 'ghp_xxxxxxxxxxxx');
```

**Wichtig:** Token niemals im Code committen! Verwenden Sie wp-config.php oder Environment-Variablen.

---

## Support

### Hilfe benötigt?

- **GitHub Issues**: [https://github.com/makr-code/ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
- **Dokumentation**: [wordpress-plugin/README.md](README.md)
- **Discussions**: [https://github.com/makr-code/ThemisDB/discussions](https://github.com/makr-code/ThemisDB/discussions)

---

## Changelog

### [1.0.0] - 2026-02-17

#### Added
- Automatisches Update-System für alle ThemisDB Plugins
- GitHub API-Integration für Update-Prüfung
- Shared Updater-Klasse für alle Plugins
- Update-Metadata (update-info.json) für jedes Plugin
- Caching-Mechanismus (12 Stunden)
- WordPress Update-System Integration
- Changelog-Anzeige im Update-Dialog
- Fehlerbehandlung und Logging

---

**ThemisDB Team**  
*Automatische Updates für maximale Komfortabilität*

**Version:** 1.0.0  
**Stand:** Februar 2026  
**Lizenz:** MIT
