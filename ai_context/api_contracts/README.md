# API Contracts (machine-readable)

Dieses Verzeichnis enthält API-Vertragsübersichten in tabellarischer Form.

## Ziel

- Öffentliche Schnittstellen für KI-Agenten eindeutig und schnell erfassbar machen
- Vertragsverletzungen (Preconditions/Postconditions/Error-Semantik) früh erkennen

## Tabellen-Schema (verbindlich)

| API | Namespace/Typ | Input Contract | Output Contract | Errors | Thread-Safety | Ownership/Lifetime | Notes |
|---|---|---|---|---|---|---|---|

## Befüllung

- Pro Modul eigene Datei (z. B. `tensor_api_contracts.md`)
- Nur verifizierte Informationen aus `include/**`, `src/**`, Tests übernehmen
- Bei Verhaltensänderungen im selben PR synchron aktualisieren
