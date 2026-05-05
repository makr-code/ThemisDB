# Multi-LoRA Adapter Routing & Domain Selection

**Metadaten:**
- Source: Hu et al. (2022) — LoRA (ICLR 2022); Chronopoulou et al. (2023) — AdapterSoup; Feng et al. (2024) — MoLoRA (Mixture-of-LoRA-Experts); HuggingFace PEFT Library; Industry practice (vLLM, Ollama multi-adapter deployment)
- URL: [LoRA arXiv](https://arxiv.org/abs/2106.09685) · [AdapterSoup arXiv](https://arxiv.org/abs/2302.07360) · [MoLoRA arXiv](https://arxiv.org/abs/2402.06878) · [vLLM Multi-LoRA](https://docs.vllm.ai/en/latest/features/lora.html)
- Tags: `lora`, `multi-lora`, `adapter-routing`, `domain-selection`, `mixture-of-experts`, `pattern-recognition`, `fine-tuning`, `peft`
- ThemisDB-Versionen: v1.3.0+ (LoRA implementiert); v2.1.0+ (Multi-Adapter-Routing — `LoRAPatternClassifier`, `MultiLoRAManager`)
- Status: [ ] Identified | [x] Partially Adopted | [ ] Fully Adopted  
  *(LoRA implementiert; Multi-Adapter-Routing geplant Q3 2027 — `src/analytics/ROADMAP.md` Phase 5)*

## 📋 Summary

Multi-LoRA-Adapter-Routing löst das Problem, dass ein einzelner LoRA-Adapter keine Universalkompetenz besitzt: verschiedene Domänen (legal, medical, financial; Pattern: fraud, anomaly, compliance) erfordern domänenspezifische Gewichte. Statt einem Adapter pro Request wird dynamisch der am besten passende Adapter aus einem Registry ausgewählt — via Embedding-Ähnlichkeit, Rule-based Routing oder Mixture-of-Experts-Gewichtung.

Für ThemisDB ist dieses Muster zentral für `LoRAPatternClassifier` (Analytics) und `KnowledgeGraphReasoner::applyLoRAScore()` (Graph): je nach Ereignis-Domäne oder Graph-Kontext wird ein spezifischer LoRA-Adapter gewählt, der domänenspezifische Mustererkennung liefert.

## 🎯 Core Principles

1. **Adapter-Registry mit Metadaten**: Jeder Adapter ist mit Domänen-Embedding, Beschreibung, Training-Datensatz-Statistiken und Qualitäts-Metriken registriert.
2. **Embedding-Ähnlichkeit für Routing**: Query/Event-Kontext wird zu Embedding transformiert; Cosine-Ähnlichkeit zu Adapter-Domänen-Embeddings bestimmt Top-1-Adapter (≤5 ms).
3. **Rule-based Fallback**: Explizite Routing-Regeln (z. B. `event.type == "financial_transaction" → adapter: "fraud_detection_v2"`) haben Vorrang vor Embedding-Routing.
4. **Adapter-Fusion (MoLoRA)**: Für Grenz-Domänen können mehrere Adapter gewichtet fusioniert werden: `Δ W = Σᵢ αᵢ · Bᵢ · Aᵢ` — ermöglicht Multi-Domain-Coverage ohne Qualitätsverlust.
5. **Confidence-Threshold + AutoML-Fallback**: Liegt Adapter-Konfidenz unter Schwellwert oder ist kein passender Adapter gefunden, wird AutoML-Klassifikator als Fallback aktiviert.
6. **Guard-Flag**: `THEMIS_ENABLE_LLM` kontrolliert LoRA-Pfad; alle Pfade haben deterministischen AutoML-Fallback.
7. **Thread-Safety**: Adapter-Inference ist stateless; mehrere parallele Inferences auf verschiedenen Adaptern sind möglich; globaler Mutex nur für Adapter-Registry-Mutations (Laden/Entladen).
8. **Security**: Adapter-Pfade werden durch `isLoRAPathTrusted()` validiert (`multi_lora_manager.cpp`); keine Ladevorgänge aus nicht vertrauenswürdigen Pfaden.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/llm/` — `MultiLoRAManager`: existierende Adapter-Registry; Hot-Swap via `hotSwap()`; `isLoRAPathTrusted()` Security-Check
- `src/analytics/` — `LoRAPatternClassifier`: Adapter-Routing für CEP-Event-Mustererkennung; Batch-Klassifikation
- `src/graph/` — `KnowledgeGraphReasoner::applyLoRAScore()`: Soft-Plausibility-Scoring für Inferenz-Kanten
- `src/rag/` — `LoRAEnhancedRetriever`: Domain-spezifisches Re-Ranking von Retrieval-Ergebnissen

### What Was Adopted?

**1. Adapter-Registry mit Domänen-Embeddings**
```cpp
// AdapterMetadata — Registrierungseintrag pro LoRA-Adapter
struct AdapterMetadata {
    std::string adapter_id;         // "fraud_detection_v2"
    std::string model_family;       // "llama-3.1-8b"
    std::string domain;             // "financial_fraud"
    std::string description;
    std::vector<float> domain_embedding;  // d=128, pre-computed
    float min_confidence_threshold; // 0.80
    std::string fallback_adapter;   // leer = AutoML-Fallback
    std::vector<std::string> trusted_paths;  // für isLoRAPathTrusted()
};

// MultiLoRAManager — Registry-API (existierend, erweiterbar)
class MultiLoRAManager {
public:
    void registerAdapter(AdapterMetadata meta, const std::string& weights_path);
    void unregisterAdapter(const std::string& adapter_id);

    // Neu: Embedding-basiertes Routing
    std::string selectAdapterForContext(const std::vector<float>& context_embedding);
    // Neu: Rule-based Routing
    std::string selectAdapterForEvent(const analytics::Event& event);
};
```

**2. Embedding-basiertes Adapter-Routing**
```cpp
// LoRAPatternClassifier::selectAdapter() — O(K) Cosine-Suche über K Adapter
std::string LoRAPatternClassifier::selectAdapter(
        const std::vector<float>& context_embedding) {

    float best_similarity = -1.0f;
    std::string best_adapter;

    // Read-Only — kein Lock nötig nach Registry-Initialisierung
    for (const auto& [id, meta] : adapter_registry_) {
        float sim = cosineSimilarity(context_embedding, meta.domain_embedding);
        if (sim > best_similarity) {
            best_similarity = sim;
            best_adapter = id;
        }
    }

    // Mindest-Ähnlichkeit nicht erreicht → AutoML-Fallback
    if (best_similarity < min_routing_similarity_) {
        return AUTOML_FALLBACK_ID;
    }
    return best_adapter;
}
```

**3. Mustererkennung-Klassifikation mit Batch-Support**
```cpp
// LoRAPatternClassifier::batchClassify() — max 4 parallele LoRA-Calls
std::vector<PatternResult> LoRAPatternClassifier::batchClassify(
        const std::vector<analytics::Event>& events) {

    // 1. Adapter-Selektion für gesamten Batch
    auto context_emb = embeddingProvider_->embed(buildBatchContext(events));
    std::string adapter_id = selectAdapter(context_emb);

    // 2. LoRA-Inference (oder AutoML-Fallback)
    if (adapter_id == AUTOML_FALLBACK_ID || !llmEnabled_) {
        return automlFallback_->classifyBatch(events);
    }

    // 3. Prompt aufbauen + LoRA-Adapter aufrufen
    auto prompt = buildClassificationPrompt(events);
    auto response = lora_manager_->generateWithAdapter(adapter_id, prompt,
                                                        GenerationConfig{.max_tokens=256});

    // 4. JSON-Antwort parsen: [{"label": "fraud_sequence", "confidence": 0.92}, ...]
    return parseClassificationResponse(response, events.size());
}
```

**4. EPL-Integration: `PATTERN CLASSIFIED_AS`**
```sql
-- LoRA-Klassifikation als CEP-Filterbedingung
CREATE RULE fraud_pattern_lora
AS SELECT COUNT(*) AS event_count, FIRST(user_id) AS user
FROM STREAM financial_events
WINDOW (TUMBLING 60s)
WHERE CLASSIFIED_AS('fraud_sequence', adapter='fraud_detection_v2', min_confidence=0.90)
  AND amount > 10000
PATTERN WITHIN 300s
ACTION alert(channel="fraud_ops");

-- Expertensystem-Bestätigung kombiniert mit LoRA
CREATE RULE compliance_combined
AS SELECT *
FROM STREAM audit_events
WHERE CLASSIFIED_AS('compliance_violation', min_confidence=0.85)
  AND EXPERT_SYSTEM_CONFIRMS('compliance_expert_rule', confidence>=0.85)
ACTION db_write(table="compliance_log");
```

**5. MoLoRA-Adapter-Fusion für Grenz-Domänen**
```cpp
// Adapter-Fusion: legal + german_language Adapter
struct FusionConfig {
    std::vector<std::pair<std::string, float>> adapters;
    // [{"legal_domain_v1", 0.7}, {"german_language_v2", 0.3}]
};

// MultiLoRAManager::generateFused() — gewichtete Delta-W-Superposition
// ΔW_fused = Σᵢ αᵢ · (Bᵢ · Aᵢ)
std::string MultiLoRAManager::generateFused(const FusionConfig& fusion,
                                             const std::string& prompt) {
    // Adapter-Deltas laden + gewichtet summieren
    Eigen::MatrixXf delta_W = Eigen::MatrixXf::Zero(d_out_, d_in_);
    for (const auto& [adapter_id, weight] : fusion.adapters) {
        const auto& adapter = registry_.at(adapter_id);
        delta_W += weight * (adapter.B * adapter.A);  // ΔW = B·A
    }
    // Temporäres fused-Adapter-Objekt + Inference
    return generateWithDeltaW(delta_W, prompt);
}
```

**6. Security: Trusted-Path-Validation vor Adapter-Load**
```cpp
// isLoRAPathTrusted() — bereits implementiert in multi_lora_manager.cpp
// Canonical-Path-Check verhindert Path-Traversal-Angriffe
bool MultiLoRAManager::isLoRAPathTrusted(const std::string& path) {
    auto canonical = std::filesystem::canonical(path);
    for (const auto& trusted : trusted_adapter_dirs_) {
        if (canonical.string().starts_with(trusted)) return true;
    }
    return false;
}
```

### Deviations & Rationale

| Best-Practice-Standard | ThemisDB Adaptation | Begründung |
|---|---|---|
| vLLM Batched Multi-LoRA | Eigene `batchClassify()` + Thread-Pool | Keine vLLM-Abhängigkeit; Integration mit CEP-Engine |
| Continuous Batching | Tumbling-Window-Batching (CEP) | CEP-Windows liefern natürliche Batch-Grenzen |
| Adapter-Hot-Swap sofort | `hotSwap()` < 50 ms | Production: Zero-Downtime-Adapter-Updates |
| Uniform confidence threshold | Per-Adapter `min_confidence_threshold` | Domänen haben unterschiedliche Qualitätsanforderungen |
| Gradient-basiertes Routing | Embedding-Cosine-Routing | Kein Online-Training für Routing; Embedding-Routing ist interpretierbar |

## ⚠️ Trade-offs & Limitations

- **Embedding-Routing-Kaltstart**: Neue Adapter ohne initialen Routing-Test können schlechte Trefferquote haben.
  - Mitigation: Adapter-Evaluation auf Validierungs-Datensatz vor Produktion; `adapter_quality_score` in Metadaten.
- **MoLoRA-Fusion-Qualität**: Gewichtete Fusion verschlechtert Performance wenn Adapter orthogonale Wissensdomänen repräsentieren.
  - Mitigation: Cosine-Ähnlichkeit zwischen Adapter-Gewichten als Kompatibilitäts-Score; Warnung bei geringer Ähnlichkeit.
- **Latenz unter Last**: Parallele LoRA-Inferences konkurrieren um GPU-VRAM.
  - Mitigation: Max 4 parallele Inferences (`batchClassify()` Thread-Pool-Limit); Priorisierung nach Adapter-ID.
- **AutoML-Fallback-Qualität**: AutoML hat geringere Domänen-Spezifität als trainierter LoRA-Adapter.
  - Mitigation: Fallback-Qualität wird in `LoRAPatternClassifier`-Metriken trackiert; Alert bei Fallback-Rate > 20%.

## 🔬 Validation

- [ ] Adapter-Routing: Cosine-Ähnlichkeit wählt korrekten Adapter für 5 Domänen-Testfälle
- [ ] Batch-Klassifikation: 64 Events ≤ 100 ms (inkl. Embedding + LoRA-Inference)
- [ ] AutoML-Fallback: korrekt aktiviert wenn `THEMIS_ENABLE_LLM=OFF`
- [ ] Security: `isLoRAPathTrusted()` blockiert Path-Traversal-Pfade
- [ ] MoLoRA-Fusion: Fused-Adapter-Output liegt zwischen beiden Einzel-Adapter-Outputs
- [ ] Module README verlinkt (`src/analytics/README.md`, `src/llm/README.md`)
- [ ] implementation_influence index aktualisiert

## 📚 Related

- [Paper: LoRA — Hu et al. (2022)](../papers/lora_low_rank_adaptation_2022.md)
- [Paper: TransE — Bordes et al. (2013)](../papers/bordes_transe_2013.md) — KGE als symbolisches Scoring-Komplement
- [Best Practice: OWL-lite Ontology Constraints](owl_lite_ontology_constraints.md)
- [Best Practice: RETE Forward-Chaining Rule Engine](rete_forward_chaining_rule_engine.md)
- [`src/analytics/FUTURE_ENHANCEMENTS.md`](../../../src/analytics/FUTURE_ENHANCEMENTS.md) — AI/ML + LoRA Pattern Classification
- [`src/analytics/ROADMAP.md`](../../../src/analytics/ROADMAP.md) — Phase 5
- [`src/llm/multi_lora_manager.cpp`](../../../src/llm/multi_lora_manager.cpp) — Existierende Implementierung
- [vLLM Multi-LoRA Serving](https://docs.vllm.ai/en/latest/features/lora.html) — Production-Reference
- [HuggingFace PEFT](https://github.com/huggingface/peft) — Library-Reference

## 🔬 Weiterführende Quellen

| Quelle | Relevanz für ThemisDB |
|--------|----------------------|
| Chronopoulou et al. (2023) — AdapterSoup | Adapter-Interpolation ohne Retraining |
| Feng et al. (2024) — MoLoRA | Mixture-of-LoRA-Experts; gewichtete Fusion |
| Dettmers et al. (2023) — QLoRA | 4-bit Quantisierung für LoRA-Basismodell |
| Zhang et al. (2023) — LLaMA-Adapter | Visually-conditioned Adapter-Routing |
| Wang et al. (2023) — AdaMix | Mixture-of-Adapter-Experts ohne MoE-Training |
| Zhao et al. (2024) — LoRA+ | Asymmetrische Lernraten für A/B-Matrizen |

---
**Last Updated:** 2026-04-22
