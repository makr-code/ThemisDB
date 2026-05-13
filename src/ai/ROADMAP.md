> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# AI Module Roadmap

## Current Status

v1.9.x – Das `ai`-Modul liefert eine produktive Phase-1-Basis für AI-gestützte Plugin-Generierung über `AIPluginGenerator` mit Eingabevalidierung und strukturiertem Fehlerpfad.

## In Progress

_(Kein aktiver Dokumentations-Task — alle Phase-1-Doku-Items abgeschlossen.)_

## Planned Features

- [ ] LLM-Endpunktverdrahtung in `AIPluginGenerator::generatePlugin` inkl. Response-Parsing (Target: Q3 2026)
- [ ] Security-Sandbox-Pipeline für generierten Code (Target: Q3 2026)
- [ ] Erweiterte Prompt-Validierung für `required_capabilities` und `dependencies` (Target: Q3 2026)
- [ ] Fehlercodes differenzieren (Validation vs. Transport vs. Parse) für bessere Betriebsdiagnostik (Target: Q3 2026)

## Implementation Phases

### Phase 1: Design / API-Vertrag
- [x] Öffentliche API für Prompt-/Result-Typen in `include/ai/ai_plugin_generator.h` definiert
- [x] Konfigurationsvertrag (`llm_endpoint`, `sandbox_dir`, `output_dir`) dokumentiert

### Phase 2: Core-Implementierung
- [x] Konstruktor/Destruktor und Basisfluss in `src/ai/ai_plugin_generator.cpp` implementiert
- [ ] HTTP-Client-Aufrufpfad zu `Config::llm_endpoint` produktiv verdrahten (Target: Q3 2026)

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] Leerer Prompt-Text wird als Fehler zurückgegeben
- [x] Prompt-Längenlimit (>8192) wird abgefangen
- [ ] Retry-/Timeout-Verhalten für Endpunktfehler ergänzen (Target: Q3 2026)

### Phase 4: Tests
- [x] Fokus-Tests APG-01..06 (`tests/test_ai_plugin_generator.cpp`) für Construction/Validation/Fehlerpfade
- [ ] Integrationstest mit kontrollierter Mock-LLM-Antwort ergänzen (Target: Q3 2026)

### Phase 5: Performance/Hardening
- [ ] P95-Latenz-Budget für `generatePlugin()` mit Live-Endpunkt verifizieren (Target: Q4 2026)
- [ ] Prompt-/Response-Größenlimits und Logging-Redaction für sensible Inhalte härten (Target: Q4 2026)

### Phase 6: Dokumentation & Abnahme
- [x] Modul-README in `src/ai/README.md` ergänzt
- [x] Public-Header-README in `include/ai/README.md` ergänzt
- [x] Querverweise zu Roadmap/Future/sekundärer Doku gesetzt
- [x] ARCHITECTURE.md, AUDIT.md, SECURITY.md, CHANGELOG.md, PERFORMANCE_EXPECTATIONS.md erstellt

## Production Readiness Checklist

- [x] Öffentliche Header-Entry-Points dokumentiert
- [x] Laufzeitverhalten und aktuelle Grenzen dokumentiert
- [x] Troubleshooting-Hinweise für bekannte Fehlerpfade vorhanden
- [I] Live-LLM-Integration noch offen (Issue folgt aus Phase-2-Task)
- [ ] End-to-End-Integrationstest gegen verdrahteten Endpunkt (Target: Q3 2026)

## Known Issues & Limitations

- `generatePlugin()` gibt bei validem Prompt aktuell einen strukturierten Fehler zurück, da der Live-Endpunkt noch nicht verdrahtet ist.
- `validatePrompt()` prüft derzeit nur `description` (leer / >8192), nicht jedoch Feldkonsistenz für `required_capabilities`/`dependencies`.
- `GeneratedPlugin` wird in der aktuellen Phase nicht befüllt.

## Breaking Changes

- Der öffentliche API-Vertrag (`PluginGenerationPrompt`, `GeneratedPlugin`, `AIPluginGenerator`) gilt aktuell als stabil im v1.x-Kontext.
- Eine Umbenennung oder semantische Änderung zentraler Typen/Felder im Header wäre breaking und benötigt Major-Version-Planung.

## See Also

- Implementierungsübersicht: [`README.md`](./README.md)
- Public API: [`../../include/ai/README.md`](../../include/ai/README.md)
- Future Enhancements: [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- Modulübergreifende Roadmap: [`../ROADMAP.md`](../ROADMAP.md)
