# MDM API Reference

> Alignment note (2026-05-31): This API reference is a secondary convenience document.
> Authoritative current workload and target behavior are defined in:
> - `src/importers/FUTURE_ENHANCEMENTS.md`
> - `src/importers/MODULE_GAPS.md`
> - `src/importers/ROADMAP.md`
> If this reference conflicts with newer planning docs, planning docs take precedence.

## Namespaces

All MDM types live in `themis::importers`.

---

## entity_matcher.h

### `DeterministicMatcher`

```cpp
class DeterministicMatcher {
public:
    struct MatchResult {
        std::string existing_entity_id;      // UUID of matched entity (empty = no match)
        double      confidence_score;        // 1.0 for exact match, 0.0 if none
        std::vector<std::string> match_keys; // Field names that produced the match
        json        evidence;                // Field → value pairs compared
    };

    std::vector<MatchResult> findExactMatches(
        const json&                     incoming_entity,
        const std::string&              collection_name,
        const std::vector<std::string>& key_fields
    ) const;

    MatchResult findByPrimaryKey(
        const json& incoming_entity,
        const std::string& collection_name
    ) const;

    std::vector<MatchResult> findByUniqueFields(
        const json&                     incoming_entity,
        const std::string&              collection_name,
        const std::vector<std::string>& unique_field_names
    ) const;

    MatchResult findByCustomIdentifier(
        const json& incoming_entity,
        const std::string& collection_name,
        const json& identifier_mapping        // {"source_field": "target_field", …}
    ) const;
};
```

### `SemanticMatchConfig`

```cpp
struct SemanticMatchConfig {
    double  overall_threshold = 0.80;
    size_t  max_results       = 10;
    bool    use_embeddings    = false;
    std::map<std::string, double>      field_weights;    // field → [0,1]
    std::map<std::string, std::string> field_algorithms; // field → algorithm name
};
```

### `SemanticMatcher`

```cpp
class SemanticMatcher {
public:
    // String distance
    static double jaroWinklerDistance(const std::string& s1, const std::string& s2);
    static double levenshteinSimilarity(const std::string& s1, const std::string& s2);

    // Name helpers
    static std::string normalizeFullName(const std::string& name);
    static double      soundexMatch(const std::string& n1, const std::string& n2);
    static double      scoreNameVariations(const std::string& n1, const std::string& n2);

    // Email helpers
    static double scoreEmailPair(const std::string& e1, const std::string& e2);
    static bool   isLikelyEmailTypo(const std::string& e1, const std::string& e2);

    // Phone helpers
    static std::string normalizePhoneNumber(const std::string& phone);
    static double      scorePhonePair(const std::string& p1, const std::string& p2);

    // Vector similarity
    static double vectorSimilarity(
        const std::vector<float>& v1,
        const std::vector<float>& v2
    );

    // Main scoring engine
    EntityMatchScore scoreEntityMatch(
        const json& incoming_entity,
        const json& existing_entity,
        const std::string& collection_name,
        const SemanticMatchConfig& config
    ) const;

    std::vector<EntityMatchScore> findSimilarEntities(
        const json& incoming_entity,
        const std::vector<json>& candidates,
        const SemanticMatchConfig& config
    ) const;
};
```

### `HybridEntityMatcher`

```cpp
class HybridEntityMatcher {
public:
    enum class MatchStrategy { DETERMINISTIC_FIRST, SEMANTIC_FIRST, WEIGHTED_ENSEMBLE };

    std::vector<HybridMatchResult> findMatchingEntities(
        const json&                     incoming_entity,
        const std::vector<json>&        existing_entities,
        const std::vector<std::string>& key_fields,
        MatchStrategy                   strategy,
        const SemanticMatchConfig&      sem_config,
        double                          threshold = 0.85
    ) const;

    static MatchStrategy selectOptimalStrategy(
        const std::vector<FieldCharacteristics>& field_stats
    );
};
```

---

## entity_linker.h

### `EntityLink`

```cpp
struct EntityLink {
    std::string              source_id;
    std::string              target_id;
    LinkType                 link_type  = LinkType::SAME_AS;
    ResolutionStatus         status     = ResolutionStatus::UNRESOLVED;
    double                   confidence = 0.0;
    json                     matching_evidence;
    std::vector<std::string> matched_fields;
    std::string              created_at;    // RFC 3339
    std::string              created_by;
    json                     metadata;

    json toJson() const;
};
```

### `EntityLinker`

```cpp
class EntityLinker {
public:
    bool createLink(const EntityLink& link, const ImportOptions& options);

    ImportStats linkBatch(
        const std::string&              collection_name,
        const std::vector<EntityLink>&  links,
        const ImportOptions&            options,
        size_t                          batch_size = 1000
    );

    std::vector<LinkAuditEntry> getLinksForEntity(
        const std::string& entity_id,
        const std::string& collection_name
    ) const;

    json exportLinkGraph(
        const std::string&              collection_name,
        const std::vector<std::string>& entity_ids,
        bool                            include_confidence_scores = true
    ) const;

    size_t linkCount() const;
    void   clear();
};
```

---

## canonical_resolver.h

### `GoldenRecord`

```cpp
struct GoldenRecord {
    std::string              canonical_id;
    json                     merged_data;
    std::vector<std::string> contributing_ids;
    double                   completeness_score;   // 0.0–1.0
    json                     field_provenance;      // field → source entity ID
    std::string              last_reconciliation;  // RFC 3339

    json toJson() const;
};
```

### `CanonicalEntityResolver`

```cpp
class CanonicalEntityResolver {
public:
    GoldenRecord createGoldenRecord(
        const std::vector<std::pair<std::string, json>>& linked_entities,
        const std::string&                               collection_name,
        ResolutionPolicy                                 policy,
        const std::map<std::string, FieldRule>&          field_rules      = {},
        const std::vector<std::string>&                  protected_fields = {}
    ) const;

    // Field-level reconciliation
    static std::string reconcileStringField(
        const std::string& value1, const std::string& value2,
        FieldRule rule, const std::string& separator = " | "
    );

    static int64_t reconcileNumericField(
        int64_t value1, int64_t value2, FieldRule rule
    );

    static json reconcileObjectField(
        const json& obj1, const json& obj2,
        ResolutionPolicy policy, int depth = -1
    );

    static double scoreFieldQuality(
        const std::string& field_name, const std::string& value,
        const FieldQualityPolicy& policy = {}
    );
};
```

---

## mdm_engine.h

### `MDMConfig`

```cpp
struct MDMConfig {
    HybridEntityMatcher::MatchStrategy match_strategy;
    double deterministic_threshold = 1.0;
    double semantic_threshold      = 0.85;
    SemanticMatchConfig semantic_config;

    std::vector<std::string> primary_key_fields;
    std::vector<std::string> unique_fields;

    LinkType         preferred_link_type    = LinkType::SAME_AS;
    bool             create_reverse_links   = true;
    ResolutionPolicy resolution_policy      = ResolutionPolicy::RICHEST_MERGE;
    bool             auto_resolve_conflicts = false;

    std::map<std::string, FieldRule> field_rules;
    std::vector<std::string>         protected_fields;

    bool        parallelize_matching = true;
    size_t      batch_size           = 1000;
    bool        log_all_decisions    = true;
    std::string initiated_by         = "importer_v2.2";

    json toJson() const;
};
```

### `MDMEngine`

```cpp
class MDMEngine {
public:
    MDMWorkflowResult executeMDMWorkflow(
        const std::vector<json>& incoming_entities,
        const std::vector<json>& existing_entities,
        const std::string&       collection_name,
        const MDMConfig&         config,
        const ImportOptions&     options
    );

    // Individual phases (for debugging / custom pipelines)
    std::vector<std::vector<HybridMatchResult>> executeMatchingPhase(...);
    std::vector<EntityLink>  executeLinkingPhase(...);
    std::vector<GoldenRecord> executeResolutionPhase(...);
};
```

---

## mdm_audit_trail.h

### `MDMAuditTrail`

```cpp
class MDMAuditTrail {
public:
    enum class Operation {
        MATCH_FOUND, LINK_CREATED, CONFLICT_DETECTED, CONFLICT_RESOLVED,
        GOLDEN_RECORD_CREATED, ENTITY_MERGED, REVIEW_REQUESTED, REVIEW_COMPLETED
    };

    struct AuditEvent {
        std::string  event_id;           // Auto-generated UUID
        Operation    operation;
        std::string  collection_name;
        std::string  source_entity_id;
        std::string  target_entity_id;
        double       confidence_score;
        json         event_details;
        std::string  timestamp;          // RFC 3339, auto-generated
        std::string  initiated_by;
        std::string  status;
        std::string  chain_hash;         // FNV-1a chain hash, auto-computed

        json toJson() const;
    };

    void recordEvent(AuditEvent event);

    std::vector<AuditEvent> getAuditFor(
        const std::string& entity_id,
        const std::string& collection_name,
        const std::optional<Operation>& operation_filter = std::nullopt
    ) const;

    bool verifyAuditChain() const;

    json exportAuditReport(
        const std::string& collection_name,
        const std::string& start_date,
        const std::string& end_date
    ) const;

    size_t eventCount() const;
    void   clear();

    static std::string operationName(Operation op);
};
```

---

## mdm_metrics.h

### `MDMMetricSnapshot`

```cpp
struct MDMMetricSnapshot {
    size_t deterministic_matches;
    size_t semantic_matches;
    double avg_semantic_confidence;
    size_t links_created;
    size_t links_with_conflicts;
    size_t conflicts_auto_resolved;
    size_t conflicts_requiring_review;
    double avg_resolution_confidence;
    size_t duplicate_records_found;
    size_t duplicate_records_merged;
    double avg_completeness_improvement;
    double matching_time_seconds;
    double linking_time_seconds;
    double resolution_time_seconds;

    json toJson() const;
};
```

### `MDMMetrics`

```cpp
class MDMMetrics {
public:
    static void emitMetrics(
        const MDMMetricSnapshot& snapshot,
        const std::string&       collection_name,
        const MetricsCallback&   callback       // nullptr = no-op
    );

    static json getDashboardMetrics(
        const MDMMetricSnapshot& snapshot,
        const std::string&       collection_name
    );
};
```

---

## Enumerations

### `LinkType`

```
SAME_AS | DUPLICATE_OF | SUBSUMED_BY | MERGED_INTO
VERSION_OF | RELATED_TO | POSSIBLY_SAME | CROSS_DOMAIN_LINK
```

### `ResolutionStatus`

```
UNRESOLVED | RESOLVED | MANUAL_REVIEW | ARCHIVED
```

### `ResolutionPolicy`

```
NEWEST_FIRST | MOST_COMPLETE | EXISTING_PREFERRED
INCOMING_PREFERRED | RICHEST_MERGE | CUSTOM_RULES
```

### `FieldRule`

```
KEEP_EXISTING | TAKE_INCOMING | TAKE_MAX | TAKE_MIN
TAKE_SUM | CONCATENATE | TAKE_LONGEST | TAKE_NEWEST
```
