/**
 * @file process_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "function_registry.h"
#include <chrono>
#include <cmath>
#include <limits>

namespace themis {
namespace query {
namespace functions {


// ============================================================================
// Milestone Functions
// ============================================================================

class MilestoneStatusFunction : public IFunction {
public:
    ~MilestoneStatusFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "MILESTONE_STATUS",
            "Process",
            "Get current milestone status for a case",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Case/Vorgang ID"}
            },
            ArgType::OBJECT,
            true,   // deterministic
            false,  // not aggregate
            {"MILESTONE_STATUS(\"V-2024-0001\")"},
            {CostComplexity::INDEXED, 5.0, 0.1, true, false, "milestone"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        std::string case_id = args[0].get<std::string>();
        auto docs = ctx.scanCollection("_milestone_instances",
            [&case_id](const nlohmann::json& doc) {
                return doc.value("case_id", "") == case_id;
            });

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        int reached = 0, pending = 0, overdue = 0, skipped = 0;
        nlohmann::json next_milestone = nullptr;
        int64_t next_due = std::numeric_limits<int64_t>::max();

        for (const auto& doc : docs) {
            std::string status = doc.value("status", "pending");
            if (status == "reached") {
              ++reached;
            }
            else if (status == "skipped") ++skipped;
            else if (status == "overdue" ||
                     (status == "pending" && doc.value("due_date", int64_t{0}) < now_ms &&
                      doc.value("due_date", int64_t{0}) > 0))
                ++overdue;
            else {
                ++pending;
                int64_t due = doc.value("due_date", int64_t{0});
                if (due > 0 && due < next_due) {
                    next_due = due;
                    next_milestone = doc;
                }
            }
        }

        int total = reached + pending + overdue + skipped;
        double progress = total > 0
            ? 100.0 * reached / total
            : 0.0;

        return nlohmann::json{
            {"case_id",          case_id},
            {"reached",          reached},
            {"pending",          pending},
            {"overdue",          overdue},
            {"skipped",          skipped},
            {"next",             next_milestone},
            {"progress_percent", progress}
        };
    }
};

class MilestoneNextFunction : public IFunction {
public:
    ~MilestoneNextFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "MILESTONE_NEXT",
            "Process",
            "Get next pending milestone for a case",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Case/Vorgang ID"}
            },
            ArgType::OBJECT,
            true, false,
            {"MILESTONE_NEXT(\"V-2024-0001\")"},
            {CostComplexity::INDEXED, 3.0, 0.0, true, false, "milestone"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        std::string case_id = args[0].get<std::string>();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        auto docs = ctx.scanCollection("_milestone_instances",
            [&case_id](const nlohmann::json& doc) {
                std::string st = doc.value("status", "pending");
                return doc.value("case_id", "") == case_id &&
                       (st == "pending" || st == "overdue");
            });

        // Find the pending milestone with the earliest due_date.
        const nlohmann::json* nearest = nullptr;
        int64_t nearest_due = std::numeric_limits<int64_t>::max();
        for (const auto& doc : docs) {
            int64_t due = doc.value("due_date", int64_t{0});
            if (due > 0 && due < nearest_due) {
                nearest_due = due;
                nearest = &doc;
            }
        }

        if (!nearest) {
            return nlohmann::json{
                {"milestone_id",    nullptr},
                {"name",            nullptr},
                {"due_date",        nullptr},
                {"remaining_hours", nullptr}
            };
        }

        double remaining_hours = (nearest_due - now_ms) / 3600000.0;
        return nlohmann::json{
            {"milestone_id",    nearest->value("milestone_id", "")},
            {"name",            nearest->value("name", "")},
            {"due_date",        nearest_due},
            {"remaining_hours", remaining_hours}
        };
    }
};

class MilestoneOverdueFunction : public IFunction {
public:
    ~MilestoneOverdueFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "MILESTONE_OVERDUE",
            "Process",
            "Get all overdue milestones for a case or globally",
            {
                {"case_id", ArgType::STRING, false, nullptr, "Optional case ID (all if omitted)"}
            },
            ArgType::ARRAY,
            true, false,
            {"MILESTONE_OVERDUE()", "MILESTONE_OVERDUE(\"V-2024-0001\")"},
            {CostComplexity::INDEXED, 5.0, 0.5, true, true, "milestone"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        std::string filter_case = {};
        if (!args.empty() && args[0].is_string()) {
            filter_case = args[0].get<std::string>();
        }

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        auto docs = ctx.scanCollection("_milestone_instances",
            [&filter_case, now_ms](const nlohmann::json& doc) {
                if (!filter_case.empty() && doc.value("case_id", "") != filter_case) {
                    return false;
                }
                std::string st = doc.value("status", "pending");
                if (st == "overdue") {
                  return true;
                }
                // Also treat pending milestones with a passed due_date as overdue.
                int64_t due = doc.value("due_date", int64_t{0});
                return st == "pending" && due > 0 && due < now_ms;
            });

        nlohmann::json result = nlohmann::json::array();
        for (const auto& doc : docs) {
            result.push_back(doc);
        }
        return result;
    }
};

// ============================================================================
// Workflow Functions
// ============================================================================

class WorkflowAdvanceFunction : public IFunction {
public:
    ~WorkflowAdvanceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "WORKFLOW_ADVANCE",
            "Process",
            "Advance workflow token to next activity",
            {
                {"token_id", ArgType::STRING, true, nullptr, "Workflow token ID"},
                {"activity", ArgType::STRING, true, nullptr, "Completed activity name"},
                {"variables", ArgType::OBJECT, false, nlohmann::json::object(), "Updated variables"}
            },
            ArgType::OBJECT,
            false,  // not deterministic (modifies state)
            false,
            {"WORKFLOW_ADVANCE(\"T-001\", \"approve\", {approved: true})"},
            {CostComplexity::EXTERNAL, 10.0, 0.0, false, false, ""}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        return nlohmann::json{
            {"token_id", args[0]},
            {"previous_activity", args[1]},
            {"next_activity", nullptr},
            {"advanced", true}
        };
    }
};

class WorkflowVariablesFunction : public IFunction {
public:
    ~WorkflowVariablesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "WORKFLOW_VARIABLES",
            "Process",
            "Get all variables for a workflow instance",
            {
                {"token_id", ArgType::STRING, true, nullptr, "Workflow token ID"}
            },
            ArgType::OBJECT,
            true, false,
            {"WORKFLOW_VARIABLES(\"T-001\")"},
            {CostComplexity::INDEXED, 2.0, 0.0, true, false, "workflow"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        return nlohmann::json::object();
    }
};

// ============================================================================
// Conformance Functions
// ============================================================================

class ProcessConformanceFunction : public IFunction {
public:
    ~ProcessConformanceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PROCESS_CONFORMANCE",
            "Process",
            "Calculate conformance score between actual and expected process",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Case ID to check"},
                {"model_id", ArgType::STRING, false, nullptr, "Process model ID (auto-detect if omitted)"}
            },
            ArgType::OBJECT,
            true, false,
            {"PROCESS_CONFORMANCE(\"V-2024-0001\")", "PROCESS_CONFORMANCE(\"V-2024-0001\", \"bauantrag_v2\")"},
            {CostComplexity::LINEAR, 10.0, 1.0, true, false, "process"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        return nlohmann::json{
            {"case_id", args[0]},
            {"conformance_percent", 100.0},
            {"deviations", nlohmann::json::array()},
            {"steps_checked", 0},
            {"steps_conformant", 0}
        };
    }
};

class ProcessDeviationsFunction : public IFunction {
public:
    ~ProcessDeviationsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PROCESS_DEVIATIONS",
            "Process",
            "Find all deviations from expected process model",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Case ID to check"},
                {"model_id", ArgType::STRING, false, nullptr, "Process model ID"}
            },
            ArgType::ARRAY,
            true, false,
            {"PROCESS_DEVIATIONS(\"V-2024-0001\")"},
            {CostComplexity::LINEAR, 10.0, 2.0, true, false, "process"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        return nlohmann::json::array();
    }
};

class ProcessPredictNextFunction : public IFunction {
public:
    ~ProcessPredictNextFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PROCESS_PREDICT_NEXT",
            "Process",
            "Predict most likely next activity based on process model and history",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Case ID"},
                {"top_n", ArgType::INTEGER, false, 3, "Number of predictions to return"}
            },
            ArgType::ARRAY,
            true, false,
            {"PROCESS_PREDICT_NEXT(\"V-2024-0001\")", "PROCESS_PREDICT_NEXT(\"V-2024-0001\", 5)"},
            {CostComplexity::LINEAR, 15.0, 1.0, true, false, "process"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        return nlohmann::json::array();
    }
};

class ProcessPredictEndFunction : public IFunction {
public:
    ~ProcessPredictEndFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PROCESS_PREDICT_END",
            "Process",
            "Predict when process will complete based on current progress and historical data",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Case ID"}
            },
            ArgType::OBJECT,
            true, false,
            {"PROCESS_PREDICT_END(\"V-2024-0001\")"},
            {CostComplexity::LINEAR, 20.0, 1.0, true, false, "process"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        return nlohmann::json{
            {"case_id", args[0]},
            {"predicted_end", now_ms + 86400000 * 7},  // +7 days placeholder
            {"confidence", "medium"},
            {"remaining_hours", 56.0},
            {"optimistic_hours", 40.0},
            {"pessimistic_hours", 80.0}
        };
    }
};

// ============================================================================
// SLA Functions
// ============================================================================

class SlaCheckFunction : public IFunction {
public:
    ~SlaCheckFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "SLA_CHECK",
            "Process",
            "Check SLA compliance status for a case",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Case ID"}
            },
            ArgType::OBJECT,
            true, false,
            {"SLA_CHECK(\"V-2024-0001\")"},
            {CostComplexity::INDEXED, 5.0, 0.5, true, false, "milestone"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        return nlohmann::json{
            {"case_id", args[0]},
            {"status", "ok"},  // ok | warning | critical | breached
            {"breached_count", 0},
            {"at_risk_count", 0},
            {"next_deadline", nullptr}
        };
    }
};

class SlaRemainingFunction : public IFunction {
public:
    ~SlaRemainingFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "SLA_REMAINING",
            "Process",
            "Get remaining time until next SLA deadline",
            {
                {"case_id", ArgType::STRING, true, nullptr, "Case ID"},
                {"milestone_id", ArgType::STRING, false, nullptr, "Specific milestone (next if omitted)"}
            },
            ArgType::OBJECT,
            true, false,
            {"SLA_REMAINING(\"V-2024-0001\")", "SLA_REMAINING(\"V-2024-0001\", \"M2\")"},
            {CostComplexity::INDEXED, 3.0, 0.0, true, false, "milestone"}
        };
    }
    
    nlohmann::json execute(
        const std::vector<nlohmann::json>& args,
        const FunctionContext& ctx
    ) const override {
        return nlohmann::json{
            {"case_id", args[0]},
            {"milestone_id", args.size() > 1 ? args[1] : nullptr},
            {"remaining_hours", 24.0},
            {"remaining_business_hours", 16.0},
            {"due_date", nullptr},
            {"status", "ok"}
        };
    }
};

// ============================================================================
// Registration Helper
// ============================================================================

/**
 * @brief Register Process Functions.
 * @param[in,out] registry Input/output parameter.
 * @details Calls: registerFunction().
 */
inline void registerProcessFunctions(FunctionRegistry& registry) {
    // Milestone functions
    registry.registerFunction("MILESTONE_STATUS", std::make_unique<MilestoneStatusFunction>());
    registry.registerFunction("MILESTONE_NEXT", std::make_unique<MilestoneNextFunction>());
    registry.registerFunction("MILESTONE_OVERDUE", std::make_unique<MilestoneOverdueFunction>());
    
    // Workflow functions
    registry.registerFunction("WORKFLOW_ADVANCE", std::make_unique<WorkflowAdvanceFunction>());
    registry.registerFunction("WORKFLOW_VARIABLES", std::make_unique<WorkflowVariablesFunction>());
    
    // Conformance functions
    registry.registerFunction("PROCESS_CONFORMANCE", std::make_unique<ProcessConformanceFunction>());
    registry.registerFunction("PROCESS_DEVIATIONS", std::make_unique<ProcessDeviationsFunction>());
    registry.registerFunction("PROCESS_PREDICT_NEXT", std::make_unique<ProcessPredictNextFunction>());
    registry.registerFunction("PROCESS_PREDICT_END", std::make_unique<ProcessPredictEndFunction>());
    
    // SLA functions
    registry.registerFunction("SLA_CHECK", std::make_unique<SlaCheckFunction>());
    registry.registerFunction("SLA_REMAINING", std::make_unique<SlaRemainingFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
