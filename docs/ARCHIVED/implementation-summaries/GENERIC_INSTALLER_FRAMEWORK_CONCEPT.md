# Generic Install/Uninstall/Update Framework (GIUF)

## Ziel

Ein eigenstaendiges, generisches Framework fuer Installation, Deinstallation und Updates von Desktop-Anwendungen auf Windows, Linux und macOS.

Das Framework soll:

- als separates Repository gepflegt werden,
- in verschiedenen Produkten wiederverwendbar sein (z. B. ThemisDB),
- Konfiguration und Setup-Regeln ausserhalb von Binaer-Artefakten halten,
- sichere Update-Mechanismen mit Integritaets- und Authentizitaetspruefung bieten.

## Leitprinzipien

- Product-agnostic Core: Engine ohne Produktwissen.
- Declarative Configuration: Verhalten ueber externe Konfigurationsdateien steuern.
- Secure by default: Signatur- und Hash-Pruefungen sind Pflicht.
- Atomic Operations: Install/Update nur ganz oder gar nicht.
- Rollback by design: Fehler fuehren zu konsistentem vorherigen Zustand.
- Portable Architecture: Einheitliche Kernlogik, kleine OS-spezifische Adapter.

## Repository-Vorschlag

Repository-Name (Vorschlag):

- `generic-installer-framework`

Lizenz (Vorschlag):

- MIT oder Apache-2.0

## Architekturuebersicht

### Hauptkomponenten

- Core Engine
  - Zustandsautomat fuer Install/Update/Uninstall
  - Orchestrierung der Schritte
- Configuration Layer
  - Laden und Validieren externer Produktkonfiguration
- Source Providers
  - GitHub Releases API
  - Optional spaeter: S3, Artefakt-Registry, interne Mirror
- Security Module
  - Signaturpruefung fuer Manifest/Config
  - SHA-256 Verifikation fuer Artefakte
- Artifact Module
  - Download, Resume, Retry, Entpacken
- Install Module
  - Staging, atomischer Wechsel, Rollback
- Uninstall Module
  - Entfernen installierter Artefakte inkl. Cleanup-Policy
- Platform Adapter
  - Dateisperren, Rechte, Pfadkonventionen, Shortcuts/Services
- UI Layer (optional pro Host-App)
  - Referenz-GUI und CLI

### Schichtenmodell

- API/CLI/UI
- Workflow Engine
- Domain Services (security, artifact, install, rollback)
- Infrastructure (filesystem, network, process, platform)

## Externe Konfiguration (ausserhalb Binary-Blob)

Konfiguration liegt als Dateien in einem separaten, versionierten Config-Paket oder Repository.

### Warum extern?

- Wiederverwendung in mehreren Produkten
- Aenderungen ohne Rebuild der Engine
- Trennung von Runtime-Logik und Produktpolitik
- Leichtes Tenanting/Branding/Channeling

### Konfigurationsarten

- Product Config (`product.yaml`)
  - Produkt-ID, Name, Support-URL
  - Installationsziele pro OS
  - Feature-Flags
- Channel Config (`channels.yaml`)
  - stable/beta/nightly
  - Rollout-Strategien
- Source Config (`sources.yaml`)
  - Release-Endpunkte, Mirrors, Timeouts, Retry-Policy
- Install Recipe (`install.recipe.yaml`)
  - Zu installierende Dateien
  - Rechte/Ownership
  - Post-Install-Schritte
- Uninstall Recipe (`uninstall.recipe.yaml`)
  - Entfernen, Preserve-Listen, Datenmigrationen
- Trust Config (`trust.yaml`)
  - Public Keys
  - Erlaubte Signaturalgorithmen
  - Mindestanforderungen an Hash/Signatur

### Signierung von Konfiguration

- Jede ausfuehrungsrelevante Konfigurationsdatei muss signiert sein.
- Die Engine akzeptiert nur gueltig signierte Konfiguration.
- Public Key Pinning in der Engine oder via gesichertem Trust Store.

## Release- und Manifestmodell

### Release Manifest (pro Version)

Dateien:

- `manifest.json`
- `manifest.sig`

Inhalte von `manifest.json`:

- Produkt, Version, Channel
- kompatible Zielplattformen/Architekturen
- Artefakt-URLs
- `sha256` je Artefakt
- Groesse, optionale Delta-Infos
- Mindestversion des Installers

Signaturfluss:

1. `manifest.json` laden
2. `manifest.sig` pruefen (Authentizitaet)
3. Artefakt laden
4. SHA-256 pruefen (Integritaet)
5. erst danach entpacken/installieren

## Workflows

### Install

1. Konfiguration laden und signaturpruefen
2. Plattform/Arch erkennen
3. passendes Artefakt im Manifest waehlen
4. downloaden (mit Resume/Retry)
5. Hash pruefen
6. in Staging entpacken
7. atomischer Switch auf Zielpfad
8. Post-Install-Hooks ausfuehren
9. Erfolg markieren, altes Staging bereinigen

### Update

1. installierte Version ermitteln
2. Update-Check nach Policy (Channel, Rollout, Min-Version)
3. Manifest + Signatur validieren
4. Download + Hash
5. Staging + Preflight Checks (Disk, Locks, Rechte)
6. atomisches Update
7. Health Check
8. bei Fehler: Rollback auf Vorversion

### Uninstall

1. installierten Zustand lesen
2. laufende Prozesse kontrolliert beenden
3. Dateien gemaess Uninstall-Recipe entfernen
4. benutzerbezogene Daten gemaess Policy optional erhalten
5. Registrierungen/Shortcuts/Services entfernen
6. Uninstall-Status protokollieren

## Datenmodell fuer installierten Zustand

State-Datei pro Installation (z. B. `installation-state.json`):

- product_id
- installed_version
- install_path
- installed_files
- previous_version_snapshot
- rollback_metadata
- install_timestamp

## Sicherheitskonzept

- Pflicht: TLS fuer alle Downloads
- Pflicht: Signaturpruefung fuer Manifest und Konfiguration
- Pflicht: SHA-256 fuer Artefakte
- Schutz gegen Downgrade ohne explizite Policy-Freigabe
- Schutz gegen Partial Updates durch atomische Switches
- Audit-Log fuer sicherheitsrelevante Entscheidungen

## Universeller Einsatz (Multi-Produkt)

Das Framework bleibt generisch durch:

- keine hartcodierten Produktnamen oder Pfade im Core
- Produktverhalten nur ueber externe Config/Recipes
- optionale Product Plugins fuer Sonderlogik
- klare API fuer Host-Integration (GUI/CLI/Service)

## Referenz-Tech-Stack (Open Source)

- Sprache: C++20
- Build: CMake
- Netzwerk: libcurl
- JSON: nlohmann/json
- YAML: yaml-cpp
- Kryptografie: OpenSSL oder libsodium
- Archiv: libarchive
- Logging: spdlog
- Tests: GoogleTest
- GUI (optional): wxWidgets

## Repository-Struktur (Vorschlag)

```text
generic-installer-framework/
  CMakeLists.txt
  cmake/
  include/giuf/
    api/
    core/
    security/
    platform/
  src/
    core/
    security/
    artifact/
    install/
    uninstall/
    platform/
  tools/
    giuf-cli/
    giuf-sign/
    giuf-verify/
  examples/
    sample-product-config/
  schemas/
    product.schema.json
    manifest.schema.json
    recipe.schema.json
  docs/
    architecture.md
    security-model.md
    integration-guide.md
  tests/
    unit/
    integration/
```

## API-Oberflaeche (MVP)

- `checkForUpdates(context) -> UpdateCheckResult`
- `install(context, installPlan) -> OperationResult`
- `update(context, updatePlan) -> OperationResult`
- `uninstall(context, uninstallPlan) -> OperationResult`
- `verifyManifest(manifest, signature) -> VerifyResult`
- `verifyArtifact(path, expectedSha256) -> VerifyResult`

## Setup- und Betriebsmodell ausserhalb des Binaries

### Setup-Artefakte

- signierte Produktkonfiguration
- signierte Release-Manifeste
- optionale Branding-Dateien (Icons, Texte, Lokalisierungen)
- Environment-overrides fuer Enterprise Deployment

### Betriebsmodus

- Online-Modus: Manifest/Artefakte direkt aus Quelle
- Mirror-Modus: interner Proxy oder Cache
- Airgap-Modus: lokal bereitgestellte Manifeste und Artefakte

## CI/CD-Konzept

- Build-Matrix fuer Win/Linux/macOS
- Erzeugung von Release-Artefakten
- SHA-256 Berechnung je Artefakt
- Manifest-Erzeugung
- Signierung mit geschuetztem Release-Key
- Verifikationsjob als harter Gate
- Publish in GitHub Release

## Roadmap (kurz)

### Phase 1: Foundation

- Core-State-Machine
- Config Loader + Schema Validation
- Manifest Verify

### Phase 2: Install/Update Engine

- Download + Resume
- Hash/Signaturfluss
- Atomic Install + Rollback

### Phase 3: Uninstall + Platform Adapter

- Deinstallationspfad
- OS-spezifische Integrationen

### Phase 4: Tooling und Integration

- CLI Tools
- Beispielkonfiguration
- Integration Guide fuer ThemisDB

### Phase 5: Hardening

- Threat-model-basierte Tests
- Fault Injection
- Performance/Robustness

## ThemisDB-Integration (spaeter)

- ThemisDB stellt nur Produktkonfiguration und Release-Manifeste bereit.
- GIUF bleibt eigenes Repo und eigenes Release-Lifecycle.
- ThemisDB kann GIUF als:
  - eingebettete Bibliothek,
  - separaten Updater-Prozess,
  - oder externes Installer-Tool nutzen.

## Entscheidungsempfehlung

- GIUF als separates, generisches OSS-Repository starten.
- Konfiguration, Setup-Rezepte und Trust-Policies strikt ausserhalb der Engine-Binaerdatei halten.
- Signatur + SHA-256 als nicht deaktivierbare Sicherheitsbasis fest verdrahten.
