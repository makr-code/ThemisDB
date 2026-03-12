# WordPress Plugin Release Pipeline

Hinweis: Die operative Schritt-fuer-Schritt-Durchfuehrung ist zentral dokumentiert in `docs/ci-cd/WORDPRESS_PLUGIN_OPERATIONS.md`.

## Ziel
Die Pipeline erstellt plugin-spezifische Releases fuer das Repository `makr-code/wordpressPlugins`.

## Workflow
- Datei: `.github/workflows/wordpress-plugin-release.yml`
- Trigger: `workflow_dispatch`
- Berechtigungen: `contents: write`

## Inputs
- `plugin_slug`: Plugin-Verzeichnisname
- `version`: Zielversion
- `target_ref`: Branch/Tag als Quelle
- `dry_run`: Nur bauen und validieren

## Layout-Unterstuetzung
Die Pipeline erkennt beide Strukturen:
- Flat: `<repo>/<plugin-slug>/...`
- Nested: `<repo>/wordpress-plugins/<plugin-slug>/...`

## Validierung
Vor dem Packaging prueft der Workflow:
- Plugin-Verzeichnis vorhanden
- `update-info.json` vorhanden
- Main-Plugin-Datei vorhanden
- Versionsgleichheit zwischen Input und `update-info.json`

## Packaging
- ZIP-Name: `dist/<plugin-slug>.zip`
- Shared Updater wird in `includes/class-themisdb-plugin-updater.php` kopiert

## Release-Konvention
- Tag: `<plugin-slug>/v<version>`
- Release-Name: `<plugin-slug> v<version>`
- Asset: `<plugin-slug>.zip`

## Lokale Skripte
- Dry-Run: `scripts/wordpress-plugin-dry-run-release.ps1`
- Batch-Releases: `scripts/wordpress-plugin-release-batch.ps1`

Beide Skripte unterstuetzen Flat und Nested Layout.

## Betriebsempfehlung
1. Erst Dry-Run (`dry_run=true` oder lokales Dry-Run-Skript).
2. Danach Pilot-Release fuer ein Plugin.
3. Erst dann Batch-Releases ausrollen.

## Runbook
- Zentrales Betriebs-Runbook: `docs/ci-cd/WORDPRESS_PLUGIN_OPERATIONS.md`
