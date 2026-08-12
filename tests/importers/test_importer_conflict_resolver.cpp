// Tests for the import conflict resolution strategies:
//   SKIP, OVERWRITE, MERGE (depth-1 and deep), ERROR
//
// Self-contained: all required types and logic are defined inline here so the
// tests run without the full ThemisDB build chain.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Minimal type mirror (matches importer_interface.h)
// ---------------------------------------------------------------------------

enum class ConflictStrategy {
    OVERWRITE,
    SKIP,
    MERGE,
    ERROR
};

enum class ImportErrorCode : uint32_t {
    SUCCESS       = 0,
    CONFLICT_ERROR = 600,
    UNKNOWN        = 900
};

enum class ImportErrorSeverity { INFO, WARNING, ERROR, CRITICAL };

struct ImportError {
    ImportErrorCode     code     = ImportErrorCode::UNKNOWN;
    ImportErrorSeverity severity = ImportErrorSeverity::ERROR;
    std::string         message;
    std::string         location;
};

struct ImportStats {
    size_t total_records      = 0;
    size_t imported_records   = 0;
    size_t failed_records     = 0;
    size_t skipped_records    = 0;
    size_t conflicts_skipped     = 0;
    size_t conflicts_overwritten = 0;
    size_t conflicts_merged      = 0;
    std::vector<ImportError> structured_errors;
};

using MetricsCallback = std::function<void(const std::string&,
                                            const std::map<std::string,std::string>&,
                                            double)>;

struct ImportOptions {
    ConflictStrategy conflict_strategy         = ConflictStrategy::OVERWRITE;
    std::vector<std::string> conflict_key_columns;
    std::vector<std::string> protected_fields;
    int  merge_depth       = 1;
    bool continue_on_error = true;
    MetricsCallback metrics_callback;
};

// ---------------------------------------------------------------------------
// Inline mirror of ImportConflictResolver (mirrors conflict_resolver.cpp)
// ---------------------------------------------------------------------------

class ImportConflictResolver {
public:
    void reset() { registry_.clear(); }

    static std::string computeKey(const json& entity,
                                   const std::vector<std::string>& key_columns) {
        if (key_columns.empty()) return {};
        static constexpr char kSep = '\x1F';
        std::string key;
        for (const auto& col : key_columns) {
            if (!key.empty()) key += kSep;
            if (entity.contains(col)) {
                const auto& v = entity[col];
                key += v.is_string() ? v.get<std::string>() : v.dump();
            }
        }
        return key;
    }

    json resolve(const json& entity,
                 const std::string& table_name,
                 const std::string& conflict_key,
                 ConflictStrategy strategy,
                 int merge_depth,
                 const std::vector<std::string>& protected_fields,
                 bool& conflict_detected) {
        conflict_detected = false;
        auto& tbl = registry_[table_name];
        auto it = tbl.find(conflict_key);
        if (it == tbl.end()) {
            tbl.emplace(conflict_key, entity);
            return entity;
        }
        conflict_detected = true;
        json& existing = it->second;
        switch (strategy) {
            case ConflictStrategy::SKIP:
                return existing;
            case ConflictStrategy::OVERWRITE:
                existing = entity;
                return entity;
            case ConflictStrategy::MERGE: {
                json merged = mergeEntities(existing, entity, merge_depth, protected_fields);
                existing = merged;
                return merged;
            }
            case ConflictStrategy::ERROR:
                return existing;
        }
        return entity;
    }

    static json mergeEntities(const json& existing, const json& incoming,
                               int depth,
                               const std::vector<std::string>& protected_fields) {
        if (!existing.is_object() || !incoming.is_object()) return incoming;
        json result = existing;
        for (auto it = incoming.begin(); it != incoming.end(); ++it) {
            const std::string& key = it.key();
            const json& value      = it.value();
            if (std::find(protected_fields.begin(), protected_fields.end(), key)
                    != protected_fields.end()) continue;
            bool can_recurse = (depth == -1 || depth > 1)
                               && result.contains(key)
                               && result[key].is_object()
                               && value.is_object();
            if (can_recurse) {
                int next_depth = (depth == -1) ? -1 : (depth - 1);
                result[key] = mergeEntities(result[key], value, next_depth, {});
            } else {
                result[key] = value;
            }
        }
        return result;
    }

private:
    std::unordered_map<std::string,
                       std::unordered_map<std::string, json>> registry_;
};

// ---------------------------------------------------------------------------
// Helper: simulate importing a list of entities and applying conflict logic
// ---------------------------------------------------------------------------

struct ImportResult {
    ImportStats stats;
    std::vector<json> emitted;       // entities that made it through
    std::vector<std::pair<std::string,std::map<std::string,std::string>>>
        metrics;                     // (metric, labels) pairs
};

static ImportResult runImport(const std::vector<json>& rows,
                               const std::string& table_name,
                               const ImportOptions& opts) {
    ImportResult result;
    ImportConflictResolver resolver;
    resolver.reset();

    auto emit_metric = [&](const std::string& m,
                           const std::map<std::string,std::string>& labels,
                           double /*v*/) {
        if (opts.metrics_callback) opts.metrics_callback(m, labels, 0.0);
        result.metrics.emplace_back(m, labels);
    };

    size_t row_num = 0;
    for (const json& row : rows) {
        ++row_num;

        if (opts.conflict_key_columns.empty()) {
            result.emitted.push_back(row);
            result.stats.imported_records++;
            continue;
        }

        std::string ckey = ImportConflictResolver::computeKey(row, opts.conflict_key_columns);
        bool conflict = false;
        json entity = resolver.resolve(row, table_name, ckey,
                                       opts.conflict_strategy,
                                       opts.merge_depth,
                                       opts.protected_fields,
                                       conflict);
        if (conflict) {
            switch (opts.conflict_strategy) {
                case ConflictStrategy::SKIP:
                    result.stats.conflicts_skipped++;
                    result.stats.skipped_records++;
                    emit_metric("importers_conflicts_total",
                                {{"table", table_name}, {"strategy", "skip"}, {"outcome", "skipped"}}, 1.0);
                    continue;
                case ConflictStrategy::OVERWRITE:
                    result.stats.conflicts_overwritten++;
                    emit_metric("importers_conflicts_total",
                                {{"table", table_name}, {"strategy", "overwrite"}, {"outcome", "overwritten"}}, 1.0);
                    break;
                case ConflictStrategy::MERGE:
                    result.stats.conflicts_merged++;
                    emit_metric("importers_conflicts_total",
                                {{"table", table_name}, {"strategy", "merge"}, {"outcome", "merged"}}, 1.0);
                    break;
                case ConflictStrategy::ERROR: {
                    ImportError err;
                    err.code     = ImportErrorCode::CONFLICT_ERROR;
                    err.severity = ImportErrorSeverity::ERROR;
                    err.message  = "Conflict for key '" + ckey + "' in table '" + table_name + "'";
                    err.location = "row " + std::to_string(row_num);
                    result.stats.structured_errors.push_back(err);
                    result.stats.failed_records++;
                    emit_metric("importers_conflicts_total",
                                {{"table", table_name}, {"strategy", "error"}, {"outcome", "error"}}, 1.0);
                    if (!opts.continue_on_error) return result;
                    continue;
                }
            }
        }
        result.emitted.push_back(entity);
        result.stats.imported_records++;
    }
    return result;
}

// ===========================================================================
// Test suites
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. ConflictStrategy::SKIP
// ---------------------------------------------------------------------------

TEST(ConflictResolverSkip, FirstOccurrenceIsImported) {
    json row1 = {{"id", 1}, {"name", "alice"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::SKIP;

    auto res = runImport({row1}, "users", opts);
    EXPECT_EQ(res.stats.imported_records, 1u);
    EXPECT_EQ(res.stats.conflicts_skipped, 0u);
    ASSERT_EQ(res.emitted.size(), 1u);
    EXPECT_EQ(res.emitted[0]["name"], "alice");
}

TEST(ConflictResolverSkip, DuplicateIsDiscarded) {
    json row1 = {{"id", 1}, {"name", "alice"}};
    json row2 = {{"id", 1}, {"name", "ALICE-updated"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::SKIP;

    auto res = runImport({row1, row2}, "users", opts);
    EXPECT_EQ(res.stats.imported_records, 1u);
    EXPECT_EQ(res.stats.conflicts_skipped, 1u);
    EXPECT_EQ(res.stats.skipped_records, 1u);
    ASSERT_EQ(res.emitted.size(), 1u);
    // First value should be kept
    EXPECT_EQ(res.emitted[0]["name"], "alice");
}

TEST(ConflictResolverSkip, MultipleDuplicatesAllDiscarded) {
    json row1 = {{"id", 7}, {"val", "first"}};
    json row2 = {{"id", 7}, {"val", "second"}};
    json row3 = {{"id", 7}, {"val", "third"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::SKIP;

    auto res = runImport({row1, row2, row3}, "items", opts);
    EXPECT_EQ(res.stats.imported_records, 1u);
    EXPECT_EQ(res.stats.conflicts_skipped, 2u);
    ASSERT_EQ(res.emitted.size(), 1u);
    EXPECT_EQ(res.emitted[0]["val"], "first");
}

TEST(ConflictResolverSkip, DifferentKeysAreNotConflicts) {
    json row1 = {{"id", 1}, {"name", "alice"}};
    json row2 = {{"id", 2}, {"name", "bob"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::SKIP;

    auto res = runImport({row1, row2}, "users", opts);
    EXPECT_EQ(res.stats.imported_records, 2u);
    EXPECT_EQ(res.stats.conflicts_skipped, 0u);
}

// ---------------------------------------------------------------------------
// 2. ConflictStrategy::OVERWRITE
// ---------------------------------------------------------------------------

TEST(ConflictResolverOverwrite, DuplicateReplacesOriginal) {
    json row1 = {{"id", 1}, {"name", "alice"}};
    json row2 = {{"id", 1}, {"name", "alice-v2"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::OVERWRITE;

    auto res = runImport({row1, row2}, "users", opts);
    EXPECT_EQ(res.stats.conflicts_overwritten, 1u);
    // Both are emitted (first + overwrite)
    ASSERT_EQ(res.emitted.size(), 2u);
    EXPECT_EQ(res.emitted[1]["name"], "alice-v2");
}

TEST(ConflictResolverOverwrite, IsDefaultStrategy) {
    ImportOptions opts;
    EXPECT_EQ(static_cast<int>(opts.conflict_strategy),
              static_cast<int>(ConflictStrategy::OVERWRITE));
}

TEST(ConflictResolverOverwrite, NoDuplicateNoConflictCounter) {
    json row1 = {{"id", 1}, {"v", "a"}};
    json row2 = {{"id", 2}, {"v", "b"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::OVERWRITE;

    auto res = runImport({row1, row2}, "t", opts);
    EXPECT_EQ(res.stats.conflicts_overwritten, 0u);
    EXPECT_EQ(res.stats.imported_records, 2u);
}

// ---------------------------------------------------------------------------
// 3. ConflictStrategy::MERGE – depth 1 (top-level fields only)
// ---------------------------------------------------------------------------

TEST(ConflictResolverMerge, TopLevelIncomingFieldsWin) {
    json existing = {{"id", 1}, {"name", "alice"}, {"age", 30}};
    json incoming = {{"id", 1}, {"age", 31}, {"city", "Berlin"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::MERGE;
    opts.merge_depth          = 1;

    auto res = runImport({existing, incoming}, "users", opts);
    ASSERT_EQ(res.emitted.size(), 2u);
    const json& merged = res.emitted[1];
    EXPECT_EQ(merged["name"], "alice");   // kept from existing
    EXPECT_EQ(merged["age"], 31);         // incoming wins
    EXPECT_EQ(merged["city"], "Berlin");  // new field from incoming
    EXPECT_EQ(res.stats.conflicts_merged, 1u);
}

TEST(ConflictResolverMerge, ProtectedFieldsArePreserved) {
    json existing = {{"id", 1}, {"created_at", "2024-01-01"}, {"val", "old"}};
    json incoming = {{"id", 1}, {"created_at", "2025-06-01"}, {"val", "new"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::MERGE;
    opts.protected_fields     = {"created_at"};
    opts.merge_depth          = 1;

    auto res = runImport({existing, incoming}, "records", opts);
    ASSERT_EQ(res.emitted.size(), 2u);
    const json& merged = res.emitted[1];
    EXPECT_EQ(merged["created_at"], "2024-01-01");  // protected → kept
    EXPECT_EQ(merged["val"], "new");                 // not protected → incoming wins
}

TEST(ConflictResolverMerge, DepthOneNestedObjectReplacedEntirely) {
    json existing = {{"id", 1}, {"meta", {{"k1", "v1"}, {"k2", "v2"}}}};
    json incoming = {{"id", 1}, {"meta", {{"k1", "CHANGED"}}}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::MERGE;
    opts.merge_depth          = 1;

    auto res = runImport({existing, incoming}, "t", opts);
    ASSERT_EQ(res.emitted.size(), 2u);
    // depth=1: nested object replaced entirely by incoming
    EXPECT_EQ(res.emitted[1]["meta"]["k1"], "CHANGED");
    EXPECT_FALSE(res.emitted[1]["meta"].contains("k2"));
}

// ---------------------------------------------------------------------------
// 4. ConflictStrategy::MERGE – deep merge (depth -1)
// ---------------------------------------------------------------------------

TEST(ConflictResolverMergeDeep, NestedObjectsMergedRecursively) {
    json existing = {{"id", 1}, {"meta", {{"k1", "v1"}, {"k2", "v2"}}}};
    json incoming = {{"id", 1}, {"meta", {{"k1", "CHANGED"}, {"k3", "new"}}}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::MERGE;
    opts.merge_depth          = -1;  // deep

    auto res = runImport({existing, incoming}, "t", opts);
    ASSERT_EQ(res.emitted.size(), 2u);
    const json& merged = res.emitted[1];
    EXPECT_EQ(merged["meta"]["k1"], "CHANGED");  // incoming wins
    EXPECT_EQ(merged["meta"]["k2"], "v2");        // kept from existing
    EXPECT_EQ(merged["meta"]["k3"], "new");       // added from incoming
}

TEST(ConflictResolverMergeDeep, DepthTwoLimitsRecursion) {
    // depth=2: merge at levels 0 (top) and 1 (nested), but not level 2
    json existing = {{"id", 1},
                     {"l1", {{"l2", {{"k", "old"}, {"keep", "yes"}}}}}};
    json incoming = {{"id", 1},
                     {"l1", {{"l2", {{"k", "new"}}}}}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::MERGE;
    opts.merge_depth          = 2;

    auto res = runImport({existing, incoming}, "t", opts);
    ASSERT_EQ(res.emitted.size(), 2u);
    const json& merged = res.emitted[1];
    // At depth=2, l1 is merged (depth→1), then l1.l2 is depth=1 so replaced entirely
    EXPECT_EQ(merged["l1"]["l2"]["k"], "new");
    // "keep" is gone because l2 was replaced at depth=1
    EXPECT_FALSE(merged["l1"]["l2"].contains("keep"));
}

// ---------------------------------------------------------------------------
// 5. ConflictStrategy::ERROR
// ---------------------------------------------------------------------------

TEST(ConflictResolverError, ConflictProducesStructuredError) {
    json row1 = {{"id", 1}, {"v", "a"}};
    json row2 = {{"id", 1}, {"v", "b"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::ERROR;
    opts.continue_on_error    = true;

    auto res = runImport({row1, row2}, "t", opts);
    EXPECT_EQ(res.stats.failed_records, 1u);
    ASSERT_FALSE(res.stats.structured_errors.empty());
    EXPECT_EQ(res.stats.structured_errors[0].code, ImportErrorCode::CONFLICT_ERROR);
}

TEST(ConflictResolverError, ContinueOnErrorFalseAbortsImport) {
    json row1 = {{"id", 1}, {"v", "a"}};
    json row2 = {{"id", 1}, {"v", "b"}};
    json row3 = {{"id", 2}, {"v", "c"}};  // would be imported if not aborted
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::ERROR;
    opts.continue_on_error    = false;

    auto res = runImport({row1, row2, row3}, "t", opts);
    // Import must abort after row2 conflict; row3 should not be imported
    EXPECT_EQ(res.stats.imported_records, 1u);
    EXPECT_EQ(res.stats.failed_records, 1u);
    ASSERT_EQ(res.emitted.size(), 1u);
}

TEST(ConflictResolverError, ContinueOnErrorTrueSkipsConflict) {
    json row1 = {{"id", 1}, {"v", "a"}};
    json row2 = {{"id", 1}, {"v", "b"}};
    json row3 = {{"id", 2}, {"v", "c"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::ERROR;
    opts.continue_on_error    = true;

    auto res = runImport({row1, row2, row3}, "t", opts);
    // row2 is a conflict → fails, row3 should be imported
    EXPECT_EQ(res.stats.imported_records, 2u);
    EXPECT_EQ(res.stats.failed_records, 1u);
}

// ---------------------------------------------------------------------------
// 6. Composite conflict keys
// ---------------------------------------------------------------------------

TEST(ConflictResolverCompositeKey, TwoColumnKey) {
    json r1 = {{"tenant", "acme"}, {"user_id", 1}, {"role", "admin"}};
    json r2 = {{"tenant", "acme"}, {"user_id", 1}, {"role", "editor"}};  // same key
    json r3 = {{"tenant", "acme"}, {"user_id", 2}, {"role", "viewer"}};  // different key
    ImportOptions opts;
    opts.conflict_key_columns = {"tenant", "user_id"};
    opts.conflict_strategy    = ConflictStrategy::SKIP;

    auto res = runImport({r1, r2, r3}, "memberships", opts);
    EXPECT_EQ(res.stats.imported_records, 2u);
    EXPECT_EQ(res.stats.conflicts_skipped, 1u);
}

TEST(ConflictResolverCompositeKey, KeysMustMatchExactly) {
    json r1 = {{"a", 1}, {"b", 1}};
    json r2 = {{"a", 1}, {"b", 2}};   // different b → different key
    json r3 = {{"a", 2}, {"b", 1}};   // different a → different key
    json r4 = {{"a", 1}, {"b", 1}};   // same as r1 → conflict
    ImportOptions opts;
    opts.conflict_key_columns = {"a", "b"};
    opts.conflict_strategy    = ConflictStrategy::SKIP;

    auto res = runImport({r1, r2, r3, r4}, "t", opts);
    EXPECT_EQ(res.stats.imported_records, 3u);
    EXPECT_EQ(res.stats.conflicts_skipped, 1u);
}

// ---------------------------------------------------------------------------
// 7. No conflict key → no conflict detection
// ---------------------------------------------------------------------------

TEST(ConflictResolverNoKey, AllRowsImported) {
    json r1 = {{"id", 1}};
    json r2 = {{"id", 1}};  // would be a conflict with key, but no key configured
    ImportOptions opts;
    // opts.conflict_key_columns is empty
    opts.conflict_strategy = ConflictStrategy::SKIP;

    auto res = runImport({r1, r2}, "t", opts);
    EXPECT_EQ(res.stats.imported_records, 2u);
    EXPECT_EQ(res.stats.conflicts_skipped, 0u);
}

// ---------------------------------------------------------------------------
// 8. Metrics emission
// ---------------------------------------------------------------------------

TEST(ConflictResolverMetrics, SkipEmitsConflictMetric) {
    json r1 = {{"id", 1}};
    json r2 = {{"id", 1}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::SKIP;

    bool metric_emitted = false;
    opts.metrics_callback = [&](const std::string& m,
                                const std::map<std::string,std::string>& labels,
                                double) {
        if (m == "importers_conflicts_total" &&
            labels.count("outcome") && labels.at("outcome") == "skipped") {
            metric_emitted = true;
        }
    };

    runImport({r1, r2}, "t", opts);
    EXPECT_TRUE(metric_emitted);
}

TEST(ConflictResolverMetrics, MergeEmitsConflictMetric) {
    json r1 = {{"id", 1}, {"x", "a"}};
    json r2 = {{"id", 1}, {"x", "b"}};
    ImportOptions opts;
    opts.conflict_key_columns = {"id"};
    opts.conflict_strategy    = ConflictStrategy::MERGE;

    std::string outcome_seen;
    opts.metrics_callback = [&](const std::string& m,
                                const std::map<std::string,std::string>& labels,
                                double) {
        if (m == "importers_conflicts_total" && labels.count("outcome")) {
            outcome_seen = labels.at("outcome");
        }
    };

    runImport({r1, r2}, "t", opts);
    EXPECT_EQ(outcome_seen, "merged");
}

// ---------------------------------------------------------------------------
// 9. ImportConflictResolver::computeKey
// ---------------------------------------------------------------------------

TEST(ConflictResolverComputeKey, EmptyColumnsReturnsEmpty) {
    json entity = {{"id", 1}, {"name", "x"}};
    EXPECT_TRUE(ImportConflictResolver::computeKey(entity, {}).empty());
}

TEST(ConflictResolverComputeKey, SingleColumn) {
    json entity = {{"id", 42}};
    EXPECT_EQ(ImportConflictResolver::computeKey(entity, {"id"}), "42");
}

TEST(ConflictResolverComputeKey, StringColumn) {
    json entity = {{"name", "alice"}};
    EXPECT_EQ(ImportConflictResolver::computeKey(entity, {"name"}), "alice");
}

TEST(ConflictResolverComputeKey, MissingColumnContributesEmpty) {
    json entity = {{"id", 1}};
    // "missing" not present → contributes empty string with separator
    std::string key = ImportConflictResolver::computeKey(entity, {"id", "missing"});
    EXPECT_FALSE(key.empty());
    EXPECT_NE(key, "1");  // there is a separator character after "1"
}

// ---------------------------------------------------------------------------
// 10. mergeEntities static helper
// ---------------------------------------------------------------------------

TEST(MergeEntitiesTest, NonObjectIncomingWins) {
    json existing = json::array({1, 2, 3});
    json incoming = json::array({4, 5});
    json result = ImportConflictResolver::mergeEntities(existing, incoming, 1, {});
    EXPECT_EQ(result, incoming);
}

TEST(MergeEntitiesTest, AllIncomingFieldsAddedWhenNoProtected) {
    json existing = {{"a", 1}};
    json incoming = {{"b", 2}, {"c", 3}};
    json result = ImportConflictResolver::mergeEntities(existing, incoming, 1, {});
    EXPECT_EQ(result["a"], 1);
    EXPECT_EQ(result["b"], 2);
    EXPECT_EQ(result["c"], 3);
}

TEST(MergeEntitiesTest, ProtectedFieldsAreSkipped) {
    json existing = {{"a", "keep"}, {"b", "keep-b"}};
    json incoming = {{"a", "OVERWRITE"}, {"b", "overwrite-b"}};
    json result = ImportConflictResolver::mergeEntities(existing, incoming, 1, {"a"});
    EXPECT_EQ(result["a"], "keep");
    EXPECT_EQ(result["b"], "overwrite-b");
}

TEST(MergeEntitiesTest, DeepMergePreservesExistingNestedKeys) {
    json existing = {{"meta", {{"created", "2024"}, {"owner", "bob"}}}};
    json incoming = {{"meta", {{"created", "2025"}}}};
    json result = ImportConflictResolver::mergeEntities(existing, incoming, -1, {});
    EXPECT_EQ(result["meta"]["created"], "2025");  // overwritten
    EXPECT_EQ(result["meta"]["owner"], "bob");     // preserved
}
