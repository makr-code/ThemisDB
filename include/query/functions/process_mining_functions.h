/**
 * @file process_mining_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "function_registry.h"
#include "analytics/process_pattern_matcher.h"
#include "analytics/process_mining.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <mutex>

namespace themis {
namespace query {
namespace functions {

/**
 * @brief Process Mining Functions for AQL
 * 
 * This module provides AQL functions for process mining and pattern matching:
 * 
 * ## Discovery & Extraction
 * - PM_EXTRACT_LOG(collection, config) -> Extract event log from data
 * - PM_DISCOVER_PROCESS(log, algorithm) -> Discover process model
 * - PM_EXTRACT_TRACE(case_id) -> Get trace for specific case
 * 
 * ## Pattern Matching & Similarity
 * - PM_FIND_SIMILAR(pattern, config) -> Find similar processes
 * - PM_COMPARE_IDEAL(case_id, ideal_model) -> Compare with ideal
 * - PM_HAS_PATTERN(case_id, pattern, threshold) -> Check if matches pattern
 * 
 * ## Conformance & Analysis
 * - PM_CONFORMANCE(case_id, model_id) -> Calculate conformance
 * - PM_DEVIATIONS(case_id, model_id) -> Find deviations
 * - PM_VARIANTS(log, top_n) -> Analyze process variants
 * 
 * ## Performance & Bottlenecks
 * - PM_BOTTLENECKS(log, threshold) -> Detect bottlenecks
 * - PM_PREDICT_END(case_id) -> Predict process end time
 * - PM_PREDICT_NEXT(case_id, top_n) -> Predict next activities
 * 
 * ## Administrative Models
 * - PM_LOAD_ADMIN_MODEL(model_id) -> Load predefined admin process
 * - PM_LIST_ADMIN_MODELS() -> List available models
 * 
 * ## Export
 * - PM_EXPORT_BPMN(model) -> Export as BPMN 2.0
 * - PM_EXPORT_PETRI_NET(model) -> Export as Petri Net (PNML)
 */

// ============================================================================
// Pattern Matching Functions (NEW)
// ============================================================================

/**
 * @brief PM_FIND_SIMILAR - Find processes similar to a pattern
 * 
 * @code
 * -- Define ideal process pattern
 * LET ideal = {
 *   activities: ["Antrag", "Prüfung", "Genehmigung"],
 *   edges: [
 *     {from: "Antrag", to: "Prüfung"},
 *     {from: "Prüfung", to: "Genehmigung"}
 *   ]
 * }
 * 
 * -- Find similar processes
 * LET similar = PM_FIND_SIMILAR(ideal, {
 *   method: "hybrid",
 *   threshold: 0.7,
 *   limit: 10
 * })
 * 
 * RETURN similar
 * @endcode
 */
class PmFindSimilarFunction : public IFunction {
public:
    ~PmFindSimilarFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_FIND_SIMILAR",
            "ProcessMining",
            "Find processes similar to a given pattern using graph, vector, or hybrid similarity",
            {
                {"pattern", ArgType::OBJECT, true, nullptr, 
                 "Process pattern with activities and edges"},
                {"config", ArgType::OBJECT, false, 
                 nlohmann::json{{"method", "hybrid"}, {"threshold", 0.7}, {"limit", 10}},
                 "Configuration: method (graph|vector|behavioral|hybrid), threshold, limit"}
            },
            ArgType::ARRAY,
            true,   // deterministic
            false,  // not aggregate
            {
                "PM_FIND_SIMILAR({activities: ['A', 'B', 'C']}, {method: 'graph', threshold: 0.8})",
                "PM_FIND_SIMILAR(ideal_pattern, {method: 'hybrid', limit: 20})"
            },
            {CostComplexity::LINEAR, 50.0, 10.0, true, true, "process_patterns"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief PM_COMPARE_IDEAL - Compare process with ideal model
 * 
 * @code
 * -- Load ideal model
 * LET ideal = PM_LOAD_ADMIN_MODEL("bauantrag_standard")
 * 
 * -- Compare actual case with ideal
 * LET comparison = PM_COMPARE_IDEAL("V-2024-0001", ideal)
 * 
 * RETURN {
 *   case_id: "V-2024-0001",
 *   fitness: comparison.fitness,
 *   precision: comparison.precision,
 *   deviations: comparison.deviations
 * }
 * @endcode
 */
class PmCompareIdealFunction : public IFunction {
public:
    ~PmCompareIdealFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_COMPARE_IDEAL",
            "ProcessMining",
            "Compare a process instance with an ideal/expected model",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Process case ID to compare"},
                {"ideal_model", ArgType::OBJECT, true, nullptr, 
                 "Ideal process pattern or model ID"}
            },
            ArgType::OBJECT,
            true, false,
            {
                "PM_COMPARE_IDEAL('V-2024-0001', ideal_pattern)",
                "PM_COMPARE_IDEAL(case.id, PM_LOAD_ADMIN_MODEL('bauantrag_standard'))"
            },
            {CostComplexity::LINEAR, 20.0, 5.0, true, false, "process"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief PM_HAS_PATTERN - Check if process matches a pattern
 * 
 * @code
 * FOR case IN process_instances
 *   FILTER PM_HAS_PATTERN(case.id, {
 *     activities: ["Genehmigung", "Ablehnung"]
 *   }, 0.8)
 *   RETURN case
 * @endcode
 */
class PmHasPatternFunction : public IFunction {
public:
    ~PmHasPatternFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_HAS_PATTERN",
            "ProcessMining",
            "Check if a process instance matches a specific pattern",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Process case ID"},
                {"pattern", ArgType::OBJECT, true, nullptr, "Pattern to check"},
                {"threshold", ArgType::NUMBER, false, 0.8, "Minimum similarity threshold (0-1)"}
            },
            ArgType::BOOLEAN,
            true, false,
            {
                "PM_HAS_PATTERN('V-001', {activities: ['A', 'B']})",
                "PM_HAS_PATTERN(case.id, pattern, 0.9)"
            },
            {CostComplexity::INDEXED, 10.0, 2.0, true, false, "process"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Event Log Extraction
// ============================================================================

/**
 * @brief PM_EXTRACT_LOG - Extract event log from collection
 * 
 * @code
 * LET log = PM_EXTRACT_LOG("audit_log", {
 *   case_id_field: "order_id",
 *   activity_field: "action",
 *   timestamp_field: "timestamp"
 * })
 * 
 * RETURN log
 * @endcode
 */
class PmExtractLogFunction : public IFunction {
public:
    ~PmExtractLogFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_EXTRACT_LOG",
            "ProcessMining",
            "Extract event log from a collection for process mining",
            {
                {"collection", ArgType::STRING, true, nullptr, "Collection name"},
                {"config", ArgType::OBJECT, true, nullptr,
                 "Configuration with case_id_field, activity_field, timestamp_field"}
            },
            ArgType::OBJECT,
            true, false,
            {
                "PM_EXTRACT_LOG('audit', {case_id_field: 'order_id', activity_field: 'action', timestamp_field: 'ts'})"
            },
            {CostComplexity::EXTERNAL, 100.0, 50.0, false, true, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief PM_EXTRACT_TRACE - Extract trace for a specific case
 * 
 * @code
 * LET trace = PM_EXTRACT_TRACE("V-2024-0001")
 * RETURN trace.activities
 * @endcode
 */
class PmExtractTraceFunction : public IFunction {
public:
    ~PmExtractTraceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_EXTRACT_TRACE",
            "ProcessMining",
            "Extract the execution trace (activity sequence) for a specific case",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Process case ID"}
            },
            ArgType::OBJECT,
            true, false,
            {"PM_EXTRACT_TRACE('V-2024-0001')"},
            {CostComplexity::INDEXED, 5.0, 1.0, true, false, "process_traces"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Process Discovery
// ============================================================================

/**
 * @brief PM_DISCOVER_PROCESS - Discover process model from log
 * 
 * @code
 * LET log = PM_EXTRACT_LOG("audit", config)
 * LET model = PM_DISCOVER_PROCESS(log, {
 *   algorithm: "heuristic",
 *   dependency_threshold: 0.9
 * })
 * RETURN model
 * @endcode
 */
class PmDiscoverProcessFunction : public IFunction {
public:
    ~PmDiscoverProcessFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_DISCOVER_PROCESS",
            "ProcessMining",
            "Discover process model from event log using mining algorithms",
            {
                {"event_log", ArgType::OBJECT, true, nullptr, "Event log object"},
                {"config", ArgType::OBJECT, false,
                 nlohmann::json{{"algorithm", "heuristic"}},
                 "Mining configuration: algorithm (alpha|heuristic|inductive), thresholds"}
            },
            ArgType::OBJECT,
            true, false,
            {
                "PM_DISCOVER_PROCESS(log, {algorithm: 'alpha'})",
                "PM_DISCOVER_PROCESS(log, {algorithm: 'heuristic', dependency_threshold: 0.9})"
            },
            {CostComplexity::LINEAR, 100.0, 20.0, true, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Variants Analysis
// ============================================================================

/**
 * @brief PM_VARIANTS - Analyze process variants
 * 
 * @code
 * LET log = PM_EXTRACT_LOG("orders", config)
 * LET variants = PM_VARIANTS(log, 20)
 * 
 * FOR v IN variants
 *   RETURN {
 *     variant_id: v.variant_id,
 *     frequency: v.frequency,
 *     percentage: v.percentage,
 *     activities: v.activities
 *   }
 * @endcode
 */
class PmVariantsFunction : public IFunction {
public:
    ~PmVariantsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_VARIANTS",
            "ProcessMining",
            "Analyze and return top process variants from event log",
            {
                {"event_log", ArgType::OBJECT, true, nullptr, "Event log object"},
                {"top_n", ArgType::INTEGER, false, 20, "Number of top variants to return"}
            },
            ArgType::ARRAY,
            true, false,
            {"PM_VARIANTS(log)", "PM_VARIANTS(log, 10)"},
            {CostComplexity::LINEAR, 30.0, 5.0, true, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Administrative Model Functions (NEW)
// ============================================================================

/**
 * @brief PM_LOAD_ADMIN_MODEL - Load predefined administrative model
 *
 * Models are resolved from the FunctionContext variable `pm_admin_models`,
 * expected as an array of objects with at least an `"id"` field.
 * 
 * @code
 * -- Load building permit model
 * LET model = PM_LOAD_ADMIN_MODEL("bauantrag_standard")
 * 
 * -- Use for comparison
 * FOR case IN bauantraege
 *   LET comparison = PM_COMPARE_IDEAL(case.id, model)
 *   FILTER comparison.fitness < 0.9
 *   RETURN {case: case.id, fitness: comparison.fitness}
 * @endcode
 */
class PmLoadAdminModelFunction : public IFunction {
public:
    ~PmLoadAdminModelFunction() override = default;
    /**
     * @brief Injectable bridge for loading a YAML-backed administrative process model.
     *
     * @param model_id  The model identifier passed to PM_LOAD_ADMIN_MODEL.
     * @return JSON object representing the loaded model, or an error descriptor.
     *
     * Wire the YAML-backed model registry through setAdminModelLoadFn() to enable
     * in-database administrative model lifecycle.
     */
    using AdminModelLoadFn = std::function<nlohmann::json(const std::string& model_id)>;

    /**
     * @brief Install a model-load bridge function (thread-safe, process-wide).
     * @param fn  Callable invoked for each PM_LOAD_ADMIN_MODEL call.  Pass nullptr
     *            to revert to the not-implemented error response.
     */
    static void setAdminModelLoadFn(AdminModelLoadFn fn);

    FunctionSignature signature() const override {
        return {
            "PM_LOAD_ADMIN_MODEL",
            "ProcessMining",
            "Load a predefined administrative process model (Bauantrag, Beschaffung, HR, etc.)",
            {
                {"model_id", ArgType::STRING, true, nullptr,
                 "Model ID: bauantrag_standard, beschaffung_vergaberecht, personal_einstellung, etc."}
            },
            ArgType::OBJECT,
            true, false,
            {
                "PM_LOAD_ADMIN_MODEL('bauantrag_standard')",
                "PM_LOAD_ADMIN_MODEL('beschaffung_vergaberecht')"
            },
            {CostComplexity::INDEXED, 2.0, 0.0, true, false, "admin_models"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;

private:
    static AdminModelLoadFn admin_model_load_fn_;
    static std::mutex       admin_model_load_fn_mutex_;
};

/**
 * @brief PM_LIST_ADMIN_MODELS - List available administrative models
 *
 * Returns the normalized `pm_admin_models` array from FunctionContext.
 * 
 * @code
 * LET models = PM_LIST_ADMIN_MODELS()
 * FOR m IN models
 *   RETURN {id: m.id, name: m.name, domain: m.domain}
 * @endcode
 */
class PmListAdminModelsFunction : public IFunction {
public:
    ~PmListAdminModelsFunction() override = default;
    /**
     * @brief Injectable bridge for enumerating available administrative process models.
     *
     * @return JSON array of model descriptors (id, name, domain, …).
     *
     * Wire the YAML-backed model registry through setAdminModelListFn() to expose
     * all available models to AQL callers.
     */
    using AdminModelListFn = std::function<nlohmann::json()>;

    /**
     * @brief Install a model-list bridge function (thread-safe, process-wide).
     * @param fn  Callable invoked for each PM_LIST_ADMIN_MODELS call.  Pass nullptr
     *            to revert to the empty-array response.
     */
    static void setAdminModelListFn(AdminModelListFn fn);

    FunctionSignature signature() const override {
        return {
            "PM_LIST_ADMIN_MODELS",
            "ProcessMining",
            "List all available predefined administrative process models",
            {},
            ArgType::ARRAY,
            true, false,
            {"PM_LIST_ADMIN_MODELS()"},
            {CostComplexity::CONSTANT, 1.0, 0.0, true, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;

private:
    static AdminModelListFn admin_model_list_fn_;
    static std::mutex       admin_model_list_fn_mutex_;
};

// ============================================================================
// Conformance & Deviations
// ============================================================================

/**
 * @brief PM_CONFORMANCE - Calculate conformance score
 * 
 * Uses existing PROCESS_CONFORMANCE function but with simplified interface
 */
class PmConformanceFunction : public IFunction {
public:
    ~PmConformanceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_CONFORMANCE",
            "ProcessMining",
            "Calculate conformance score between actual process and model",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Process case ID"},
                {"model", ArgType::OBJECT, true, nullptr, "Process model or model ID"}
            },
            ArgType::OBJECT,
            true, false,
            {
                "PM_CONFORMANCE('V-001', model)",
                "PM_CONFORMANCE(case.id, PM_LOAD_ADMIN_MODEL('bauantrag_standard'))"
            },
            {CostComplexity::LINEAR, 15.0, 3.0, true, false, "process"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief PM_DEVIATIONS - Find process deviations
 */
class PmDeviationsFunction : public IFunction {
public:
    ~PmDeviationsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_DEVIATIONS",
            "ProcessMining",
            "Find all deviations of a process from expected model",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Process case ID"},
                {"model", ArgType::OBJECT, true, nullptr, "Process model or model ID"}
            },
            ArgType::ARRAY,
            true, false,
            {"PM_DEVIATIONS('V-001', model)"},
            {CostComplexity::LINEAR, 15.0, 5.0, true, false, "process"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Performance Analysis
// ============================================================================

/**
 * @brief PM_BOTTLENECKS - Detect process bottlenecks
 */
class PmBottlenecksFunction : public IFunction {
public:
    ~PmBottlenecksFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_BOTTLENECKS",
            "ProcessMining",
            "Detect bottlenecks in process based on performance data",
            {
                {"event_log", ArgType::OBJECT, true, nullptr, "Event log with timing data"},
                {"threshold_percentile", ArgType::NUMBER, false, 0.9,
                 "Percentile threshold for bottleneck detection (0-1)"}
            },
            ArgType::ARRAY,
            true, false,
            {"PM_BOTTLENECKS(log)", "PM_BOTTLENECKS(log, 0.95)"},
            {CostComplexity::LINEAR, 40.0, 10.0, true, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

/**
 * @brief PM_PREDICT_END - Predict process end time
 *
 * Reads predictions from FunctionContext variable `pm_predicted_end_by_case`,
 * expected as an object map `case_id -> predicted_end` value.
 *
 * @param case_id  Process case ID whose completion time should eventually be
 *                 forecast.
 * @return JSON object with a `predicted_end` field. The field is currently
 *         `null` when no prediction is available in the context map.
 */
class PmPredictEndFunction : public IFunction {
public:
    ~PmPredictEndFunction() override = default;
    /**
     * @brief Inject a process-end prediction backend.
     *
     * When set, `execute()` delegates to the provider to produce a real
     * forecast timestamp for the given case_id.  The provider receives the
     * case_id string and must return a JSON object with at least a
     * `"predicted_end"` field (ISO-8601 string or null on failure).
     *
     * @param fn  Provider callable, or `nullptr` to revert to null-placeholder.
     */
    using PredictEndFn = std::function<nlohmann::json(const std::string& case_id)>;
    static void setPredictEndFn(PredictEndFn fn);
    static void clearPredictEndFn();

    FunctionSignature signature() const override {
        return {
            "PM_PREDICT_END",
            "ProcessMining",
            "Predict when a running process will complete",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Process case ID"}
            },
            ArgType::OBJECT,
            true, false,
            {"PM_PREDICT_END('V-2024-0001')"},
            {CostComplexity::LINEAR, 25.0, 2.0, true, false, "process"}
        };
    }

    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;

private:
    static PredictEndFn predict_end_fn_;
    static std::mutex   predict_end_fn_mutex_;
};

// ============================================================================
// Export Functions
// ============================================================================

/**
 * @brief PM_EXPORT_BPMN - Export process model as BPMN 2.0
 */
class PmExportBpmnFunction : public IFunction {
public:
    ~PmExportBpmnFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PM_EXPORT_BPMN",
            "ProcessMining",
            "Export process model as BPMN 2.0 XML",
            {
                {"model", ArgType::OBJECT, true, nullptr, "Process model to export"}
            },
            ArgType::STRING,
            true, false,
            {"PM_EXPORT_BPMN(discovered_model)"},
            {CostComplexity::LINEAR, 10.0, 1.0, true, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override;
};

// ============================================================================
// Registration Helper
// ============================================================================

/**
 * @brief Register all process mining functions with the registry
 */
inline void registerProcessMiningFunctions(FunctionRegistry& registry) {
    // Pattern matching (NEW)
    registry.registerFunction(std::make_unique<PmFindSimilarFunction>());
    registry.registerFunction(std::make_unique<PmCompareIdealFunction>());
    registry.registerFunction(std::make_unique<PmHasPatternFunction>());
    
    // Event log extraction
    registry.registerFunction(std::make_unique<PmExtractLogFunction>());
    registry.registerFunction(std::make_unique<PmExtractTraceFunction>());
    
    // Process discovery
    registry.registerFunction(std::make_unique<PmDiscoverProcessFunction>());
    registry.registerFunction(std::make_unique<PmVariantsFunction>());
    
    // Administrative models (NEW)
    registry.registerFunction(std::make_unique<PmLoadAdminModelFunction>());
    registry.registerFunction(std::make_unique<PmListAdminModelsFunction>());
    
    // Conformance & deviations
    registry.registerFunction(std::make_unique<PmConformanceFunction>());
    registry.registerFunction(std::make_unique<PmDeviationsFunction>());
    
    // Performance analysis
    registry.registerFunction(std::make_unique<PmBottlenecksFunction>());
    registry.registerFunction(std::make_unique<PmPredictEndFunction>());
    
    // Export
    registry.registerFunction(std::make_unique<PmExportBpmnFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
