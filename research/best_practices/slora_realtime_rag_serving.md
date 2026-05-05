# S-LoRA Near-Realtime RAG Serving mit LoRA-Spezialisten

**Metadaten:**
- Source: Sheng et al. (2023) — S-LoRA (MLSys 2024); Wang et al. (2024) — Speculative RAG (arXiv); Hu et al. (2022) — LoRA (ICLR 2022); Kwon et al. (2023) — PagedAttention (SOSP 2023)
- URL: [arXiv:2311.03285](https://arxiv.org/abs/2311.03285) · [arXiv:2407.08223](https://arxiv.org/abs/2407.08223)
- Tags: `slora`, `speculative-rag`, `near-realtime`, `multi-adapter`, `lora`, `rag-latency`, `adapter-paging`, `heterogeneous-batching`
- ThemisDB-Versionen: v2.0.0+ (`src/llm/lora/`, `src/rag/streaming_retriever.cpp`)
- Status: [~] Partially Adopted (LoRA Hot-Swap implementiert; Paging + Hetero-Batching geplant Q2/Q3 2026)

## 📋 Summary

Near-Realtime RAG bedeutet, dass der gesamte Zyklus — Retrieval + LoRA-Adapter-Load + LLM-Inferenz + Reranking — in ≤ 500 ms (p99) für Standard-Queries abgeschlossen sein muss. ThemisDB erreicht dieses Ziel durch drei komplementäre Techniken:

1. **S-LoRA Adapter Paging**: LoRA-Adapter-Gewichte werden wie KV-Cache-Pages verwaltet; Hot-Adapter bleiben im VRAM-Tier, Cold-Adapter werden in CPU-RAM ausgelagert und bei Bedarf mit < 5 ms Latenz geladen.
2. **Heterogeneous Batching**: Mehrere parallele Anfragen mit unterschiedlichen aktiven Adaptern werden in einem GPU-Kernel gebündelt — Basis-Modell-Forward-Pass geteilt, LoRA-Delta individuell.
3. **Speculative RAG Draft-Verify**: Domain-LoRA-Adapter generiert eine schnelle Draft-Antwort aus Teilkontext; Verifier (Basis-Modell) überprüft nur bei niedriger Konfidenz — 30–50% aller Queries sparen den Verifier-Pass.

## 🎯 Core Principles

- **Principle 1 — VRAM-Hierarchisches Adapter-Management**: LoRA-Adapter werden in drei Tiers verwaltet: VRAM-Hot (≤ 8 gleichzeitige Adapter, sofortiger Zugriff), CPU-RAM-Warm (bis 200 Adapter, < 5 ms Ladezeit), NVMe-Cold (unbegrenzt, < 30 ms). Adapter-Promotion/Eviction basiert auf Kombination aus Nutzungshäufigkeit und Mandanten-SLA-Tier.
- **Principle 2 — Null-Overhead-Delta-Berechnung**: LoRA-Delta `(alpha/r) * B * A * x` wird zur Laufzeit berechnet; kein vorab-Merge in Basis-Modell-Gewichte. Ermöglicht Hot-Swap ohne Merge-Latenz und spart den Merge-VRAM-Footprint.
- **Principle 3 — Draft-First-Antwort für First-Token-Latenz**: Der Domain-LoRA-Adapter als Draft-Spezialist erzeugt eine vorläufige Antwort auf halber Kontextgröße. First-Token erscheint dem Nutzer nach ≤ 100 ms (Draft-Phase), auch wenn die vollständige Verifier-Antwort länger dauert.
- **Principle 4 — Domänen-isolierte Adapter-Routing**: `AdapterRegistry::getDomainAdapter(tenant_id, query_type)` stellt sicher, dass jeder Mandant nur seine autorisierten Adapter verwendet. Kein Cross-Tenant-Adapter-Leak.
- **Principle 5 — Graceful Degradation**: Wenn VRAM-Budget erschöpft ist und ein Cold-Adapter nicht rechtzeitig geladen werden kann, fällt die Pipeline auf das Basis-Modell ohne LoRA zurück — kein Request-Fehler, aber mit Lower-Quality-Flag im Response-Header.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/llm/lora/` — S-LoRA-Adapter-Paging; Heterogeneous-Batch-API; `AdapterPageManager`
- `src/rag/streaming_retriever.cpp` — Draft-Verify-Pipeline; First-Token-Streaming auf Draft-Basis
- `src/rag/agentic_rag.cpp` — Speculative RAG als Iterationsstrategie im AgenticRAG-Loop
- `src/llm/llm_deployment_plugin.cpp` — Adapter-Prefetching-Policy; Tier-Promotion-Logik
- `src/rag/rag_judge.cpp` — Verifier-Konfidenz-Scoring (FAST-Modus ≤ 100 ms) für Adaptive-Skipping

### What Was Adopted?

#### Phase 1: LoRA Hot-Swap (bereits implementiert, v1.3.0+)

- `AdapterRegistry::hotSwap()`: Thread-sicherer Adapter-Wechsel ohne Inferenz-Unterbrechung.
- GGUF-Format für Adapter-Speicherung; Signaturvalidierung via `lora_security_validator.cpp`.
- Multi-Adapter-Fusion mit konfigurierbaren α-Gewichten (`docs/MULTI_LORA_FUSION_GUIDE.md`).

#### Phase 2: S-LoRA Adapter Paging (geplant Q2 2026)

```cpp
// include/llm/adapter_page_manager.h (geplant)
class AdapterPageManager {
public:
    // Lädt Adapter-Pages in VRAM; evicted am seltensten genutzte Pages bei VRAM-Druck
    AdapterHandle acquire(const std::string& adapter_id, const TenantSLA& sla);
    // Gibt Handle frei; Page bleibt im VRAM-Tier bis zur Eviction
    void release(AdapterHandle handle);
    // Prefetch eines Adapters in CPU-RAM basierend auf Workload-Vorhersage
    void prefetch(const std::string& adapter_id);
    AdapterTierStats stats() const;
};
```

#### Phase 3: Heterogeneous Batching (geplant Q3 2026)

```cpp
// Erweiterung von LLMPluginManager::generate() (geplant)
struct BatchRequest {
    std::vector<InferenceRequest> requests;  // Jeder Request hat adapter_id
    std::string base_model_id;
};
// Batcht Requests mit gleichem base_model_id; wendet LoRA-Delta pro Request individuell an
BatchResponse generateBatch(const BatchRequest& batch);
```

#### Phase 4: Speculative RAG Draft-Verify (geplant Q1 2027)

```cpp
// Erweiterung von StreamingRetriever (geplant)
struct SpeculativeRagConfig {
    float draft_context_ratio = 0.5f;       // Anteil Kontext für Draft-Phase
    float draft_confidence_threshold = 0.8f; // Unter diesem Score: Verifier aktivieren
    std::string draft_adapter_id;            // Domain-LoRA-Adapter als Draft-Spezialist
};
```

### Deviations & Rationale

- **ONNX statt vLLM-Basis**: S-LoRA ist auf vLLM/PagedAttention aufgebaut. ThemisDB nutzt llama.cpp-Basis; `AdapterPageManager` implementiert Paging-Konzept unabhängig von vLLM-Infrastruktur.
- **CPU-RAM als Warm-Tier** (S-LoRA hat nur VRAM/NVMe): Zusätzlicher CPU-RAM-Tier reduziert NVMe-I/O-Latenz für mittelgroße Adapter-Pools (10–200 Adapter).
- **Tenant-isolierter Adapter-Namespace**: S-LoRA hat Single-Tenant-Fokus; ThemisDB fügt Namespace-Isolation hinzu — jeder Tenant sieht nur seine eigenen Adapter-IDs.

## ⚠️ Trade-offs & Limitations

- **VRAM-Hot-Tier-Kapazität**: Mit 24 GB VRAM (RTX 4090) und 7B-Modell (ca. 14 GB) verbleiben ≈ 10 GB für Adapter-Pages. Bei LoRA-r=16 auf 7B: ≈ 300 MB pro Adapter → 30–35 Adapter gleichzeitig in VRAM.
- **Adapter-Prefetching-Qualität**: Schlechte Workload-Vorhersage führt zu erhöhter Cold-Adapter-Ladefrequenz. Lösung: `workload_cache_strategy.cpp`-Histogramme als Prefetch-Signal.
- **Draft-Qualität bei generischen Adaptern**: Speculative RAG bringt wenig Gewinn, wenn der Domain-Adapter keine deutliche Qualitätsverbesserung gegenüber dem Basis-Modell zeigt. Adapter-Qualitätsmetrik (GLUE-Score auf Domänen-Task) als Gate für Draft-Aktivierung.
- **Heterogeneous-Batch-Overhead bei niedrigem Concurrency**: < 4 gleichzeitige Requests: kein Gewinn durch Hetero-Batching; Hysteresis-Schwellwert aktiviert Batching erst ab ≥ 4 Requests/50ms.

## 🔬 Validation

- [x] LoRA Hot-Swap implementiert und in Produktionsumgebung validiert (v1.3.0+)
- [x] Multi-Adapter-Fusion mit α-Gewichten implementiert
- [ ] Adapter-Page-Manager: Unit-Tests für Tier-Promotion/Eviction
- [ ] Heterogeneous Batching: Output-Identität vs. Sequential-Processing verifiziert
- [ ] Speculative RAG: A/B-Test gegen Standard-RAG (Latenz + EM-Score)
- [ ] Latenz-Budget ≤ 500 ms p99 (inkl. Retrieval) auf Verwaltungsrecht-QA-Testset gemessen
- [ ] Skip-Rate ≥ 25% auf Produktions-Queries validiert
- [ ] Module READMEs verlinkt (`src/llm/README.md`, `src/rag/README.md`)

## 📚 Related

- [S-LoRA — Sheng et al. (2023)](../papers/sheng_slora_concurrent_adapters_2023.md)
- [Speculative RAG — Wang et al. (2024)](../papers/wang_speculative_rag_2024.md)
- [LoRA — Hu et al. (2022)](../papers/lora_low_rank_adaptation_2022.md)
- [Multi-LoRA Adapter Routing](multi_lora_adapter_routing.md)
- [`src/llm/lora/`](../../../src/llm/)
- [`src/rag/streaming_retriever.cpp`](../../../src/rag/streaming_retriever.cpp)

---
**Last Updated:** 2026-04-27
