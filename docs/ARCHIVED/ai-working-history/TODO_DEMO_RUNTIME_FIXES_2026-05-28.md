# TODO: ThemisDB Demo Runtime Fixes (2026-05-28)

## Ziel
Die Kickstarter-Demo soll ohne Caveat-Warnungen durchlaufen, insbesondere fuer:
- Section 5 (LLM inference)
- Section 6 (Complex AQL)
- Section 7 (RAG)
- Section 8 (docs.db help mode)

## Aktueller Ist-Zustand (verifiziert)
- Section 5: HTTP 200 (LLM inference gruen)
- Section 6: stabiler Graph-Explain-Probepfad HTTP 200
- Section 7: HTTP 200 (RAG gruen via `/api/v1/llm/rag`)
- Section 8: HTTP 200 nach Bereitstellung von `data/docs_database.json`

## Prioritaet P0 (muss fuer Demo gruen werden)

- [ ] P0.1 LLM Runtime Initialisierung robust machen
  - Scope:
    - src/server/http_server.cpp
    - config/core/config.yaml
    - optional: model bootstrap helper in scripts/
  - Tasks:
    - Sicherstellen, dass beim Startup ein gueltiger default model Pfad konfiguriert ist.
    - Falls kein Modell geladen ist, bei Demo-Start gezielt model load triggern.
    - Fehlertext bei fehlendem Modell eindeutiger machen (inkl. erwarteter model path).
  - Akzeptanzkriterien:
    - POST /api/v1/llm/inference liefert HTTP 200 mit text.
    - POST /api/v1/llm/rag liefert HTTP 200 mit text und documents_retrieved.

- [x] P0.2 docs.db Help Endpoint im aktiven HTTP-Server verdrahten
  - Scope:
    - src/server/http_server.cpp
    - ggf. src/server/llm_api_handler.cpp
    - tools/themisctl.cpp (nur falls Route angepasst wird)
  - Tasks:
    - Route fuer POST /api/v1/llm/docs/query im aktiven Request-Pfad registrieren.
    - Optional auch /api/v1/llm/docs/config und /api/v1/llm/docs/troubleshoot registrieren.
    - Einheitliche Fehlerantworten (404 nur bei wirklich unbekannter Route).
  - Akzeptanzkriterien:
    - POST /api/v1/llm/docs/query liefert nicht mehr 404.
    - themisctl help --mode lora liefert HTTP 200 oder fachliche 4xx, aber kein Route-404.

- [x] P0.3 Demo-sicheren Section-6-Probepfad bereitstellen
  - Scope:
    - demo/kickstarter_demo_script.ps1
    - optional: aql translator/query engine if full function support intended
  - Tasks:
    - Kurzfristig: Section 6 auf garantiert verfuegbaren Explain-Probepfad umstellen.
    - Mittelfristig: Collection-AQL-Ausfuehrung (query endpoint) separat stabilisieren.
  - Akzeptanzkriterien:
    - Section 6 endet ohne Hard-Fail und liefert HTTP 200 auf dem Probepfad.

## Prioritaet P1 (Stabilisierung)

- [x] P1.1 Runtime-Preflight fuer Demo-Features
  - Scope:
    - demo/kickstarter_demo_script.ps1
  - Tasks:
    - Vor den Sections 5-8 aktive Feature-Readiness pruefen (health + route checks).
    - Bei fehlender Readiness klare actionable Meldung ausgeben.
  - Akzeptanzkriterien:
    - Keine unklaren Caveats ohne konkrete Ursache im Output.

- [ ] P1.2 Einheitliche Capability/Discovery Endpoints
  - Scope:
    - src/server/http_server.cpp
    - ggf. offene API/capability routing
  - Tasks:
    - Entweder /api/v1/capabilities bereitstellen oder in CLI sauber fallbacken.
    - Discovery-Ausgabe fuer Demo und Support reproduzierbar machen.
  - Akzeptanzkriterien:
    - Route-Discovery liefert konsistente Ergebnisse ohne 404-Ueberraschung.

## Prioritaet P2 (Qualitaet)

- [ ] P2.1 Integrationstest fuer Demo-Flow ohne Caveats
  - Scope:
    - tests/ (neue integration test suite)
    - demo/kickstarter_demo_script.ps1
  - Tasks:
    - Headless Testlauf fuer Sections 1-11 mit erwarteten HTTP-Codes.
    - Regression-Check fuer LLM/RAG/docs/AQL Pfade.
  - Akzeptanzkriterien:
    - Test faellt, sobald eine der vier kritischen Sections regressiert.

## Konkrete Verifikation nach Umsetzung

1. Server starten:
   - .\\build-msvc-windows-release\\bin\\themis_server.exe --db .\\demo\\data\\themis_db --port 8765 --allow-degraded-build --allow-stub-hsm

2. Manuelle API-Checks:
   - /api/v1/llm/health (GET)
   - /api/v1/llm/models/load (POST)
   - /api/v1/llm/inference (POST)
   - /api/v1/llm/rag (POST)
   - /api/v1/llm/docs/query (POST)

3. End-to-End Demo:
   - .\\demo\\kickstarter_demo_script.ps1
   - Erwartung: Keine Caveat-Liste fuer Sections 5-8.

## Umsetzungsstand (2026-05-28)

- Erledigt:
  - docs routes fuer `/api/v1/llm/docs/query`, `/api/v1/llm/docs/config`, `/api/v1/llm/docs/troubleshoot` im aktiven `HttpServer::routeRequest` verdrahtet
  - Demo-Prechecks fuer Sections 5-8 implementiert (inkl. klare SKIP-Meldungen)
  - Section 6 auf stabilen Graph-Explain-Probepfad (`/api/v1/graph/query/explain`) umgestellt
  - Section 7 auf direkten RAG-API-Call (`POST /api/v1/llm/rag`) umgestellt; vermeidet instabile `themisctl rag query` Transport-Abbrueche
  - LLM Auto-Load im Demo-Skript standardmaessig aktiviert (bevorzugt `models/phi4.gguf`, sonst erstes lokales `.gguf`; ENV-Override via `THEMIS_DEMO_LLM_MODEL_PATH` bleibt moeglich)
  - docs-Datenbank aus `tools/build_docs_db.ps1` erzeugt und als `data/docs_database.json` + `data/docs_artifact.json` bereitgestellt
  - Vollstaendiger Headless-Demo-Lauf (`THEMIS_DEMO_NO_PAUSE=1`) ohne Caveat-Liste verifiziert

- Offen (naechste Blocker):
  - P0.1 LLM Runtime sauber initialisieren (default model robust laden)
  - Query-Engine/Optimizer fuer Collection-AQL stabilisieren (derzeit `Optimized entity execution failed`)
  - Verifiziert: aktuelles Modell `models/gemma3-4b.gguf` ist mit dem laufenden llama-loader inkompatibel (`token_embd.weight` shape mismatch 262145 vs 262144)
  - Positiv verifiziert: `models/phi4.gguf` funktioniert fuer `/api/v1/llm/inference` und `/api/v1/llm/rag`, wenn `THEMIS_DEMO_LLM_MODEL_PATH` auf dieses Modell gesetzt ist

## Verantwortliche Module
- Server Routing/API: src/server/http_server.cpp
- LLM Handler: src/server/llm_api_handler.cpp
- CLI Surface: tools/themisctl.cpp
- Demo-Ablauf: demo/kickstarter_demo_script.ps1
- Runtime Config: config/core/config.yaml
