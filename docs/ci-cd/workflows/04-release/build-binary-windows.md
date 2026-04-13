# Build Binary Release · Windows

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/04-release_build-binary-windows.yml`

## Aufgabe

CI-Workflow zur Erstellung von Windows-Release-Binaries (x64) inklusive ZIP-Paketierung, optionaler MSI-Erzeugung (WIX/CPack) und optionalem Upload auf den zugehörigen GitHub Release.

## Auslöser (Triggers)

- **`push`** — Automatisch bei Tags `v*`
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `tag_name` | Tag to build (z. B. `v1.9.0`) | ✅ | — |
| `dry_run` | Build/Paketierung ohne Release-Upload | — | `true` |

## Jobs

### `build-windows`
**Anzeigename:** Build & Package – Windows x64

**Läuft auf:** `windows-latest`

**Schritte:**

- **Resolve tag and version** — Tag/Version aus Event oder Input ermitteln
- **Checkout repository** — `actions/checkout@v4`
- **Enable Developer Command Prompt (MSVC)** — `ilammy/msvc-dev-cmd@v1`
- **Cache vcpkg** — `actions/cache@v4`
- **Setup vcpkg** — vcpkg bootstrappen und Dependencies installieren
- **Configure CMake** — Release-Konfiguration erzeugen
- **Build** — Projekt kompilieren
- **Package with CPack (ZIP + optional MSI)** — ZIP-Artefakte erzeugen und MSI-Erzeugung versuchen
- **Collect packages and generate checksums** — Artefakte sammeln und SHA256 erstellen
- **Upload packages as workflow artifacts** — `actions/upload-artifact@v4`
- **Write job summary** — Ergebniszusammenfassung in `$GITHUB_STEP_SUMMARY`

## Berechtigungen

- `contents`: `write`

## Verwandte Ressourcen

- [Binary Package Layout & Installer Policy](binary-package-layout.md)
- [Workflow-Datei](../../.github/workflows/04-release_build-binary-windows.yml)
- [Alle Workflows](../README.md)
