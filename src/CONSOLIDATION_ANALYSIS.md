# Code Consolidation Analysis — ThemisDB src/ Duplicates & Opportunities

**Document Version:** 1.0  
**Generated:** 2026-05-18  
**Status:** Active Analysis · Linked to ROADMAP.md Consolidation Epic  
**Scope:** src/ modules · Focus: utility functions, error handling, networking, ingestion  

---

## 1. Executive Summary

### Current State
Scan of 50+ modules and 1000+ files identified **5 major consolidation opportunities** with varying maturity levels:

| Priority | Category | Issue | Occurrences | Effort | Impact |
|----------|----------|-------|-------------|--------|--------|
| **HIGH** | String Utilities | `trim()` duplicated | 8 modules | Low | High (10–20 instances refactored) |
| **HIGH** | Retry/Backoff | Manual loops despite `retry_policy.h` | 30+ files | Medium | Very High (eliminates 15+ retry variants) |
| **MEDIUM** | Markdown Processing | `stripMarkdownFences()` variants | 2 files | Low | Medium (consistency, testability) |
| **MEDIUM** | Ingestion Plumbing | `RetryConfig` repeated setup | 8 connectors | Medium | Medium (DRY principle, API clarity) |
| **LOW** | Distance Functions | Haversine wrappers | 3+ files | Low | Low (already centralized; thin adapters ok) |

### Module Status Coverage

The consolidation work is easiest to interpret against the current module posture in [`MODULE_INDEX.md`](MODULE_INDEX.md):

| Status | Module groups | What it means for consolidation |
|---|---|---|
| Production-ready / minor gaps | `server`, `storage`, `network`, `auth`, `security`, `cache`, `analytics`, `failover`, `maintenance`, `updates`, `process`, `execution` | Consolidation here should be low-risk cleanup only; avoid destabilizing working paths. |
| Hardening in progress | `themis`, `transaction`, `query`, `index`, `sharding`, `replication`, `graph`, `cdc`, `llm`, `rag`, `gpu`, `acceleration`, `geo`, `voice`, `access_model`, `ethics_ai` | These modules are where duplication reduction and gap closure can still move the implementation needle. |
| Planned / externalized | `chimera`, `user_storage`, selected plugin-externalization paths | Prefer documentation and boundary clarification before consolidation changes. |

### Recommendations
- **Immediate (v1.9.x):** Implement consolidated `string_utils.h` for `trim()` family; migrate 2 highest-effort retry loops.
- **Near-term (v1.5–1.6):** Unify ingestion connector `RetryConfig` plumbing via mixin/base; audit Markdown fence stripping.
- **Long-term (v1.6+):** Complete retry-policy migration; establish utility code review guidelines to prevent new duplicates.

---

## 2. Detailed Findings

### 2.1 String Utilities — `trim()` Implementations

**Problem:** Eight independent file-local `trim()` implementations with identical logic.

#### Instances

| File | Line(s) | Type | Signature | Notes |
|------|---------|------|-----------|-------|
| `analytics/knowledge_base.cpp` | 58–63 | static func | `std::string trim(const std::string& s)` | Simple loop + find_first/last_not_of |
| `updates/dependency_resolver.cpp` | 76–81 | static func | `std::string trim(const std::string& s)` | Identical to above |
| `ingestion/legal_domain.cpp` | 45–50 | std func | `std::string trim(const std::string& s)` | Identical; in anonymous namespace |
| `scheduler/event_trigger.cpp` | 47–57 | static func | `std::string trim(const std::string& s)` | Identical |
| `llm/aql_train_parser.cpp` | 58–63 | std func | `std::string trim(const std::string& s)` | Identical |
| `training/modality_parser.cpp` | 83–88 | static func | `std::string trim(const std::string& s)` | Identical |
| `rag/multi_hop_reasoner.cpp` | 64–72 | std func | `std::string trim(const std::string& s)` | Identical |
| `process/dmn_evaluator.cpp` | 52–60 | std func | `std::string_view trim(std::string_view sv)` | Returns string_view (avoids copy) |
| `prompt_engineering/prompt_template_compiler.cpp` | 102–108 | static func | `std::string trim(std::string_view sv)` | Returns std::string from std::string_view |

#### Pattern

```cpp
// Variant 1: std::string input → std::string output (7 instances)
static std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Variant 2: std::string_view input → std::string_view output (1 instance)
std::string_view trim(std::string_view sv) {
    sv = {sv.data() + (sv.find_first_not_of(" \t\r\n") - sv.data()), …};
    // Similar logic
}

// Variant 3: std::string_view input → std::string output (1 instance)
static std::string trim(std::string_view sv) { … }
```

#### Consolidation Plan

**Create:** `include/utils/string_utils.h`

```cpp
#pragma once
#include <string>
#include <string_view>

namespace themis::utils {

/// Trim leading/trailing whitespace (default: " \t\r\n").
/// @param s Input string.
/// @param ws Whitespace character set (default " \t\r\n").
/// @return Trimmed string (returns empty if input is all-whitespace).
std::string trim(const std::string& s, std::string_view ws = " \t\r\n");

/// Trim std::string_view in-place (no allocation).
/// @return Trimmed view of original data (may be empty).
std::string_view trim_view(std::string_view sv, std::string_view ws = " \t\r\n");

/// Left-trim only.
std::string ltrim(const std::string& s, std::string_view ws = " \t\r\n");

/// Right-trim only.
std::string rtrim(const std::string& s, std::string_view ws = " \t\r\n");

} // namespace themis::utils
```

**Implementation:** `src/utils/string_utils.cpp` (50 lines).

**Migration Path:**
1. Phase 1: Add header + implementation; test with unit tests.
2. Phase 2a: Migrate 3 easiest files (no other interdependencies).
2. Phase 2b: Migrate remaining 5 files.
3. Phase 3: Remove file-local implementations; update includes.

**Effort:** Low (1–2 hours including tests).  
**Impact:** Eliminates 8 duplicate implementations; centralizes whitespace handling logic; enables future extensions (e.g., custom charsets).

---

### 2.2 Retry/Backoff Loops — Manual Duplication vs. `retry_policy.h`

**Problem:** 30+ manual retry loops despite centralized `include/utils/retry_policy.h` (v1.9.0).

#### Canonical Source
- **File:** `include/utils/retry_policy.h`
- **Provides:** `retry_with_backoff<T>()` template, `ExponentialBackoff` class, `RetryConfig` struct
- **Status:** Production-ready; used in `rag/http_metrics_client.cpp` and `rag/llm_judge_integration.cpp`

#### Instances of Manual Duplication

| File | Line(s) | Pattern | Config | Backoff |
|------|---------|---------|--------|---------|
| `updates/hardware_telemetry.cpp` | 506–520 | `for(attempt <= max_retries)` | `2s` fixed sleep | Fixed |
| `updates/parallel_downloader.cpp` | 368–??? | `for(attempt <= max_retries)` | `sleep_ms` calc | Exponential |
| `exporters/huggingface_hub_client.cpp` | 512–530 | `for(attempt <= max_retries)` | `retry_delay_ms * (1<<(attempt-1))` | Exponential w/ jitter |
| `transaction/saga.cpp` | 141–??? | `for(attempt <= max_retries)` | unclear | ??? |
| `transaction/saga_orchestrator.cpp` | 243–??? | `for(attempt <= max_retries)` | unclear | ??? |
| `transaction/distributed_saga.cpp` | 410–???, 534–??? | `for(attempt <= max_retries)` | unclear | ??? |
| `ingestion/ingestion_quality_judge.cpp` | 629–??? | `for(attempt < max_attempts)` | ??? | ??? |
| `ingestion/ingestion_manager.cpp` | 2235–??? | `for(attempt=1; attempt<=max_attempts)` | ??? | ??? |
| `process/process_model_generator.cpp` | 375–???, 443–??? | `for(attempt < max_retries)` | ??? | ??? |
| `observability/alertmanager.cpp` | 172–??? | `for(attempt=1; attempt<=attempts)` | ??? | ??? |
| `cache/adaptive_query_cache.cpp` | 118–??? | `while(retry_count < max_retries)` | ??? | ??? |
| `base/remote_registry_client.cpp` | 532–???, 666–??? | `for(attempt < attempts)` | ??? | ??? |
| `ingestion/web_crawler_connector.cpp` | 503–??? | `for(attempt=1; attempt<=retry_config_.max_attempts)` | ??? | ??? |
| `aql/llm_aql_handler.cpp` | 1494–???, 1590–???, 1891–??? | `for(attempt < max_attempts)` | ??? | ??? |
| `sharding/data_migrator.cpp` | 382–??? | `for(attempt < max_retries)` | ??? | ??? |
| `sharding/cross_shard_transaction.cpp` | 2657–??? | `for(attempt <= max_retries)` | ??? | ??? |
| `sharding/mtls_client.cpp` | 158–??? | `while(retry_count <= max_retries)` | ??? | ??? |
| `sharding/wal_applier.cpp` | 122–135 | `for(attempt < max_apply_retries)` | `100 * (attempt+1) ms` | Exponential |
| `scheduler/task_scheduler.cpp` | 263–295, 854–???, 1777–??? | `for(attempt < max_attempts)` | ??? | ??? |
| `prompt_engineering/structured_output.cpp` | 397–??? | `for(attempt=1; attempt<=max_attempts)` | ??? | ??? |
| `rag/http_metrics_client.cpp` | 153–??? | ✅ **ALREADY MIGRATED** | `ExponentialBackoff` | Yes |
| `storage/database_connection_manager.cpp` | 338–??? | custom backoff logic | ??? | ??? |
| *... +8 more* | | | | |

#### Example Canonical Usage
```cpp
// From rag/http_metrics_client.cpp (v1.9.0, already migrated)
const themis::utils::RetryConfig retry_cfg{
    .max_attempts       = 3,
    .initial_backoff_ms = 100,
    .max_backoff_ms     = 5'000,
    .multiplier         = 2.0,
    .jitter_fraction    = 0.1,
};

themis::utils::ExponentialBackoff backoff(retry_cfg);
for (int attempt = 0; attempt < retry_cfg.max_attempts; ++attempt) {
    if (tryOnce()) return;
    if (!backoff.wait()) break; // Sleeps + advances exponentially
}

// OR use the template directly:
auto result = themis::utils::retry_with_backoff(
    [&]() -> std::optional<Result> {
        auto r = doWork();
        if (r.is_transient()) return std::nullopt;  // retry
        return r;
    },
    retry_cfg);
```

#### Consolidation Plan

**Migration Strategy:**
1. **Phase 1:** Audit all 30+ loops; categorize by pattern (fixed delay, exponential, backoff, jitter presence).
2. **Phase 2:** For each file, migrate to `ExponentialBackoff` or `retry_with_backoff()`:
   - Replace `for (int attempt = 0; attempt <= max_retries; ++attempt)` with template or class.
   - Replace `std::this_thread::sleep_for(fixed_ms)` with `backoff.wait()`.
   - Remove ad-hoc backoff calculation; use `RetryConfig` instead.
3. **Phase 3:** Update ingestion connectors to use shared `RetryConfig` struct (see 2.3).
4. **Phase 4:** Add GTest cases for each migrated module to verify retry behavior.

**High-Effort Files (tier 1):**
- `exporters/huggingface_hub_client.cpp` (512–530) — has exponential logic; slight refactor needed
- `updates/parallel_downloader.cpp` (368–???) — clarify config source
- `sharding/wal_applier.cpp` (122–135) — exponential; straightforward

**Effort:** High (15–20 hours across all 30 loops, but parallelizable by file).  
**Impact:** Very High
- Eliminates ad-hoc retry logic; reduces bugs (jitter handling, max-delay clamping).
- Centralizes timeout/backoff semantics; improves observability.
- Enables future enhancements (e.g., metrics per retry attempt) in one place.

**Roadmap Target:** v1.5–1.6 (staggered migration by module criticality).

---

### 2.3 Ingestion Connectors — Repeated `RetryConfig` Plumbing

**Problem:** Eight connectors repeat identical `setRetryConfig()` / `getRetryConfig()` boilerplate.

#### Affected Connectors

| Connector | File | Impl Lines | Pattern |
|-----------|------|-----------|---------|
| API | `ingestion/api_connector.cpp` | ~625, ~635 | Private impl + public delegate |
| HuggingFace | `ingestion/huggingface_connector.cpp` | ~666, ~748 | Same pattern |
| Kafka | `ingestion/kafka_connector.cpp` | ~553, ~590 | Same pattern |
| S3 | `ingestion/s3_connector.cpp` | ~696, ~738 | Same pattern |
| Object Storage | `ingestion/object_storage_connector.cpp` | ~680, ~714 | Same pattern |
| Database | `ingestion/database_connector.cpp` | ~724, ~754 | Same pattern |
| CDC | `ingestion/cdc_connector.cpp` | ~821, ~851 | Same pattern |
| Web Crawler | `ingestion/web_crawler_connector.cpp` | ~624, ~654 | Same pattern |

#### Boilerplate Pattern

**Private Impl:**
```cpp
class FooConnector::Impl {
    RetryConfig retry_config_;
    void setRetryConfig(const RetryConfig& c) { retry_config_ = c; }
    RetryConfig getRetryConfig() const { return retry_config_; }
    // ... used internally in connect/fetch loops
};
```

**Public Facade:**
```cpp
void FooConnector::setRetryConfig(const RetryConfig& config) {
    impl_->setRetryConfig(config);
}
RetryConfig FooConnector::getRetryConfig() const {
    return impl_->getRetryConfig();
}
```

#### Consolidation Plan

**Option A: Mixin Base Class (Recommended)**

Create `include/ingestion/iconnector_retry_policy.h`:

```cpp
class ISourceConnectorRetryPolicy {
protected:
    RetryConfig retry_config_;
    
public:
    virtual ~ISourceConnectorRetryPolicy() = default;
    
    void setRetryConfig(const RetryConfig& config) {
        retry_config_ = config;
    }
    
    RetryConfig getRetryConfig() const {
        return retry_config_;
    }
    
    // Protected helpers for subclasses:
    const RetryConfig& getRetryConfigRef() const { return retry_config_; }
};
```

Each connector's Impl class inherits:
```cpp
class ApiConnector::Impl : public ISourceConnectorRetryPolicy {
    // No more manual setRetryConfig/getRetryConfig!
};
```

**Option B: Delegating Wrapper (Simpler)**

Update the public facade to inherit from the Impl pattern via macro:

```cpp
#define DEFINE_RETRY_CONFIG_METHODS(CLASS, IMPL_VAR) \
    void CLASS::setRetryConfig(const RetryConfig& cfg) { IMPL_VAR->setRetryConfig(cfg); } \
    RetryConfig CLASS::getRetryConfig() const { return IMPL_VAR->getRetryConfig(); }
```

Less intrusive; easier to adopt in existing code.

**Effort:** Low (2–3 hours; mechanical refactor).  
**Impact:** Medium
- Reduces per-connector boilerplate by ~10 lines.
- Clarifies that all connectors share a common retry lifecycle.
- Enables future additions (e.g., per-connector retry metrics) in one place.

---

### 2.4 Markdown Fence Stripping — Two Independent Implementations

**Problem:** Two similar but not identical `stripMarkdownFences()` functions with different regex engines and coverage.

#### Instances

| File | Line(s) | Method | Regex? | Handles `json` language tag? | Handles CRLF? |
|------|---------|--------|--------|------------------------------|---------------|
| `aql/llm_aql_handler.cpp` | 1427–1449 | String search + manual substr | ❌ No | N/A | ✅ Auto-handled |
| `prompt_engineering/structured_output.cpp` | 20–28 | Regex-based | ✅ Yes (std::regex) | ✅ Explicit `\r?\n?` | ✅ Yes |

#### Implementations

**Variant 1: String Search (llm_aql_handler.cpp)**
```cpp
std::string LLMAQLHandler::stripMarkdownFences(std::string raw) {
    size_t start_marker = raw.find("```");
    if (start_marker != std::string::npos) {
        size_t query_start = raw.find('\n', start_marker);
        if (query_start != std::string::npos) {
            query_start++;
            size_t end_marker = raw.find("```", query_start);
            if (end_marker != std::string::npos) {
                raw = raw.substr(query_start, end_marker - query_start);
            }
        }
    }
    // Manual trim using std::isspace lambda
    raw.erase(raw.begin(), std::find_if(raw.begin(), raw.end(), 
        [](unsigned char ch) { return !std::isspace(ch); }));
    // ... trim end
    return raw;
}
```

**Variant 2: Regex (structured_output.cpp)**
```cpp
std::string StructuredOutputEnforcer::stripMarkdownFences(const std::string& text) {
    static const std::regex open_fence(R"(^```(?:json)?\r?\n?)", 
        std::regex::ECMAScript);
    static const std::regex close_fence(R"(\r?\n?```$)", 
        std::regex::ECMAScript);
    
    std::string result = std::regex_replace(text, open_fence, "");
    result = std::regex_replace(result, close_fence, "");
    return result;
}
```

#### Issues

1. **Inconsistency:** Variant 2 handles `\r\n` (Windows CRLF); Variant 1 does not.
2. **Language tag:** Variant 2 strips `json` prefix; Variant 1 does not.
3. **Performance:** Regex slower for simple string search; Variant 1 more efficient.

#### Consolidation Plan

**Create:** `include/prompt_engineering/markdown_utils.h`

```cpp
namespace themis::prompt_engineering {

/// Strip outer code-fence markers (``` or ```json, etc.).
/// Handles Unix (LF) and Windows (CRLF) line endings.
/// @param text Input text possibly wrapped in ```...```.
/// @param language_tag Output param: filled with language tag if present (e.g., "json").
/// @return Text with fences removed and trimmed.
std::string stripMarkdownFences(const std::string& text, 
                                std::string* language_tag = nullptr);

/// Strip markdown code fences and optional line comments (// ...).
std::string stripMarkdownAndComments(const std::string& text);

} // namespace themis::prompt_engineering
```

**Implementation Strategy:**
- Use hybrid approach: string search for opening fence + capture language tag; regex for closing fence to handle CRLF.
- Cache compiled regex in static to avoid recompilation.
- Add comprehensive unit tests for edge cases (Windows CRLF, nested fences, empty content, language tags).

**Migration:**
1. Implement new function in `src/prompt_engineering/markdown_utils.cpp`.
2. Update `structured_output.cpp` to use new function.
3. Update `llm_aql_handler.cpp` to use new function.
4. Remove old implementations.

**Effort:** Low (1–2 hours including tests).  
**Impact:** Medium
- Eliminates duplication; improves consistency (both Windows and Unix line endings handled).
- Enables reuse in other modules (e.g., prompt compression, output enforcer).

---

### 2.5 Haversine Distance Wrappers — Localized Adapters (Status: OK)

**Analysis:** 3+ modules wrap the canonical `themis::geo::haversine_km()` / `themis::geo::haversine_m()` from `include/utils/geometric_distances.h`.

#### Instances

| File | Function | Wraps |
|------|----------|-------|
| `index/spatial_index.cpp` | `haversineDistance()` | `themis::geo::haversine_m()` |
| `index/secondary_index.cpp` | `haversineDistance()` | `themis::geo::haversine_km()` |
| `acceleration/cpu_backend.cpp` | `haversineDistance()` | `themis::geo::haversine_km()` |
| `acceleration/graphics_backends.cpp` | `opengl_haversine_km()`, `vulkan_haversine_km()` | `themis::geo::haversine_km()` |

#### Assessment

✅ **No Action Required:** These are thin adapter/wrapper functions serving module-specific contexts (e.g., GPU dispatch, unit conversions). They are:
- Intentionally kept local for API clarity (e.g., "spatial index uses meters; secondary index uses km").
- Minimal logic (1–2 lines; just delegates to canonical function).
- Part of established Phase 1 consolidation (ROADMAP.md:440–446).

**Conclusion:** Already consolidated at the canonical level; local wrappers provide appropriate abstraction boundaries.

---

## 3. Priority & Phasing

### Recommended Execution Order

| Phase | Items | Target | Effort | Blockers |
|-------|-------|--------|--------|----------|
| **Phase 1 (v1.9.x)** | String utils (`trim()`), Markdown fences | Now–Q2 2026 | 3–4h | None |
| **Phase 2a (v1.5.0)** | High-effort retry loops (HuggingFace, WAL, parallel downloader) | Q2 2026 | 5–8h | None |
| **Phase 2b (v1.5–1.6)** | Ingestion connector plumbing (mixin or macro) | Q2–Q3 2026 | 2–3h | Phase 2a completion |
| **Phase 3 (v1.6+)** | Remaining 20+ retry loops (low-urgency files) | Q3 2026 | 8–12h | Phase 2a knowledge transfer |

### Quick Wins
1. **`trim()` consolidation:** 1–2 hours; eliminates 8 duplicates; no API changes required.
2. **Markdown fence unification:** 1–2 hours; improves cross-platform compatibility.

### High Impact
3. **Retry policy migration (tier 1):** 5–8 hours; prevents future bugs; improves observability.

---

## 4. Acceptance Criteria

### Per Consolidation Item

#### String Utilities
- [ ] `include/utils/string_utils.h` created with `trim()`, `ltrim()`, `rtrim()`, `trim_view()`.
- [ ] `src/utils/string_utils.cpp` implements all functions.
- [ ] Unit tests pass: `tests/unit/test_string_utils.cpp` (8+ cases).
- [ ] All 8 duplicate implementations removed; callers updated.
- [ ] No regression in existing functionality (CI green).

#### Retry/Backoff (Tier 1)
- [ ] Audit document completed: 30+ files categorized by retry pattern.
- [ ] 3 highest-priority files migrated (`huggingface_hub_client`, `parallel_downloader`, `wal_applier`).
- [ ] `retry_policy.h` usage verified; backoff behavior matches original.
- [ ] Tests pass for each migrated file.

#### Ingestion Connector Plumbing
- [ ] `include/ingestion/iconnector_retry_policy.h` created (Option A) OR macro defined (Option B).
- [ ] All 8 connectors refactored; boilerplate eliminated.
- [ ] No API change for public-facing `setRetryConfig()` / `getRetryConfig()` methods.
- [ ] CI green; no behavioral change.

#### Markdown Fences
- [ ] `include/prompt_engineering/markdown_utils.h` created.
- [ ] `src/prompt_engineering/markdown_utils.cpp` implemented.
- [ ] Unit tests: Windows CRLF, Unix LF, language tags, nested fences.
- [ ] Both callers updated; old implementations removed.

---

## 5. Known Non-Issues

### Items NOT Consolidated

1. **Haversine wrappers:** Thin adapters; no consolidation needed (already centralized).
2. **Domain-specific stubs:** `src/storage/transaction_retry_manager.cpp` intentionally separate (transactional semantics differ).
3. **Single-shot sleeps:** Network/comms modules with fixed 100ms pauses; not retry loops; no migration applicable.
4. **Copy-paste in test files:** Minimal overlap; test duplication acceptable (isolated test contexts).

---

## 6. Follow-Up Actions

1. **Immediate:** Create GitHub issues for Phase 1 items (string utils, markdown utils).
2. **Q2 2026:** Start Phase 2a migration (retry loops); assign to dedicated owner.
3. **Q3 2026:** Complete Phase 2b + 3; establish code review checklist to prevent new duplicates.
4. **Post-v1.6:** Review new utility code; enforce consolidation patterns in PR reviews.

---

## 7. References

- **ROADMAP.md:** Consolidation Epic phases (lines 433–490)
- **retry_policy.h:** Canonical retry/backoff interface (usage docs + examples)
- **geometric_distances.h:** Centralized distance functions (already consolidated)
- **STUB_INVENTORY.md:** Simulation/stub tracking (orthogonal concern; separate epic)

---

## Appendix A: File-by-File Inventory

### String Utilities Candidates
```
✓ analytics/knowledge_base.cpp:58–63              (std::string → std::string)
✓ updates/dependency_resolver.cpp:76–81           (std::string → std::string)
✓ ingestion/legal_domain.cpp:45–50                (std::string → std::string)
✓ scheduler/event_trigger.cpp:47–57               (std::string → std::string)
✓ llm/aql_train_parser.cpp:58–63                  (std::string → std::string)
✓ training/modality_parser.cpp:83–88              (std::string → std::string)
✓ rag/multi_hop_reasoner.cpp:64–72                (std::string → std::string)
✓ process/dmn_evaluator.cpp:52–60                 (std::string_view → std::string_view)
✓ prompt_engineering/prompt_template_compiler.cpp:102–108 (std::string_view → std::string)
```

### Retry/Backoff Manual Loops (30+)
```
✓ updates/hardware_telemetry.cpp:506–520
✓ updates/parallel_downloader.cpp:368+
✓ exporters/huggingface_hub_client.cpp:512–530
✓ transaction/saga.cpp:141+
✓ transaction/saga_orchestrator.cpp:243+
✓ transaction/distributed_saga.cpp:410+, 534+
... (21 more files listed in section 2.2)
```

### Ingestion Connectors (RetryConfig Plumbing)
```
✓ ingestion/api_connector.cpp
✓ ingestion/huggingface_connector.cpp
✓ ingestion/kafka_connector.cpp
✓ ingestion/s3_connector.cpp
✓ ingestion/object_storage_connector.cpp
✓ ingestion/database_connector.cpp
✓ ingestion/cdc_connector.cpp
✓ ingestion/web_crawler_connector.cpp
```

### Markdown Fence Strippers
```
✓ aql/llm_aql_handler.cpp:1427–1449
✓ prompt_engineering/structured_output.cpp:20–28
```

---

**Document Status:** Draft · Ready for stakeholder review  
**Next Review:** Post-Phase-1 completion  
**Owner:** Code Quality & Consolidation Team
