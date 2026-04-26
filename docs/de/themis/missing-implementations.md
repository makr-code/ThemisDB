[docs](../../README.md) > [de](../README.md) > [themis](./index.md) > [reference](./missing-implementations.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/themis/ROADMAP.md`
- `include/themis/ROADMAP.md`
- `include/themis/runtime_license_gate.h`
- `include/themis/module_signature_verifier.h`

**Bezug / Reference:**
- Issue: [MODULE] themis
- Kontext: Fehlende Implementierungen und offene Lücken für Milestone `v1.8.0` im Modul `themis`.

---

# Missing Implementations Report — themis

| Gap | Impact | Evidence | Priorität | Folge-Issue |
|-----|--------|----------|-----------|-------------|
| Vollständige Modularisierung (monolithisch → loadable modules) | Verzögert Entkopplung und modulare Deployments; erhöht Integrationskomplexität | `src/themis/ROADMAP.md` („Full modularization …“ offen) | Hoch | #2472 |
| Zertifikatskettenprüfung für signierte Module (Enterprise CA) | Signaturprüfung bleibt auf aktuelle Plattformpfade begrenzt; fehlende Chain/Revocation-Abdeckung | `include/themis/ROADMAP.md` (offen), `include/themis/module_signature_verifier.h` ohne Chain-API | Hoch | Neu anzulegen (Roadmap-Eintrag vorhanden) |
| Runtime-Upgrade-API für Edition (`EditionManager::upgrade(token)`) | Lizenz-/Edition-Upgrade ohne Neustart nicht möglich | `include/themis/ROADMAP.md` (geplant, offen), keine entsprechende API in `include/themis/edition_manager.h` | Mittel | Neu anzulegen (Roadmap-Eintrag vorhanden) |
| Async License Refresh im RuntimeGate | Langläufer benötigen weiter manuelles/externes Refresh-Handling | `include/themis/ROADMAP.md` (Target v1.9.0), `include/themis/runtime_license_gate.h` ohne `scheduleRefresh(...)` | Mittel | Neu anzulegen (Roadmap-Eintrag vorhanden) |
| `BuildInfo::plugin_api_version()` für ABI-Kompatibilität | Plugin-ABI-Kompatibilität muss extern/implizit geprüft werden | `include/themis/ROADMAP.md` (offen), keine API in `include/themis/build_info.h` | Mittel | Neu anzulegen (Roadmap-Eintrag vorhanden) |

## Ergänzende Doku-Lücken (Secondary-relevant)

| Gap | Impact | Evidence | Priorität | Folge-Issue |
|-----|--------|----------|-----------|-------------|
| `src/themis/ARCHITECTURE.md` enthält historische Planungsformulierungen | Erhöht Risiko falscher Architekturannahmen bei neuen Beiträgen | `src/themis/ARCHITECTURE.md` (mehrere „planned v1.7.0+“-Stellen) | Mittel | Neu anzulegen (Doku-Bereinigung) |
| `include/themis/ROADMAP.md` ohne explizite Abschnitte `Known Issues & Limitations` und `Breaking Changes` | Inkonsistenz zum Roadmap-Standard, erschwert moduleinheitliche Reviews | `include/themis/ROADMAP.md` | Niedrig | Neu anzulegen (Roadmap-Format-Harmonisierung) |
