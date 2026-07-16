# [Manual] Create Release Archive

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🖱️ **Manuell**

> **Workflow-Datei (historisch):** .github/workflows/04-release_create-release-archive.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Manuell ausgelöster Workflow für: **Create Release Archive**.

## Auslöser (Triggers)

- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `version` | Version number (e.g., 1.0.0, 1.3.4) | ✅ | — |

## Jobs

### `create-release-archive`
**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Validate version format** — `VERSION="${{ github.event.inputs.version }}"`
- **Find commit for version** — `VERSION="${{ env.VERSION }}"`
- **Create source archive** — `chmod +x scripts/archive-version.sh`
- **Display archive info** — `ls -lh themisdb-${{ env.VERSION }}-source.zip`
- **Read SHA256 checksum into environment** — `CHECKSUM=$(cat themisdb-${{ env.VERSION }}-source.zip.sha256)`
- **Set up git user** — `git config --global user.name "github-actions[bot]"`
- **Create or update Git tag** — `VERSION="${{ env.VERSION }}"`
- **Create GitHub Release** — `softprops/action-gh-release@v2`
- **Write job summary** — `echo "## 📦 Create Release Archive – v${{ env.VERSION }}" >> "$GITHUB_STEP_SUMMAR`

## Berechtigungen

- `contents`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/04-release_create-release-archive.yml)
- [Alle Workflows](../README.md)


