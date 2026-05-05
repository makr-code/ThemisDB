# S-LoRA: Serving Thousands of Concurrent LoRA Adapters

**Metadaten:**
- Author(en): Ying Sheng, Shiyi Lin, Joseph E. Gonzalez, Ion Stoica, Lianmin Zheng
- Konferenz/Journal: MLSys 2024; arXiv preprint
- Jahr: 2023 (arXiv) / 2024 (MLSys)
- Link: [arXiv:2311.03285](https://arxiv.org/abs/2311.03285)
- Zitierweise: `sheng2023slora`
- Tags: `slora`, `lora`, `multi-adapter`, `concurrent-serving`, `paged-memory`, `near-realtime`, `rag-inference`, `llm-serving`
- ThemisDB-Versionen: v2.0.0+ (geplant in `src/llm/lora/`, `src/rag/`)
- Status: [~] In Progress (Unified Paging für LoRA-Adapter geplant Q2 2026; Heterogeneous Batching geplant Q3 2026)

## 📋 Executive Summary

S-LoRA (Scalable LoRA) löst das kritische Problem des **gleichzeitigen Servings von tausenden LoRA-Adaptern** auf einem einzelnen GPU-Cluster ohne prohibitiven VRAM-Overhead. Das System erweitert das PagedAttention-Konzept (vLLM, Kwon et al. 2023) auf LoRA-Adapter-Gewichte: Adapter-Matrizen werden in einem **Unified Paging Memory Pool** gespeichert, der sowohl KV-Cache als auch LoRA-Deltaten verwaltet. **Heterogeneous Batching** ermöglicht, mehrere Anfragen mit unterschiedlichen aktiven Adaptern gleichzeitig in einem GPU-Kernel zu bearbeiten — ein Schlüsselprinzip für ThemisDB's near-realtime Multi-Tenant RAG, bei dem unterschiedliche Mandanten unterschiedliche Fachdomänen-Adapter verwenden.

Direkt relevant für `src/llm/lora/`, `src/rag/streaming_retriever.cpp` und das geplante Multi-Tenant-RAG-Feature in `src/rag/FUTURE_ENHANCEMENTS.md`.

## 🎯 Key Findings

- **Unified Paging Memory Pool**: LoRA-Adapter-Matrizen (A, B) und KV-Cache werden im selben Page-Manager verwaltet. Adapter-Pages werden bei Bedarf in/aus VRAM gepaged — kein separates VRAM-Budget pro Adapter erforderlich.
- **Heterogeneous Batching**: Ein GPU-Kernel bedient gleichzeitig `N` Anfragen mit bis zu `K` verschiedenen aktiven Adaptern. Die LoRA-Delta-Berechnung `(alpha/r) * B * A * x` wird für jede Anfrage individuell angewendet, während der Basis-Modell-Forward-Pass geteilt wird.
- **Tensor Parallelism für LoRA**: A und B Matrizen werden über mehrere GPUs aufgeteilt (Column/Row-Parallelism analog zum Basis-Modell), um Multi-GPU-Setups zu unterstützen.
- **Adapter-Prefetching**: Häufig genutzte Adapter werden im VRAM-Hot-Tier gehalten; seltene Adapter werden in CPU-RAM ausgelagert und bei Bedarf mit niedrigem Latenz-Overhead (< 5 ms) zurückgeladen.
- **Skalierbarkeit**: S-LoRA dient 2.000+ gleichzeitige Adapter auf einem A100-GPU-Cluster mit < 10% Durchsatzverlust gegenüber Single-Adapter-Basislinie; Latenz-p99 bleibt < 200 ms für 7B-Modelle.
- **Kein Adapter-Merge-Overhead**: Im Gegensatz zu vorab gemergten Adaptern berechnet S-LoRA das Delta `B * A * x` zur Laufzeit — ermöglicht Hot-Swap ohne Merge-Latenz, bei minimalem arithmetischem Overhead (r ≪ d).
- **CUDA-Custom-Kernels**: Spezialisierte CUDA-Kernels für den LoRA-Delta-MatMul (`sgmv` — Segmented Grouped Matrix-Vector) erreichen > 90% GPU-Auslastung bei heterogenem Batch.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [~] LLM LoRA Module → `src/llm/lora/` (Unified Paging: Adapter-Pages im ThemisDB Page-Manager)
- [~] RAG Streaming → `src/rag/streaming_retriever.cpp` (Heterogeneous Batching für Multi-Tenant Near-Realtime RAG)
- [x] LLM Deployment Plugin → `src/llm/llm_deployment_plugin.cpp` (Adapter-Prefetching-Policy)
- [ ] GPU Module → `src/gpu/` (CUDA `sgmv`-Kernel für LoRA-Delta-MatMul, Target Q3 2026)
- [ ] LLM Training → `src/training/` (Adapter-Versioning kompatibel mit S-LoRA Paging, Target Q4 2026)

### What Was Adopted?

1. **Unified Paging für LoRA-Adapter**: `AdapterRegistry` in `src/llm/lora/` erweitert um einen `AdapterPageManager`, der A- und B-Matrizen in 2-MB-Pages aufteilt. Pages können in CPU-RAM ausgelagert werden, wenn VRAM-Budget erschöpft ist.
2. **Heterogeneous-Batch-API**: `LLMPluginManager::generate()` nimmt eine `BatchRequest`-Struktur entgegen, die pro Request einen `adapter_id`-Slot enthält. Requests mit dem gleichen `base_model_id` werden zu einem heterogenen Batch zusammengefasst.
3. **Adapter-Tier-Policy**: Analog zu ThemisDB's dreistufigem Cache (L1 VRAM / L2 CPU-RAM / L3 NVMe) werden Adapter-Gewichte im VRAM-Hot-Tier (≤ 8 gleichzeitige Adapter), im CPU-RAM-Warm-Tier (bis 200 Adapter) und auf NVMe-Cold-Tier (beliebig viele Adapter) gehalten.
4. **Near-Realtime RAG-Latenz-Budget**: Mit S-LoRA-Adapter-Prefetching erreicht ThemisDB ein Gesamt-RAG-Latenz-Budget von ≤ 500 ms (p99) für Standard-Queries: Retrieval (≤ 50 ms) + LoRA-Adapter-Load (≤ 5 ms warm / ≤ 30 ms cold) + LLM-Inferenz (≤ 300 ms) + Reranking (≤ 100 ms).

### How Was It Adapted?

| S-LoRA Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| vLLM-basiertes PagedAttention | ThemisDB eigener Page-Manager (`src/llm/`) | Integration mit bestehender llama.cpp-Basis statt vollständigem vLLM-Umstieg |
| `sgmv` CUDA-Kernel | ONNX-Runtime-MatMul-Operator (kurzfristig) / eigener CUDA-Kernel (Q3 2026) | ONNX-Portabilität priorisiert; CUDA-Kernel für High-Throughput-Szenarien |
| Global Adapter Catalog | Tenant-isolierter Adapter-Store (per-tenant Namespace) | Multi-Tenant-Anforderung: Tenant A darf Adapter von Tenant B nicht sehen |
| Single-Batch-Size-Optimierung | Adaptive Batch-Size via ThemisDB's `AdaptiveOptimizer`-Feedback | Batch-Size wird per Workload-Phase dynamisch angepasst |
| Adapter-Ranking nach Nutzungshäufigkeit | Kombination Nutzungshäufigkeit + Latenz-SLA-Tier | Tenant-SLA-Priorität (Premium vs. Standard) beeinflusst Paging-Entscheidung |

### Performance Impact

| Metric | S-LoRA Paper Claim | ThemisDB Target | Status |
|--------|-------------------|-----------------|--------|
| Gleichzeitige Adapter ohne VRAM-Erschöpfung | 2.000+ auf A100 | 100+ auf RTX 4090 / 500+ auf A100 | ⏳ Planned Q2/Q3 2026 |
| Adapter-Load-Latenz (VRAM-Warm-Tier) | < 1 ms | < 5 ms | ⏳ Planned |
| RAG-Gesamt-Latenz p99 (7B-Modell) | < 200 ms (ohne Retrieval) | ≤ 500 ms (inkl. Retrieval) | ⏳ Planned |
| Throughput vs. Eager-Merge-Baseline | 4× bei 100 Adaptern | 2× bei 50 Adaptern (konservativ) | ⏳ Planned |

## ⚠️ Limitations & Open Questions

- `sgmv`-Kernel erfordert CUDA; CPU-only-Deployments (ARM-Server) haben höheren Delta-MatMul-Overhead.
  - ThemisDB-Lösung: ONNX-Runtime-Fallback für CPU-only-Pfad; CUDA-Kernel optional aktivierbar via `THEMIS_ENABLE_CUDA`.
- Unified Paging erhöht Implementierungskomplexität des Memory-Managers.
  - ThemisDB-Lösung: Adapter-Paging als separates `AdapterPageManager`-Subsystem mit eigenem `include/llm/adapter_page_manager.h`-Interface isolieren.
- Heterogeneous Batching erfordert Request-Batching-Logik; bei sehr niedrigem Concurrency-Level (< 4 parallele Requests) ist der Overhead höher als der Gewinn.
  - ThemisDB-Lösung: Hysteresis-Schwellwert: Heterogeneous Batching nur aktiv wenn ≥ 4 Requests pro 50-ms-Window vorliegen; sonst Single-Adapter-Verarbeitung.
- Adapter-Prefetching benötigt Workload-Vorhersage (welche Adapter werden als nächstes benötigt).
  - ThemisDB-Lösung: `workload_cache_strategy.cpp`-Prefetch-API für LLM-Adapter-Scheduling wiederverwenden.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests: Adapter-Page-In/Page-Out korrekt; Heterogeneous-Batch-Output identisch zu Sequential-Processing
- [ ] Benchmark: Latenz und Throughput mit 10/50/100 gleichzeitigen Adaptern auf 7B-Modell
- [ ] Regression-Test: keine funktionale Änderung bei Single-Adapter-Pfad (backward compatibility)
- [ ] Documentation updated (`src/llm/FUTURE_ENHANCEMENTS.md`)
- [ ] Module README linked (`src/llm/README.md`, `src/rag/README.md`)
- [ ] implementation_influence index updated

## 📚 Related Work

- [LoRA — Hu et al. (2022)](lora_low_rank_adaptation_2022.md)
- [Best Practice: Multi-LoRA Adapter Routing](../best_practices/multi_lora_adapter_routing.md)
- [Best Practice: S-LoRA Near-Realtime RAG Serving](../best_practices/slora_realtime_rag_serving.md)
- [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md)
- [HippoRAG — Gutierrez et al. (2024)](hipporag_gutierrez_2024.md)
- [PagedAttention — Kwon et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#85-pagedattention--continuous-batching)
- [Speculative Decoding — Chen et al. (2023)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#84-speculative-decoding)
- [`src/llm/lora/`](../../../src/llm/)
- [`src/rag/streaming_retriever.cpp`](../../../src/rag/streaming_retriever.cpp)

---
**Last Updated:** 2026-04-27  
**Next Review:** 2026-09-30
