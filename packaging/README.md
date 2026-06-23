# Packaging README

Dieser Ordner enthält Vorlagen und Hinweise für das Packaging/Deployment von ThemisDB.

Konzept
- Basis: Verwende `cmake/CPackConfig.cmake` für konsistente Archive/Installer (ZIP, TGZ, DEB, RPM, WIX).
- Editions-Logik: setze `-DTHEMIS_EDITION_STRING=<minimal|community|enterprise|hyperscaler>` beim `cmake`-Configure.
- CI: Erzeuge Builds pro Edition mit CMake-Presets und nutze `cpack --config <build-dir>/CPackConfig.cmake`.

Beispiel: Community ZIP (Windows)
```powershell
cmake -S . -B build-community-release --preset windows-release
cmake --build build-community-release --preset windows-release --config Release
cpack -G ZIP --config build-community-release/CPackConfig.cmake
```

Beispiel: Enterprise ZIP
```powershell
cmake -S . -B build-enterprise-release --preset windows-release -DTHEMIS_EDITION_STRING=enterprise
cmake --build build-enterprise-release --preset windows-release --config Release
cpack -G ZIP --config build-enterprise-release/CPackConfig.cmake
```

Wichtige Hinweise
- `install()`-Rules in `CMakeLists.txt` müssen die Dateien/Verzeichnisse korrekt abdecken, damit CPack sie einsammelt.
- Systemd/Service-Files: lege `debian/themisdb.service` und preinst/postinst-Skripte im `debian/`-Ordner ab.
- Signierung: Signiere generierte Artefakte außerhalb von CPack oder erweitere CI mit Signatur-Schritt.

Weiteres
- Siehe `docs/CPACK_ANALYSIS.md` für die vollständige Analyse und Roadmap zur CPack-Integration.
# packaging

Pfad: `packaging`

## Zweck
Dieser Ordner enthält 5 Unterordner und 2 Dateien und bildet einen abgegrenzten Teil der Repository-Struktur.

## Unterordner
- `chocolatey/`
- `enterprise/`
- `homebrew/`
- `hyperscaler/`
- `winget/`

## Dateien nach Kategorien
- **Sonstiges**: `PKGBUILD`, `themisdb.spec`

## Hinweise
- Änderungen in diesem Ordner sollten mit den übergeordneten Architektur- und Sicherheitsrichtlinien des Projekts abgestimmt werden.
- Für tieferliegende Teilbereiche existieren ggf. zusätzliche README- und Moduldokumente.

## Packaging-Targets (CMake)
Die Packaging-Pipeline ist in getrennte CMake-Targets aufgeteilt.

Standard-Ausgabeziel fuer erzeugte Pakete: `releases/`

Typische Struktur:

- `releases/x64-windows/debug`
- `releases/x64-windows/release`
- `releases/x64-linux/debug`
- `releases/x64-linux/release`

- `package-deps`: Baut/aktualisiert vcpkg-Abhängigkeitspakete.
- `package-deps-windows-debug`: Abhängigkeitspakete für `x64-windows` (Debug).
- `package-deps-windows-release`: Abhängigkeitspakete für `x64-windows` (Release).
- `package-deps-linux-debug`: Abhängigkeitspakete für `x64-linux` (Debug).
- `package-deps-linux-release`: Abhängigkeitspakete für `x64-linux` (Release).
- `package-zip`: Erstellt ein ZIP-Paket (primär via CPack).
- `package-msi`: Erstellt ein MSI-Paket (Windows, via WiX/CPack wenn verfügbar).
- `package-installer`: Erstellt ein Installer-Paket (NSIS, sonst WiX-Fallback wenn verfügbar).
- `package-tgz`: Erstellt ein `.tar.gz`-Paket.
- `package-tools-report`: Zeigt erkannte Packaging-Tools und Capability-Flags.
- `package-all`: Aggregiert die verfügbaren Packaging-Typen.

Kompatibilitäts-Aliase (bestehende CI/Script-Namen):

- `build-all-packages`
- `build-packages-windows-debug`
- `build-packages-windows-release`
- `build-packages-linux-debug`
- `build-packages-linux-release`

## Lokaler Release-Prozess (ohne CI/CD)

Release-Default (Best-Practice):

- Build-Ordner: `build/windows-release`
- Release ZIP: Runtime-only
- Debug ZIP: Runtime + Development (`include/`, `*.lib`)
- Tag-Policy: stabile Tags `vX.Y.Z`; Pre-Release nur mit explizitem Schalter
- Docker-Tags: `vX.Y.Z`, `X.Y`, `latest` nur bei stable
- Compliance-Artefakte: SHA256, CycloneDX SBOM, GPG-Signatur

Empfohlener lokaler Ablauf:

1. `vcpkg`-Pakete bauen: `vcpkg/vcpkg.exe install --triplet x64-windows`
2. Konfigurieren: `cmake --preset windows-release`
3. Build: `cmake --build --preset windows-release --parallel`
4. Tests: `ctest --preset windows-release --output-on-failure`
5. Deploy-ZIP: `cmake --build build/windows-release --target package-zip --parallel`

Automatisierter End-to-End-Flow (lokal):

- `scripts/release/publish-local-release.ps1 -Tag v1.9.0`
- Optionales Pre-Release: `-AllowPreRelease`
- Optionales GitHub Release: `-PublishGitHub`
- Optionales DockerHub Publish: `-PublishDocker`

_Automatisch erzeugt/aktualisiert am 2026-04-17._
