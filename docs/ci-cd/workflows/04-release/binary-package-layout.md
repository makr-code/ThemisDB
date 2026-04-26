# Binary Package Layout & Installer Policy

🔄 **Release Packaging Standard**

> **Gilt für:** `.github/workflows/04-release_build-binary-linux.yml`, `.github/workflows/04-release_build-binary-windows.yml`

## Ziel

Verbindliche Definition der Release-Binärpakete für Linux und Windows: Formate, enthaltene Ordner/Dateien, Dateinamen-Konventionen und Installer-Strategie.

## Paketformate je Plattform

| Plattform | Primäre Formate | Optional | Workflow |
|-----------|------------------|----------|----------|
| Linux x86_64 | `TGZ`, `DEB`, `RPM` | — | `04-release_build-binary-linux.yml` |
| Windows x64 | `ZIP` | `MSI` (via CPack/WIX) | `04-release_build-binary-windows.yml` |

## Verzeichnisstruktur innerhalb von Binärpaketen

Die Struktur wird durch CMake-`install(...)`-Regeln bestimmt.

| Pfad im Paket | Muss enthalten | Quelle |
|---------------|----------------|--------|
| `bin/` | Laufzeit-Binaries (`themis_server`, CLI-Tools wie `themis-export`, `themis-model`) | `cmake/CMakeLists.txt` |
| `lib/` | Shared/Static Libraries (`*.so`, `*.dll`, `*.a`, `*.lib`) | `cmake/CMakeLists.txt`, `cmake/ModularBuild.cmake` |
| `include/` | Public Header für SDK/Integration | `cmake/CMakeLists.txt` |

### Bedingt enthaltene Inhalte

| Pfad im Paket | Bedingung |
|---------------|-----------|
| `data/docs.db` | Wenn vorgebaut vorhanden oder per Build-Option erzeugt |
| `models/` | Verbindlich in Release-Paketen (`THEMIS_MODELS_MODE=PACKAGE`) |
| `bin/benchmarks/` | Nur wenn Benchmarks gebaut/installiert werden |

## Release-Asset-Struktur (GitHub Release)

Assets werden flach am Release-Tag veröffentlicht:

- Linux: `themisdb-<VERSION>-Linux-x86_64.<ext>` + `SHA256SUMS-linux.txt`
- Windows: `ThemisDB-<VERSION>-Windows-x64.zip` + optional `ThemisDB-<VERSION>-Windows-x64.msi` + `SHA256SUMS-windows.txt`

## Installer-Strategie (Windows MSI)

- `ZIP` ist das **verpflichtende** Windows-Artefakt.
- `MSI` ist **optional**, wird aber in CI per `CPack -G WIX` versucht.
- Falls WIX/MSI auf dem Runner nicht verfügbar ist, darf der Build mit ZIP-only weiterlaufen.
- Release-Dokumentation muss den MSI-Status pro Version klar ausweisen.

## Mindestanforderungen für Release-Abnahme

- Linux: mindestens ein erfolgreiches Binärformat (`TGZ`, `DEB` oder `RPM`) + SHA256-Liste.
- Windows: mindestens `ZIP` + SHA256-Liste.
- Checksums müssen alle hochgeladenen Binärartefakte der jeweiligen Plattform abdecken.

## CMake-Steuerung (lokal vs. CI/CD)

Für reproduzierbares Verhalten werden folgende Cache-Variablen verwendet:

- `THEMIS_DOCS_DB_MODE` = `AUTO|BUILD|PREBUILT|OFF`
- `THEMIS_DOCS_DB_PREBUILT_PATH` = Pfad zu vorgebautem `docs.db`
- `THEMIS_DOCS_DB_CACHE_PATH` = lokaler Cachepfad für `docs.db`
- `THEMIS_MODELS_MODE` = `AUTO|PACKAGE|SKIP`
- `THEMIS_MODELS_SOURCE_DIR` = Quellpfad für Runtime-Modelle

Empfehlung:

- Lokal: `THEMIS_DOCS_DB_MODE=AUTO`, `THEMIS_MODELS_MODE=AUTO`
- CI/CD Release: `THEMIS_DOCS_DB_MODE=AUTO`, `THEMIS_MODELS_MODE=PACKAGE`

## Verwandte Ressourcen

- [Linux Binary Build](build-binary-linux.md)
- [Windows Binary Build](build-binary-windows.md)
- [Create Release Archive](create-release-archive.md)
