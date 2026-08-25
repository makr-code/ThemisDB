# Plan: Gemma Preflight Guard fuer Llama-Benchmark

## Ziel
- Frueher, klarer Abbruch bei bekanntem Gemma-Artefakt-Fehlerbild (token_embd-Shape-Mismatch), bevor der Loader Layer-Retry-Logs erzeugt.

## Betroffene Dateien
- benchmarks/bench_llama_cpp_inference.cpp
- src/llm/gguf_loader.cpp

## Umsetzung
- In `bench_llama_cpp_inference.cpp` einen Preflight-Check vor `loadModel()` einfuegen.
- GGUF parsen und bei Gemma folgende Checks durchfuehren:
  - `token_embd.weight` vorhanden und valide Shape.
  - Token-Anzahl aus `tokenizer.ggml.tokens` (Array-Laenge) gegen Vokab-Dimension von `token_embd.weight` vergleichen.
- Bei Mismatch: `SkipWithError` mit kurzer Ursache + konkreter Handlungsempfehlung.
- Bei GGUF-Parsefehler (z. B. inkompatibles Quant-Format): ebenfalls fail-fast mit kurzer, handlungsleitender Meldung.

## Akzeptanzkriterien
- Benchmark bricht bei bekannt inkompatiblen Gemma-Artefakten vor `loadModel()` ab.
- Keine Layer-Retry-Liste als primaere Fehlerursache im Benchmark-Kontext.
- Fehlertext nennt Ursache und naechsten Schritt (kompatibles Artefakt / Runtime-Update).
