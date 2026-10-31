> **Source review:** 2026-10-31. Anforderungen unten unterscheiden Modulgarantien von Deployment-Aufgaben; source inspection is not deployment sign-off.

# ThemisDB Document Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument grenzt die im Modulcode vorhandenen Garantien von Anforderungen ab, die ein Produktionsadapter oder Deployment zusätzlich nachweisen muss. Der Modulcode enthält keine vollständige Produktionskonfiguration oder Deployment-Validierung.

## Dokumentabgrenzung (Canonical Split)

- **`src/document/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/document/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/document/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/document/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Im Modulcode nachgewiesene Grenzen

- `InMemoryDocumentStore` hält Datensätze im Prozessspeicher; ein `IDocumentStore` Vertrag allein stellt keine dauerhafte Persistenz bereit.
- `InMemoryDocumentSchemaEvolution` prüft registrierte Pflichtfelder und JSON-Typen. `StoreBackedRoundTripEditor` ruft diese Validierung nicht selbst auf.
- Der Round-Trip-Editor speichert Snapshots über die `IDocumentStore` Schnittstelle und gibt Store-Fehler zurück.
- Die geprüften Dokument-Header implementieren keine deploymentweite Startkonfiguration, Authentisierung/Autorisierung, Ressourcenlimits, externe Verbindungsrichtlinien oder Produktions-Audit-Pipeline.
- Der XDOMEA-In-Memory-Connector ersetzt keine XSD-Validierung; siehe [SECURITY.md](SECURITY.md).

## TODO für Produktionsintegrationen

Der Betreiber muss für die konkrete Integration nachweisen, dass:

- ein geeigneter persistenter Store eingesetzt und dessen Ausfall-/Wiederherstellungsverhalten geprüft wird;
- Autorisierung und Zugriffskontrolle außerhalb der bloßen Dokument-API durchgesetzt werden;
- benötigte Schema- und XML/XSD-Validierung an der Integrationsgrenze ausgeführt wird;
- Größen-/Zeitlimits, Fehlerprotokollierung und Audit-Anforderungen konfiguriert sind.

Diese Punkte sind Deployment-Anforderungen, keine bereits vom hier geprüften In-Memory-Code garantierten Kontrollen. TODO: den verantwortlichen Integrationsowner und konkrete Nachweise pro Deployment dokumentieren.

## Prüfbasis

Geprüft am 2026-10-31: `document_store.h`, `document_manager.h`, `document_schema_evolution.h`, `xdomea_connector.h`, `round_trip_editor.h` und `round_trip_editor.cpp`. Keine Produktionsumgebung oder Laufzeitkonfiguration wurde auditiert.
