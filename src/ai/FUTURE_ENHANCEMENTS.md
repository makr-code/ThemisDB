> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: README.md · ROADMAP.md · ../../include/ai/README.md -->

# AI Module - Future Enhancements

## Scope

- Produktivverdrahtung von `AIPluginGenerator::generatePlugin()` gegen konfigurierbare LLM-Endpunkte.
- Strukturierte Transformation von LLM-Antworten in `GeneratedPlugin` inklusive Manifest-/Dependency-Felder.
- Security-Hardening des Generierungspfads (Eingabe, Ausgabe, Logging, Fehlerpfade).
- Test- und Observability-Ausbau für reproduzierbare, diagnosefähige Ausführung.

## Design Constraints

- Der API-Vertrag in `include/ai/ai_plugin_generator.h` bleibt für v1.x abwärtskompatibel; neue Felder nur additive Erweiterungen.
- `generatePlugin()` darf keine unvalidierten Promptdaten ungefiltert in Logs schreiben; max. gekürzte, redigierte Payload-Anteile.
- Netzwerkzugriffe auf LLM-Endpunkte müssen bounded sein (Timeout + begrenzte Retry-Strategie), um Hänger zu vermeiden.
- `GeneratedPlugin` darf nur zurückgegeben werden, wenn alle Pflichtteile (`header_code`, `implementation_code`, `manifest`) vorhanden und konsistent sind.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `AIPluginGenerator::validatePrompt(const PluginGenerationPrompt&)` | API-Aufrufer, `generatePlugin()` | Erweitern auf Feldkonsistenz (`required_capabilities`, `dependencies`) |
| `AIPluginGenerator::generatePlugin(const PluginGenerationPrompt&)` | CLI-/Service-Aufrufer | Live-Endpunktaufruf, Parse, Mapping auf `GeneratedPlugin` |
| `AIPluginGenerator::Config` | Initialisierung im Host | Endpunkt/Timeout/Output-Sandbox-Konfiguration (additiv) |
| `GeneratedPlugin` | Build-/Plugin-Pipeline | Muss konsistente Artefakte und Manifestdaten liefern |

## Implementation Notes

- Endpunktverdrahtung in `src/ai/ai_plugin_generator.cpp` mit klaren Fehlerklassen:
  - Validierungsfehler
  - Transport-/Timeoutfehler
  - Antwort-Parse-/Schemafehler
- Antwortschema für LLM-Ausgaben definieren und strikt validieren, bevor `GeneratedPlugin` befüllt wird.
- Sicherheitsgates vor Rückgabe:
  - Mindest-Validierung auf leere/inkonsistente Codeblöcke
  - Begrenzung maximaler Antwortgröße
- Logging auf Debug-Level nur mit gekürzten Inhalten und ohne potenziell sensitive Vollprompts.

## Test Strategy

- Unit: Erweiterung von APG-01..06 um Endpunkt-/Parse-/Fehlerklassifizierungsfälle.
- Integration: Kontrollierter Test-Endpunkt mit deterministischen Antworten (valid/invalid/timeout).
- Regression: Sicherstellen, dass bestehende Phase-1-Fehlerpfade weiterhin stabil strukturiert bleiben.
- Docs Validation: `docs-lint.py` + `link-check.py --internal-only` für betroffene Moduldocs.

## Performance Targets

- `generatePlugin()` P95-Latenz bei erfolgreichem Endpunktcall: ≤ 2.0 s für Standardprompt bis 4 KB.
- Validierungs-Overhead (`validatePrompt`) ≤ 2 ms für Prompts bis 8 KB.
- Fehlerpfad-Latenz bei Timeout: deterministisch durch konfiguriertes Timeout, keine unbegrenzte Blockierung.

## Security / Reliability

- Keine unredigierten sensitiven Prompt-/Response-Inhalte in persistenten Logs.
- Fail-closed bei Parse-/Schemafehlern: niemals teilweise befülltes `GeneratedPlugin` als Erfolg zurückgeben.
- Deterministische Fehlercodes/Fehlermeldungsklassen für Monitoring und Incident-Triage.
- Retry-Strategie begrenzen (max. Attempts + Backoff) und Endpunktfehler ohne Prozessabsturz zurückgeben.

## See Also

- Current Implementation: [`README.md`](./README.md)
- Roadmap: [`ROADMAP.md`](./ROADMAP.md)
- Public API: [`../../include/ai/README.md`](../../include/ai/README.md)
- Plugins Roadmap Context: [`../plugins/ROADMAP.md`](../plugins/ROADMAP.md)
