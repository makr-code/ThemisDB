# WordPress Plugin Automatic Updates

## Zweck
Diese Seite ist die kanonische Dokumentation fuer automatische Updates der ThemisDB-WordPress-Plugins.

## Update-Quelle
- GitHub Repository: `makr-code/wordpressPlugins`
- Release-Strategie: Ein Release pro Plugin
- Tag-Format: `<plugin-slug>/v<version>`
- Release-Asset: `<plugin-slug>.zip`

## Updater-Implementierung
- Shared Updater: `wordpress-plugins/includes/class-themisdb-plugin-updater.php`
- Fallback-Layouts fuer Metadaten:
  - Flat: `<repo>/<plugin-slug>/update-info.json`
  - Nested: `<repo>/wordpress-plugins/<plugin-slug>/update-info.json`
  - Legacy: `<repo>/wordpress-plugin/<plugin-slug>/update-info.json`

## Plugin-Integration
Jedes Plugin initialisiert den Updater mit:
- Hauptdatei (`plugin file`)
- Plugin-Slug
- Aktuelle Version

Der Updater:
- registriert WordPress-Update-Hooks,
- prueft plugin-spezifische Releases,
- liefert Details fuer den Update-Dialog,
- leert nach Installationen den Cache.

## Metadata Contract (`update-info.json`)
Pflichtfelder:
- `name`
- `version`
- `homepage`
- `description`
- `author`
- `author_uri`
- `requires`
- `tested`
- `requires_php`

Empfohlene Werte fuer `homepage` und `author_uri`:
- `https://github.com/makr-code/wordpressPlugins`

## Release-Ablauf
1. Versionsnummer in Plugin-Hauptdatei und `update-info.json` anheben.
2. Changelog ergaenzen.
3. ZIP mit Namen `<plugin-slug>.zip` bauen.
4. Tag `<plugin-slug>/v<version>` erstellen.
5. GitHub Release mit ZIP-Asset veroeffentlichen.

## CI/CD
Die Release-Pipeline ist dokumentiert unter:
- `docs/ci-cd/WORDPRESS_PLUGIN_RELEASE_PIPELINE.md`

Die operative Durchfuehrung ist dokumentiert unter:
- `docs/ci-cd/WORDPRESS_PLUGIN_OPERATIONS.md`

## Troubleshooting
- Keine Updates sichtbar:
  - WordPress-Updatepruefung manuell ausloesen.
  - Plugin-Transient loeschen: `themisdb_update_<plugin-slug>`.
- Falsche Version:
  - `version` in `update-info.json` muss exakt zum Release passen.
- Download nicht gefunden:
  - Asset muss exakt `<plugin-slug>.zip` heissen.

## Migration-Hinweis
Diese Doku wurde aus `wordpress-plugins/AUTOMATIC_UPDATES.md` nach `docs/` ueberfuehrt. Die alte Datei bleibt als Redirect erhalten.
