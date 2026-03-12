# WordPress Plugin Operations Runbook

## Zweck
Zentrale Betriebsdokumentation fuer Build, Dry-Run und Release der WordPress-Plugins.

## Geltungsbereich
- Repository: `makr-code/wordpressPlugins`
- Release-Strategie: ein Release pro Plugin
- Tag-Schema: `<plugin-slug>/v<version>`
- Asset-Schema: `<plugin-slug>.zip`

## Voraussetzungen
- Schreibrechte auf Releases und Tags
- `gh` CLI authentifiziert
- Konsistente Version in:
  - Haupt-Plugin-Datei
  - `update-info.json`

## Artefakt- und Metadatenregeln
- ZIP muss genau `dist/<plugin-slug>.zip` heissen.
- Release muss das ZIP als Asset enthalten.
- `update-info.json` muss vorhanden und gueltig sein.

## CI Workflow
- Datei: `.github/workflows/wordpress-plugin-release.yml`
- Trigger: manuell (`workflow_dispatch`)
- Inputs:
  - `plugin_slug`
  - `version`
  - `target_ref`
  - `dry_run`

## Layout-Erkennung
Die Pipeline und lokale Skripte unterstuetzen:
- Flat: `<repo>/<plugin-slug>/...`
- Nested: `<repo>/wordpress-plugins/<plugin-slug>/...`

## Lokale Betriebsbefehle
### Dry-Run
```powershell
./scripts/wordpress-plugin-dry-run-release.ps1 -PluginSlug themisdb-downloads -Version 1.2.0
```

### Batch-Release
```powershell
./scripts/wordpress-plugin-release-batch.ps1 -Repository "makr-code/wordpressPlugins"
```

## Empfohlener Ablauf
1. Versionen aktualisieren (Plugin + `update-info.json`).
2. Dry-Run fuer das betroffene Plugin.
3. Pilot-Release via Workflow mit `dry_run=false`.
4. Release in WordPress testweise pruefen.
5. Erst danach Batch-Releases starten.

## Operative Verifikation
### Release sichtbar?
```bash
gh release view "<plugin-slug>/v<version>" -R makr-code/wordpressPlugins
```

### API erreichbar?
```bash
curl -I "https://api.github.com/repos/makr-code/wordpressPlugins/releases?per_page=10"
```

### Metadata erreichbar?
```bash
curl "https://raw.githubusercontent.com/makr-code/wordpressPlugins/main/<plugin-slug>/update-info.json"
```

## Fehlerbilder
### Keine Updates in WordPress sichtbar
- Transient loeschen: `themisdb_update_<plugin-slug>`
- Im Admin manuell neu pruefen.
- Release-Tag/Asset und Version abgleichen.

### ZIP wird nicht gefunden
- Asset-Name muss exakt `<plugin-slug>.zip` sein.
- Tag muss exakt `<plugin-slug>/v<version>` sein.

### Versionskonflikt
- Workflow-Input `version` muss identisch zu `update-info.json` sein.

## Verwandte Dokumente
- `docs/ci-cd/WORDPRESS_PLUGIN_RELEASE_PIPELINE.md`
- `docs/plugins/WORDPRESS_PLUGIN_AUTOMATIC_UPDATES.md`
- `docs/plugins/WORDPRESS_PLUGIN_UPDATE_EXAMPLES.md`
