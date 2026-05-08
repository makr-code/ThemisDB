# ThemisDB v1.9.0-alpha — Release Notes

**Release Date:** 2026-04-26
**Type:** Alpha Pre-Release
**Previous Version:** v1.8.2
**Milestone:** v1.9.0-alpha

## Uebersicht

ThemisDB v1.9.0-alpha ist ein Vorab-Release fuer das Windows-Deployment und die Release-Vorbereitung.
Der Schwerpunkt liegt auf reproduzierbaren ZIP- und MSI-Artefakten, einem zentralen Laufzeit-Logging
unter `logs/` sowie der Vereinfachung der manuellen Release-Strategie.

## Enthaltene Artefakte

- `ThemisDB-COMMUNITY-1.9.0-alpha-windows-x64.zip`
- `ThemisDB-COMMUNITY-1.9.0-alpha-windows-x64.msi`

## Wichtigste Aenderungen

### Windows Packaging

- ZIP- und MSI-Artefakte wurden fuer den Community-Windows-Release neu erzeugt.
- Das MSI-Packaging wurde von doppelten WiX-Komponenten fuer `ggml`/`llama`-DLLs bereinigt.
- Runtime-Komponenten werden fuer das Packaging konsistent dem `runtime`-Component zugeordnet.

### Zentrales Logging

- Deployment-Artefakte enthalten jetzt ein zentrales `logs/`-Verzeichnis.
- Der Server schreibt sein rotierendes Laufzeitlog nach `logs/themis_server.log`.
- Audit-Logs werden ebenfalls unter `logs/` abgelegt.

### Release-Dokumentation

- Die Release-Strategie wurde auf einen kompakten, manuellen, CI-freien Prozess reduziert.
- GitHub-Milestones und zugeordnete Issues sind als feste Release-Anknuepfungspunkte definiert.
- Ein Release-Tag steht fuer genau einen freigegebenen Quellstand; ZIP und MSI erhalten keinen separaten Tag.

## Hinweise

- Dies ist ein Alpha-Release und kein stabiler Produktionsstand.
- Der Release dient der Validierung des Windows-Deployments, der Paketstruktur und des manuellen Release-Prozesses.
- Die Release-Artefakte wurden lokal erfolgreich gebaut und aus dem Paketlayout gestartet.

## Upgrade Und Rollback

- Bestehende produktive Installationen sollten noch nicht auf diesen Alpha-Stand gehoben werden.
- Bei Problemen auf den vorherigen stabilen Release zurueckgehen und einen neuen Patch- oder RC-Stand erzeugen.
