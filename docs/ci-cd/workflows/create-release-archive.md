# [Manual] Create Release Archive

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🖱️ **Manuell**

> **Workflow-Datei (historisch):** .github/workflows/create-release-archive.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Manuell ausgelöster Workflow für: **Create Release Archive**.

## Auslöser (Triggers)

- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `version` | Release version (e.g., 1.4.0) | ✅ | — |
| `prerelease` | Mark as pre-release | — | `False` |

## Jobs

### `create-release`
**Anzeigename:** Create Release Archive

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout code** — `actions/checkout@v4`
- **Validate version** — `VERSION="${{ github.event.inputs.version }}"`
- **Create release archive (ZIP)** — `VERSION="${{ github.event.inputs.version }}"`
- **Generate SHA256 checksum** — `VERSION="${{ github.event.inputs.version }}"`
- **Create Git tag** — `VERSION="${{ github.event.inputs.version }}"`
- **Create GitHub Release** — `softprops/action-gh-release@v2`
- **Release summary** — `VERSION="${{ github.event.inputs.version }}"`

## Berechtigungen

- `contents`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/create-release-archive.yml)
- [Alle Workflows](README.md)


