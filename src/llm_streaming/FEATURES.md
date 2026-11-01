---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# LLM-Streaming in ThemisDB

## Aktueller, belegter Umfang

Der Pfad `src/llm_streaming/` ist dokumentationsbasiert. Belegt ist ein
Hilfs-API in
`include/llm/streaming_handler.h` mit Implementierung in
`src/llm/streaming_handler.cpp`:

- `formatSseEvent` erzeugt einen SSE-`data:`-Block mit JSON-Feldern `id`,
  `token`, `index` und `done`; Stringwerte werden JSON-escaped.
- `formatDoneEvent` erzeugt `data: [DONE]\n\n`.
- `formatChunkedData` formatiert einen einzelnen HTTP-Chunk.
- `makeStreamCallback` weist Token-Aufrufen atomare Indizes zu und ruft einen
  vom Aufrufer bereitgestellten Sink mit SSE-formatierten Strings auf.

Zusätzlich existiert eine HTTP/SSE-Route `GET /api/v1/llm/stream` in
`src/server/llm_api_handler.cpp`. Sie ruft `EmbeddedLLM::generateStreamingSSE`
auf, sammelt SSE-Ereignisse in einem String und liefert nach Abschluss der
Generierung einen HTTP-`string_body`-Response. Das ist SSE-formatierte Ausgabe,
aber keine inkrementelle Übertragung zum Client; die Route nutzt den
`StreamingHandler`-Helper nicht. Die atomare Indexvergabe des Helpers stellt
bei parallelen Aufrufen keine Reihenfolge der Sink-Zustellung sicher.

Auch `POST /v1/chat/completions` unterstützt `stream: true`: Der Handler
wandelt Callback-Ausgaben in OpenAI-kompatible SSE-Chunks um, sammelt sie
inklusive finalem Chunk und `[DONE]` im Speicher und liefert anschließend einen
vollständigen `string_body`-Response zurück. Auch hier erfolgt kein
inkrementelles Flushen an den Client.

## Validierung und Grenzen

`tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`,
`tests/integration/test_llm_streaming_soak.cpp` und
`benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp` implementieren
eigene In-Process-Stubs. Sie rufen den `StreamingHandler` nicht auf. Die dort
genannten Last- oder Performance-Schwellen sind daher keine gemessenen
Eigenschaften einer Streaming-Runtime.

`tests/llm/test_llm_aql_explain_stream_api.cpp` prüft unauthentifizierte
Route-/Auth-Fälle, aber keine erfolgreiche authentifizierte Generierung oder
SSE-Zustellung.

Der Route-Code verarbeitet `prompt`, `request_id` und `max_tokens` als
Query-Parameter; `max_tokens` ist standardmäßig 512 und akzeptiert Werte bis
4096. Es wurden keine speziellen Streaming-Konfigurationsschlüssel,
Disconnect-Abbruch, Backpressure-Steuerung oder Streaming-Metriken gefunden.
Lebenszyklus und Wiederaufnahme sind weiterhin nicht als Runtime-Vertrag
belegt.

## Geplante Klärung

Vor Aussagen über inkrementelle Übertragung oder Produktionsreife müssen die
geforderte Transportsemantik sowie End-to-End-Tests für den vorhandenen
gepufferten HTTP/SSE-Pfad nachgewiesen und dokumentiert werden.

## Quellen

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- `src/llm/embedded_llm.cpp`
- `src/server/llm_api_handler.cpp`
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
- `tests/integration/test_llm_streaming_soak.cpp`
- `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`
