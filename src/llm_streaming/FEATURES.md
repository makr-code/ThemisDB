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

Der Pfad `src/llm_streaming/` ist dokumentationsbasiert und enthält keine
Streaming-Runtime. Belegt ist ein Hilfs-API in
`include/llm/streaming_handler.h` mit Implementierung in
`src/llm/streaming_handler.cpp`:

- `formatSseEvent` erzeugt einen SSE-`data:`-Block mit JSON-Feldern `id`,
  `token`, `index` und `done`; Stringwerte werden JSON-escaped.
- `formatDoneEvent` erzeugt `data: [DONE]\n\n`.
- `formatChunkedData` formatiert einen einzelnen HTTP-Chunk.
- `makeStreamCallback` weist Token-Aufrufen atomare Indizes zu und ruft einen
  vom Aufrufer bereitgestellten Sink mit SSE-formatierten Strings auf.

Das belegt weder eine Serverroute noch die Integration mit LLM-Inferenz,
gRPC, HTTP-Verbindungen oder einem Produktions-Backpressure-Mechanismus.
Insbesondere stellt die atomare Indexvergabe bei parallelen Aufrufen keine
Reihenfolge der Sink-Zustellung sicher.

## Validierung und Grenzen

`tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`,
`tests/integration/test_llm_streaming_soak.cpp` und
`benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp` implementieren
eigene In-Process-Stubs. Sie rufen den `StreamingHandler` nicht auf. Die dort
genannten Last- oder Performance-Schwellen sind daher keine gemessenen
Eigenschaften einer Streaming-Runtime.

Lebenszyklus, Abbruch, Disconnect-Behandlung, End-to-End-Reihenfolge,
Backpressure, Telemetrie und Wiederaufnahme sind derzeit Design- bzw.
Validierungsthemen; ihre Runtime-Umsetzung ist mit den hier geprüften Quellen
nicht belegt.

## Geplante Klärung

Vor Aussagen über eine produktive Funktion müssen der tatsächliche Aufrufer,
die Transportintegration, Konfiguration und End-to-End-Tests in einem
kanonischen Runtime-Pfad nachgewiesen und dokumentiert werden.

## Quellen

- `include/llm/streaming_handler.h`
- `src/llm/streaming_handler.cpp`
- `tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp`
- `tests/integration/test_llm_streaming_soak.cpp`
- `benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`
