[docs](../../index.md) > [de](../index.md) > [llama_cpp](./index.md) > [reference](./reality-check.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/llama_cpp/llama_cpp_plugin.cpp`
- `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`
- `src/llama_cpp/README.md`
- `src/llama_cpp/ARCHITECTURE.md`
- `src/llama_cpp/AUDIT.md`

**Bezug / Reference:**
- Issue: `[MODULE] llama_cpp`
- Kontext: Task 1 — Reality-Check der Primary-Dokumentation gegen den Implementierungsstand.

---

# Reality-Check — llama_cpp

## Ergebnis

`llama_cpp` ist funktional implementiert (inkl. Streaming, Batch, Registrar, optionaler `LlamaWrapper`-Delegation), aber mehrere Primary-Dokumente sind inhaltlich veraltet.

## Abweichungen mit Evidence

| Bereich | Dokument-Claim | Beobachtung im Code | Evidence |
|---|---|---|---|
| Versions-/Reifestand | `src/llama_cpp/README.md` beschreibt v2.0.0/Beta mit 30 Tests | Implementierung und Tests enthalten v2.1.0-Features und 50 Tests (A–N) | `src/llama_cpp/README.md`, `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`, `tests/CMakeLists.txt:22155` |
| Capabilities | `src/llama_cpp/ARCHITECTURE.md` listet `supports_streaming=false` | Code setzt `supports_streaming=true` und `supports_batching=true` | `src/llama_cpp/ARCHITECTURE.md`, `src/llama_cpp/llama_cpp_plugin.cpp:311-317` |
| RAG-Fluss | `ARCHITECTURE.md` beschreibt simples Context-Concatenation-Pattern | `generateRAG()` nutzt `RAGContextAssembler` inkl. Budget-Berechnung | `src/llama_cpp/ARCHITECTURE.md`, `src/llama_cpp/llama_cpp_plugin.cpp:236-285` |
| Audit-Status | `src/llama_cpp/AUDIT.md` führt 30 Tests und fehlendes Streaming als Gap | Streaming (`generateStream`) ist implementiert und getestet, Suite umfasst 50 Tests | `src/llama_cpp/AUDIT.md`, `src/llama_cpp/llama_cpp_plugin.cpp:365-383`, `src/llama_cpp/tests/test_llama_cpp_plugin.cpp` |
| Compile-Flag-Hinweis | `FUTURE_ENHANCEMENTS.md` referenziert `THEMIS_ENABLE_LLAMA_CPP` | Build + Code nutzen `THEMIS_LLM_ENABLED` | `src/llama_cpp/FUTURE_ENHANCEMENTS.md:43`, `src/llama_cpp/CMakeLists.txt:46-50`, `src/llama_cpp/llama_cpp_plugin.cpp` |

## Konsolidierungsbedarf (Primary)

- `src/llama_cpp/README.md`: Versions-/Teststand an aktuelle Suite angleichen.
- `src/llama_cpp/ARCHITECTURE.md`: Capability-Tabelle und RAG-Datenfluss aktualisieren.
- `src/llama_cpp/AUDIT.md`: Findings auf aktuellen Funktionsumfang (50 Tests, Streaming vorhanden) anpassen.
- `src/llama_cpp/FUTURE_ENHANCEMENTS.md`: Makro-Namen und bereits gelieferte Features bereinigen.

## Update — v2.4.0 (2026-08-09)

Alle Abweichungen aus dem Reality-Check vom 2026-04-16 wurden behoben. Die Primary-Quellen reflektieren nun den Implementierungsstand:

| Bereich | Lösung |
|---|---|
| Versions-/Reifestand | ROADMAP, AUDIT, CHANGELOG auf v2.4.0 aktualisiert; 89 Tests (Gruppen A–X) |
| Capabilities | ARCHITECTURE.md-Flussdiagramme aktualisiert; alle `supports_*`-Flags korrekt |
| RAG-Fluss | ARCHITECTURE.md zeigt jetzt Mutex-Snapshot-Pattern und RAGContextAssembler |
| Audit-Status | AUDIT.md: 0 offene Security-Issues, 0 offene Functional-Issues |
| Security | Alle 4 SECURITY.md-Checkboxen auf `[x]`; GGUF-Magic, Digest-Gate, Policy-Hook |
| Concurrency | CRITICAL `data_race`-Findings behoben; Atomic-Counter; Retry-Logik |
| Server-Integration | `initFromServerConfig()` als sauberer Startup-Integrationspunkt hinzugefügt |
