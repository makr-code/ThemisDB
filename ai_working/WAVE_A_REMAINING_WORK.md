# Wave A — Remaining Work Checklist

Stand: 2026-05-19

## A1: Speculative Decoding (`src/llama_cpp/llama_cpp_plugin.cpp`)

- [ ] Echte `llama_get_logits()` Integration (statt synthetischer peaked-distribution)
- [ ] Draft-Rejection-Rate ≤ 40% validieren (SD-REAL-06..08)
- [ ] Latenz-Benchmark: ≥ 2.0× Speedup für 512-Token-Generierungen (`benchmarks/bench_llm_inference.cpp`)
- [ ] Fallback-Pfad dokumentieren (STUB/SIMULATION NOTE ergänzen)
- [ ] Golden-Output-Determinismus Test (greedy vs. speculative)

## A2: DPR Vectorizer (`src/rag/dpr_vectorizer.cpp`)

- [ ] Hash-basiertes Embedding durch echte ONNX Runtime Inference ersetzen (`Ort::Session::Run()`)
- [ ] Passage-Encoder Batch-Parallelisierung vervollständigen (Ziel: ≥ 100 docs/sec bei batch_size=32)
- [ ] Query-Latenz ≤ 150 ms validieren (DPR-08..10)
- [ ] MRR@10 Benchmark mit Ziel ≥ +15% vs. BM25-Baseline (DPR-07)
- [ ] STUB/SIMULATION NOTE am Übergangspfad ergänzen

## A3: Fairness Detector (`src/rag/fairness_detector.cpp`)

- [ ] Term-counting-Fallback durch echte embedding-basierte Bias-Projektion ersetzen
- [ ] Bias-Score-Korrelation ≥ 0.70 mit Human-Ratings validieren (FAIR-06..08)
- [ ] Overhead ≤ 5 ms/Dokument messen und nachweisen
- [ ] Integration in `RAGJudge::evaluateRelevance()` vervollständigen
- [ ] Intersektionalen Bias-Pfad (gender × ethnicity) vollständig implementieren
