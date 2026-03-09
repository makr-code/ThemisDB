# Migration Guide: v1.3 → v1.4

> **Scope:** Breaking and behavior changes between ThemisDB v1.3.x and v1.4.0

---

## FLARE Active Retrieval – Enabled by Default (Breaking Behavior Change)

### What Changed

Starting with **v1.4.0**, the FLARE (Feedback Loop Active Retrieval) feature with
Token Perplexity Threshold (TPT) gating is **enabled by default** in
`KnowledgeGapConfig`:

```cpp
// v1.3.x
bool enable_flare = false;  // was disabled by default

// v1.4.0+
bool enable_flare = true;   // enabled by default
```

### Impact

- Applications that instantiate `KnowledgeGapDetector` with default configuration
  will now execute FLARE re-retrieval rounds when token perplexity exceeds the
  threshold (`perplexity_threshold = 100.0`).
- Typical latency change: **< 5 %** for queries where FLARE does not trigger.
  Queries that trigger FLARE may use up to 3 additional retrieval rounds
  (< 500 ms each).

### Migration Options

#### Option 1 – Keep new default (recommended)

No code changes required. FLARE improves retrieval quality for complex queries.

#### Option 2 – Restore v1.3 behaviour explicitly

```cpp
// Use legacy factory (v1.3-compatible, FLARE disabled)
auto detector = KnowledgeGapDetectorFactory::createLegacy();
```

Or configure manually:

```cpp
KnowledgeGapConfig config;
config.enable_flare = false;  // explicit opt-out
auto detector = std::make_unique<KnowledgeGapDetector>(config);
```

#### Option 3 – Use production-ready factory (FLARE + full defaults)

```cpp
auto detector = KnowledgeGapDetectorFactory::createProductionReady();
```

---

## Configuration File

A new production configuration file has been added:

```
config/rag/default_production.yaml
```

Load it to use the recommended v1.4.0 defaults without any code changes.

---

## Factory Methods Added

| Method | FLARE | Description |
|---|---|---|
| `createProductionReady()` | ✅ enabled | New in v1.4.0 – recommended for new deployments |
| `createLegacy()` | ❌ disabled | New in v1.4.0 – v1.3 backward-compatible mode |
| `createFast()` | ❌ disabled | Unchanged – pre-generation only |
| `createBalanced()` | ❌ disabled | Unchanged – note: use `createProductionReady()` instead |
| `createThorough()` | ❌ disabled | Unchanged |

---

## Related Documentation

- [FLARE + TPT Production Guide (DE)](de/features/RAG_FLARE_TPT_PRODUCTION_GUIDE.md)
- [Knowledge Gap Detector Phase 2 History](implementation-history/KNOWLEDGE_GAP_DETECTOR_PHASE2_COMPLETE.md)
