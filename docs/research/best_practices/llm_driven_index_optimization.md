# LLM-Driven Index Optimization: AI/ML Hook Pattern for Index Maintenance

**Metadaten:**
- Source: Zhou et al. (2022) — AI Meets Database (AI4DB Survey, SIGMOD); Lim et al. (2022) — ISUM; RocksDB Production Deployment Studies; ThemisDB Engineering
- URL: [AI4DB arXiv:2209.05237](https://arxiv.org/abs/2209.05237) · [ISUM arXiv:2206.09219](https://arxiv.org/abs/2206.09219) · [RocksDB Tuning Guide](https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide)
- Tags: `index-optimization`, `ai-advisor`, `llm-database`, `index-maintenance`, `rocksdb`, `tiered-storage`, `ai4db`, `storage-layout`
- ThemisDB-Versionen: v1.9.0+ (`src/storage/index_analyzer.cpp`, `src/storage/storage_layout_advisor.cpp`)
- Status: [x] Fully Adopted

## 📋 Summary

The LLM-Driven Index Optimization pattern establishes how an AI/ML model (including an LLM) can intervene in ThemisDB's internal index maintenance decisions without replacing the rule-based safety net. The rule engine always produces a baseline recommendation (NONE / UPDATE_STATS / REORGANIZE / PARTIAL_REBUILD / FULL_REBUILD); an optional `IIndexAnalysisAdvisor` implementation receives the full analysis report and may return an override. The AI layer sees the same metrics the rule engine uses — RocksDB fragmentation, orphan entries, statistics staleness, storage tier — plus the preliminary rule-based recommendation as a prior. This follows the AI4DB principle: AI augments, it does not replace, deterministic safeguards.

## 🎯 Core Principles

1. **Non-invasive augmentation**: The AI advisor is opt-in (`ai_advisor.enabled: false` by default) and can only override — it cannot prevent a safety-critical FULL_REBUILD if the rule engine mandates it.
2. **Full-context input**: The advisor receives the complete `IndexAnalysisReport` (fragmentation %, orphan entries, size_bytes, stats_age_hours, storage tier, and the rule-based `IndexRecommendation`) so the model can learn from the rule engine's prior.
3. **Exception safety**: Any exception in `advise()` is caught and logged; the rule-based recommendation is always preserved as the fallback.
4. **Tier-aware thresholds**: Maintenance triggers are intentionally looser for warm/cold tiers (more fragmentation tolerated) because rebuild cost is amortized over longer access intervals.
5. **Cron-scheduled off-peak analysis**: Analysis runs on a configurable cron schedule (default `0 2 * * *`) to avoid compounding write amplification during peak load.
6. **Decision auditability**: `StorageLayoutAdvisor::emitDecisionRecord()` submits structured `LAYOUT_RECOMMENDATION` records to a `DecisionRecordYamlProcessor` for LLM post-processing, governance logging, and GDPR approval gating.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/storage/index_analyzer.cpp` — `IndexAnalyzer`: rule-based analysis + `applyAdvisor()` dispatch to `IIndexAnalysisAdvisor`
- `include/storage/index_analyzer.h` — `IIndexAnalysisAdvisor` interface; `IndexAnalysisReport`; `IndexRecommendation` enum; `TierThresholds`
- `src/storage/storage_layout_advisor.cpp` — `StorageLayoutAdvisor`: heuristic layout recommendation + `emitDecisionRecord()` for LLM post-processing
- `config/index_analyze.yaml` — YAML configuration: `ai_advisor.enabled`, `ai_advisor.model`, per-tier thresholds, per-index overrides

### What Was Adopted?

**1. `IIndexAnalysisAdvisor` interface — AI hook**

```cpp
// include/storage/index_analyzer.h
class IIndexAnalysisAdvisor {
public:
    virtual ~IIndexAnalysisAdvisor() = default;

    // Receives the preliminary rule-based report.
    // Returns override recommendation + reason, or nullopt to keep rule-based result.
    // MUST be thread-safe (called from background scheduler thread).
    virtual std::optional<std::pair<IndexRecommendation, std::string>>
    advise(const IndexAnalysisReport& report) = 0;
};
```

Register via `IndexAnalyzer::setAdvisor(shared_ptr<IIndexAnalysisAdvisor>)`.
Enable via `ai_advisor.enabled: true` in `config/index_analyze.yaml`.

**2. `IndexAnalysisReport` — full-context feature set**

| Field | Type | AI Input Value |
|-------|------|----------------|
| `index_name` | string | Index identifier |
| `tier` | StorageTierLevel | HOT / WARM / COLD |
| `fragmentation_pct` | double | (total_sst - live_sst) / total_sst × 100 + l0_files × 2.0 |
| `total_entries` | uint64 | `rocksdb.estimate-num-keys` |
| `orphan_entries` | uint64 | pending_compact_bytes / 1 MB × 1000 (heuristic) |
| `size_bytes` | uint64 | `rocksdb.total-sst-files-size` |
| `stats_age_hours` | uint64 | Hours since last statistics update |
| `stats_stale` | bool | `stats_age_hours >= threshold` |
| `recommendation` | IndexRecommendation | Rule-based prior for the AI model |
| `reason` | string | Human-readable rule explanation |

**3. Tier-aware thresholds (calibrated against production studies)**

| Tier | Reorganize | Partial Rebuild | Full Rebuild | Stats Stale |
|------|-----------|----------------|-------------|-------------|
| HOT (NVMe) | 10 % | 20 % | 35 % | 1 h |
| WARM (SATA SSD) | 18 % | 32 % | 50 % | 6 h |
| COLD (object storage) | 30 % | 50 % | 70 % | 24 h |

**4. `applyAdvisor()` — safe dispatch pattern**

```cpp
// src/storage/index_analyzer.cpp
void IndexAnalyzer::applyAdvisor(IndexAnalysisReport& report) {
    std::shared_ptr<IIndexAnalysisAdvisor> advisor;
    bool ai_enabled;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        advisor    = advisor_;
        ai_enabled = config_.ai_advisor_enabled;
    }

    if (!ai_enabled || !advisor) return;

    try {
        auto override_result = advisor->advise(report);
        if (override_result) {
            report.ai_recommendation = override_result->first;
            report.ai_reason         = override_result->second;
            // Callers SHOULD prefer ai_recommendation when set.
        }
    } catch (const std::exception& ex) {
        THEMIS_ERROR("IndexAnalyzer: AI advisor threw; rule-based recommendation preserved: {}", ex.what());
        // report.recommendation is unchanged — rule-based result always survives.
    }
}
```

**5. `StorageLayoutAdvisor` decision record emission**

```cpp
// src/storage/storage_layout_advisor.cpp
void StorageLayoutAdvisor::emitDecisionRecord(const LayoutRecommendation& rec) const {
    if (!dr_processor_) return;
    themis::llm::DecisionRecord dr;
    dr.decision_type = "LAYOUT_RECOMMENDATION";
    dr.record_id     = "layout-" + rec.collection_name;
    dr.parameters["recommended_layout"]    = layoutName(rec.recommended_layout);
    dr.parameters["confidence"]            = std::to_string(rec.confidence);
    dr.parameters["gdpr_approval_required"] = rec.gdpr_approval_required ? "true" : "false";
    dr.parameters["rationale"]             = rec.rationale;
    dr_processor_->submit(std::move(dr));
    // LLM backend can annotate, escalate, or learn from this decision record.
}
```

**6. YAML configuration**

```yaml
# config/index_analyze.yaml
index_analyze:
  enabled: true
  cron_expression: "0 2 * * *"   # Daily at 02:00
  thresholds:
    hot:   { reorganize_pct: 10, partial_rebuild_pct: 20, full_rebuild_pct: 35, stats_stale_hours: 1 }
    warm:  { reorganize_pct: 18, partial_rebuild_pct: 32, full_rebuild_pct: 50, stats_stale_hours: 6 }
    cold:  { reorganize_pct: 30, partial_rebuild_pct: 50, full_rebuild_pct: 70, stats_stale_hours: 24 }
  ai_advisor:
    enabled: false          # opt-in; set true + provide IIndexAnalysisAdvisor impl
    model: ""               # forwarded to registered advisor as metadata
  indices:
    - name: primary
      tier: hot
    - name: vectors
      tier: warm
      thresholds:           # per-index override
        reorganize_pct: 25
```

### Deviations & Rationale

| Best-Practice Standard | ThemisDB Adaptation | Rationale |
|---|---|---|
| AI as sole decision-maker | AI as optional override; rule-based always runs | Production safety; incorrect FULL_REBUILD causes minutes of write stall |
| Online/continuous learning loop | Scheduled cron + opt-in AI override | Avoids compounding write amplification during peak; deterministic scheduling |
| Centralized AI advisor service | In-process `IIndexAnalysisAdvisor` interface | Zero network latency; ThemisDB is an embedded database engine |
| Universal thresholds | Per-tier thresholds (hot/warm/cold) | Maintenance cost per tier differs by orders of magnitude |
| AI model manages GDPR | `gdpr_approval_required` flag gates migration; DBA approval required | Compliance: AI can flag but cannot bypass GDPR data migration controls |

## ⚠️ Trade-offs & Limitations

- The AI advisor cannot observe query access patterns or frequency; its input is limited to RocksDB storage metrics.
  - Mitigation: Planned enrichment of `IndexAnalysisReport` with `src/observability/` query-frequency data (Target: v2.0.0).
- Statistics staleness is approximated (`kPlaceholderStatsAgeHours = 2`) until a dedicated metadata column family for stats-update timestamps is added.
  - Mitigation: Constant is documented; accurate tracking via metadata CF is on the backlog.
- No concrete LLM-backed `IIndexAnalysisAdvisor` implementation is shipped; the interface is ready but the model-backed impl is pending.
  - Mitigation: `LLMIndexAdvisor` planned for v2.0.0 using `ILLMProvider` + structured prompt from `IndexAnalysisReport`.

## 🔬 Validation

- [x] Code reviewed against AI4DB survey interface design
- [x] Unit tests written (IA-01..IA-15 in `tests/test_index_analyzer.cpp`)
- [x] Exception isolation validated (IA-08..IA-09: advisor exception preserves rule result)
- [ ] Benchmark executed (AI advisor decision quality vs. rule-based baseline)
- [x] Module documented (`src/storage/FUTURE_ENHANCEMENTS.md`; `config/index_analyze.yaml`)
- [ ] Module README linked
- [x] implementation_influence index updated

## 📚 Related

- [Paper: LLM Index Advisor Integration](../papers/llm_index_advisor_integrated_2024.md)
- [Paper: AI4DB Survey — Zhou et al. (2022)](../papers/zhou_ai4db_survey_2022.md)
- [Paper: Bao Learned Query Optimization — Marcus et al. (2021)](../papers/marcus_bao_learned_query_opt_2021.md)
- [`src/storage/index_analyzer.cpp`](../../../src/storage/index_analyzer.cpp)
- [`include/storage/index_analyzer.h`](../../../include/storage/index_analyzer.h)
- [`src/storage/storage_layout_advisor.cpp`](../../../src/storage/storage_layout_advisor.cpp)
- [`config/index_analyze.yaml`](../../../config/index_analyze.yaml)

---
**Last Updated:** 2026-04-27
