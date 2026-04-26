/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            execution_plan.cpp                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     240                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "execution_plan.h"
#include <stdexcept>

namespace themis {
namespace llm_translator {

// Helper to convert PlanValue to JSON
static nlohmann::json planValueToJson(const PlanValue& value) {
    return std::visit([](auto&& arg) -> nlohmann::json {
        return arg;
    }, value);
}

// Helper to convert JSON to PlanValue
static PlanValue jsonToPlanValue(const nlohmann::json& j) {
    if (j.is_string()) {
        return j.get<std::string>();
    } else if (j.is_number_integer()) {
        return j.get<int64_t>();
    } else if (j.is_number_float()) {
        return j.get<double>();
    } else if (j.is_boolean()) {
        return j.get<bool>();
    } else if (j.is_array()) {
        return j.get<std::vector<std::string>>();
    }
    return std::string("");
}

nlohmann::json FilterCondition::toJson() const {
    return {
        {"field", field},
        {"op", op},
        {"value", planValueToJson(value)}
    };
}

FilterCondition FilterCondition::fromJson(const nlohmann::json& j) {
    FilterCondition filter;
    filter.field = j["field"].get<std::string>();
    filter.op = j["op"].get<std::string>();
    filter.value = jsonToPlanValue(j["value"]);
    return filter;
}

nlohmann::json Aggregation::toJson() const {
    return {
        {"function", function},
        {"field", field},
        {"alias", alias}
    };
}

Aggregation Aggregation::fromJson(const nlohmann::json& j) {
    Aggregation agg;
    agg.function = j["function"].get<std::string>();
    agg.field = j["field"].get<std::string>();
    agg.alias = j.value("alias", "");
    return agg;
}

nlohmann::json ExecutionPlan::toJson() const {
    nlohmann::json j = {
        {"operation", static_cast<int>(operation)},
        {"datasource", datasource},
        {"fields", fields},
        {"group_by", group_by}
    };
    
    // Filters
    nlohmann::json filters_json = nlohmann::json::array();
    for (const auto& filter : filters) {
        filters_json.push_back(filter.toJson());
    }
    j["filters"] = filters_json;
    
    // Aggregations
    nlohmann::json aggs_json = nlohmann::json::array();
    for (const auto& agg : aggregations) {
        aggs_json.push_back(agg.toJson());
    }
    j["aggregations"] = aggs_json;
    
    // Parameters
    nlohmann::json params_json;
    for (const auto& [key, value] : parameters) {
        params_json[key] = planValueToJson(value);
    }
    j["parameters"] = params_json;
    
    // Metadata
    j["metadata"] = {
        {"original_prompt", original_prompt},
        {"confidence_score", confidence_score},
        {"llm_model_used", llm_model_used},
        {"generation_time_ms", generation_time_ms}
    };
    
    return j;
}

ExecutionPlan ExecutionPlan::fromJson(const nlohmann::json& j) {
    ExecutionPlan plan;
    
    plan.operation = static_cast<OperationType>(j["operation"].get<int>());
    plan.datasource = j["datasource"].get<std::string>();
    plan.fields = j.value("fields", std::vector<std::string>());
    plan.group_by = j.value("group_by", std::vector<std::string>());
    
    // Filters
    if (j.contains("filters")) {
        for (const auto& filter_json : j["filters"]) {
            plan.filters.push_back(FilterCondition::fromJson(filter_json));
        }
    }
    
    // Aggregations
    if (j.contains("aggregations")) {
        for (const auto& agg_json : j["aggregations"]) {
            plan.aggregations.push_back(Aggregation::fromJson(agg_json));
        }
    }
    
    // Parameters
    if (j.contains("parameters")) {
        for (const auto& [key, value] : j["parameters"].items()) {
            plan.parameters[key] = jsonToPlanValue(value);
        }
    }
    
    // Metadata
    if (j.contains("metadata")) {
        const auto& meta = j["metadata"];
        plan.original_prompt = meta.value("original_prompt", "");
        plan.confidence_score = meta.value("confidence_score", 0.0);
        plan.llm_model_used = meta.value("llm_model_used", "");
        plan.generation_time_ms = meta.value("generation_time_ms", 0);
    }
    
    return plan;
}

bool ExecutionPlan::validate(std::string* error_message) const {
    // Basic validation
    if (datasource.empty()) {
        if (error_message) {
            *error_message = "Datasource cannot be empty";
        }
        return false;
    }
    
    // Validate filters
    for (const auto& filter : filters) {
        if (filter.field.empty()) {
            if (error_message) {
                *error_message = "Filter field cannot be empty";
            }
            return false;
        }
        
        // Validate operator
        static const std::vector<std::string> valid_ops = {
            "=", ">", "<", ">=", "<=", "!=", "IN", "NOT IN", 
            "LIKE", "NOT LIKE", "IS NULL", "IS NOT NULL"
        };
        
        bool valid_op = false;
        for (const auto& op : valid_ops) {
            if (filter.op == op) {
                valid_op = true;
                break;
            }
        }
        
        if (!valid_op) {
            if (error_message) {
                *error_message = "Invalid operator: " + filter.op;
            }
            return false;
        }
    }
    
    // Validate aggregations for AGGREGATE operations
    if (operation == OperationType::AGGREGATE) {
        if (aggregations.empty()) {
            if (error_message) {
                *error_message = "AGGREGATE operation requires at least one aggregation";
            }
            return false;
        }
        
        static const std::vector<std::string> valid_funcs = {
            "COUNT", "SUM", "AVG", "MIN", "MAX", "STDDEV", "VARIANCE"
        };
        
        for (const auto& agg : aggregations) {
            bool valid_func = false;
            for (const auto& func : valid_funcs) {
                if (agg.function == func) {
                    valid_func = true;
                    break;
                }
            }
            
            if (!valid_func) {
                if (error_message) {
                    *error_message = "Invalid aggregation function: " + agg.function;
                }
                return false;
            }
        }
    }
    
    return true;
}

} // namespace llm_translator
} // namespace themis
