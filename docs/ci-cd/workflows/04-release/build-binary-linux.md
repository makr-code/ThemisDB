# Build Binary Release · Linux

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/04-release_build-binary-linux.yml`

## Aufgabe

CI-Workflow zur Erstellung von Linux-Release-Binaries (x86_64) inklusive Paketierung und optionalem Upload auf den zugehörigen GitHub Release.

## Auslöser (Triggers)

- **`push`** — Automatisch bei Tags `v*`
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `tag_name` | Tag to build (z. B. `v1.9.0`) | ✅ | — |
| `dry_run` | Build/Paketierung ohne Release-Upload | — | `true` |

## Jobs

### `build-linux`
**Anzeigename:** Build & Package – Linux x86_64

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Resolve tag and version** — Tag/Version aus Event oder Input ermitteln
- **Checkout repository** — `actions/checkout@v4`
- **Install system dependencies** — Build- und Packaging-Abhängigkeiten installieren
- **Cache vcpkg** — `actions/cache@v4`
- **Setup vcpkg** — vcpkg bootstrappen und Dependencies installieren
- **Configure CMake** — Release-Konfiguration erzeugen
- **Build** — Projekt kompilieren
- **Package with CPack (TGZ + DEB + RPM)** — Binärpakete erzeugen
- **Collect packages and generate checksums** — Artefakte sammeln und SHA256 erstellen
- **Upload packages as workflow artifacts** — `actions/upload-artifact@v4`
- **Write job summary** — Ergebniszusammenfassung in `$GITHUB_STEP_SUMMARY`

## Berechtigungen

- `contents`: `write`

## Verwandte Ressourcen

- [Binary Package Layout & Installer Policy](binary-package-layout.md)
- [Workflow-Datei](../../.github/workflows/04-release_build-binary-linux.yml)
- [Alle Workflows](../README.md)
