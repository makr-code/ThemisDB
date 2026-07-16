# ThemisDB Runtime Layout (Canonical)

Diese Datei ist die zentrale Definition der Laufzeitumgebung fuer Packaging und Deployment.

## Zielstruktur im Release-Paket

- `bin/`: Server, CLI-Tools, Tests, Benchmarks, Runtime-DLLs (inkl. vcpkg-abhaengig)
- `lib/`: optionale Bibliotheken/Archive
- `config/`: Laufzeitkonfigurationen
- `data/`: Laufzeitdaten und optionale Dokumentationsdatenbank
- `models/`: deploybare Laufzeitmodelle (derzeit TinyLlama-Baseline)
- `certs/`: Zertifikatsmaterial
- `plugins/`: Plugin-Manifeste und pluginbezogene Ressourcen
- `docs/`: Packaging-/Runtime-Dokumentation

## Packaging-Regeln

1. `cmake/packaging/RuntimeLayout.cmake` ist die maschinenlesbare Quelle fuer Komponenten, Pflichtordner und Doku-Dateien.
2. ZIP-Packages kopieren zusaetzlich den gesamten Build-Ordner `bin/`, um alle erzeugten EXE/DLL/Test/Benchmark-Binaries mitzuliefern.
3. Windows-Install-Regeln erfassen Runtime-DLL-Abhaengigkeiten ueber `RUNTIME_DEPENDENCY_SET` und installieren sie nach `bin/`.
4. Einstiegsdokumente (`README.md`, `CHANGELOG.md`, `SETUP.md`, usw.) muessen im Paket-Root vorhanden sein.
5. `scripts/setup-runtime-env.ps1` ist das Standard-Setup-Tool fuer Windows-Laufzeitvariablen.

## Schalter fuer Test-/Benchmark-Deploy

- `THEMIS_PACKAGE_INCLUDE_TESTS`: Wenn `ON`, werden gebaute CTest-Binaries mit ausgeliefert.
- `THEMIS_PACKAGE_INCLUDE_BENCHMARKS`: Wenn `ON`, werden gebaute Benchmark-Binaries mit ausgeliefert.
- Standardwerte folgen den Build-Schaltern:
  - `THEMIS_BUILD_TESTS` -> `THEMIS_PACKAGE_INCLUDE_TESTS`
  - `THEMIS_BUILD_BENCHMARKS` -> `THEMIS_PACKAGE_INCLUDE_BENCHMARKS`

## OS-Zielorte

- Windows ZIP/MSI:
  - Paket-Root ist die Installationsbasis.
  - Ausfuehrbare Dateien und DLLs: `bin/`
  - Setup-Tool: `bin/setup-runtime-env.ps1`
- Linux TGZ/DEB/RPM:
  - Prefix: `/usr`
  - Binaries: `/usr/bin`
  - Libraries: `/usr/lib`
  - Includes: `/usr/include`
  - Runtime-Daten: `/usr/data`, `/usr/config`, `/usr/models` (paketabhaengig)
- macOS TGZ/ZIP:
  - Paket-Root analog Windows-ZIP (relativer Prefix)
  - Binaries in `bin/`, Runtime-Daten in den gleichnamigen Top-Level-Ordnern

## Wartung

Bei Anpassungen an Runtime-Ordnern, Einstiegsdokumenten oder Packaging-Komponenten muessen aktualisiert werden:

- `cmake/packaging/RuntimeLayout.cmake`
- `cmake/packaging/CreateZipPackage.cmake` (falls Verteilungslogik geaendert wird)
- `cmake/CMakeLists.txt` Install-Regeln (Runtime-Dependencies, Ordnerinstallation)
- diese Datei
