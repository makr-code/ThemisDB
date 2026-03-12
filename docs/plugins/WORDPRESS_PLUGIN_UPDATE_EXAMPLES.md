# WordPress Plugin Update Examples

## Admin-Beispiele
### Updates manuell pruefen
- WordPress Admin: `Dashboard -> Updates -> Check Again`

### Update installieren
- WordPress Admin: `Dashboard -> Updates -> Update Now`

## Entwickler-Beispiele
### Release vorbereiten
```bash
PLUGIN_SLUG="themisdb-downloads"
VERSION="1.2.0"
TAG_NAME="${PLUGIN_SLUG}/v${VERSION}"
```

### Release erstellen
```bash
git add .
git commit -m "Release ${PLUGIN_SLUG} v${VERSION}"
git push

git tag "${TAG_NAME}"
git push origin "${TAG_NAME}"
```

### ZIP bauen und hochladen
```bash
# Nested Layout
(cd wordpress-plugins && zip -r "../dist/${PLUGIN_SLUG}.zip" "${PLUGIN_SLUG}")

gh release create "${TAG_NAME}" \
  "dist/${PLUGIN_SLUG}.zip" \
  --title "${PLUGIN_SLUG} v${VERSION}" \
  --generate-notes
```

## CI/CD-Beispiele
### Dry-Run lokal
```powershell
./scripts/wordpress-plugin-dry-run-release.ps1 -PluginSlug themisdb-downloads -Version 1.2.0
```

### Batch-Release lokal
```powershell
./scripts/wordpress-plugin-release-batch.ps1 -Repository "makr-code/wordpressPlugins"
```

### GitHub Actions (manuell)
Workflow:
- `.github/workflows/wordpress-plugin-release.yml`

Runbook:
- `docs/ci-cd/WORDPRESS_PLUGIN_OPERATIONS.md`

Inputs:
- `plugin_slug`
- `version`
- `target_ref`
- `dry_run`

## API-Checks
```bash
curl -I "https://api.github.com/repos/makr-code/wordpressPlugins/releases?per_page=10"
```

```bash
curl "https://raw.githubusercontent.com/makr-code/wordpressPlugins/main/<plugin-slug>/update-info.json"
```

## Migration-Hinweis
Diese Doku wurde aus `wordpress-plugins/UPDATE_EXAMPLES.md` nach `docs/` ueberfuehrt. Die alte Datei bleibt als Redirect erhalten.
