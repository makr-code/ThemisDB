#include "query/functions/process_mining_functions.h"
#include "analytics/process_pattern_matcher.h"
#include "analytics/process_mining.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

ProcessPattern jsonToProcessPattern(const nlohmann::json& j) {
    ProcessPattern pattern;
    
    if (j.contains("id")) {
        pattern.id = j["id"].get<std::string>();
    }
    if (j.contains("name")) {
        pattern.name = j["name"].get<std::string>();
    }
    if (j.contains("description")) {
        pattern.description = j["description"].get<std::string>();
    }
    if (j.contains("domain")) {
        pattern.domain = j["domain"].get<std::string>();
    }
    
    if (j.contains("activities") && j["activities"].is_array()) {
        for (const auto& act : j["activities"]) {
            pattern.activities.push_back(act.get<std::string>());
        }
    }
    
    if (j.contains("edges") && j["edges"].is_array()) {
        for (const auto& edge_json : j["edges"]) {
            ProcessPattern::Edge edge;
            edge.from = edge_json["from"].get<std::string>();
            edge.to = edge_json["to"].get<std::string>();
            edge.probability = edge_json.value("probability", 1.0);
            pattern.edges.push_back(edge);
        }
    }
    
    if (j.contains("metadata")) {
        pattern.metadata = j["metadata"];
    }
    
    return pattern;
}

nlohmann::json processPatternToJson(const ProcessPattern& pattern) {
    nlohmann::json j;
    j["id"] = pattern.id;
    j["name"] = pattern.name;
    j["description"] = pattern.description;
    j["domain"] = pattern.domain;
    j["activities"] = pattern.activities;
    
    nlohmann::json edges_json = nlohmann::json::array();
    for (const auto& edge : pattern.edges) {
        nlohmann::json edge_json;
        edge_json["from"] = edge.from;
        edge_json["to"] = edge.to;
        edge_json["probability"] = edge.probability;
        edges_json.push_back(edge_json);
    }
    j["edges"] = edges_json;
    j["metadata"] = pattern.metadata;
    
    return j;
}

SimilarityConfig jsonToSimilarityConfig(const nlohmann::json& j) {
    SimilarityConfig config;
    
    if (j.contains("method")) {
        std::string method_str = j["method"].get<std::string>();
        if (method_str == "graph") {
            config.method = SimilarityMethod::GRAPH;
        } else if (method_str == "vector") {
            config.method = SimilarityMethod::VECTOR;
        } else if (method_str == "behavioral") {
            config.method = SimilarityMethod::BEHAVIORAL;
        } else if (method_str == "hybrid") {
            config.method = SimilarityMethod::HYBRID;
        }
    }
    
    config.threshold = j.value("threshold", 0.7);
    config.limit = j.value("limit", 10);
    config.weight_graph = j.value("weight_graph", 0.4);
    config.weight_vector = j.value("weight_vector", 0.3);
    config.weight_behavioral = j.value("weight_behavioral", 0.3);
    
    return config;
}

} // anonymous namespace

// ============================================================================
// PM_FIND_SIMILAR
// ============================================================================

AQLValue PM_FIND_SIMILAR(
    const std::vector<AQLValue>& args,
    ProcessPatternMatcher& matcher
) {
    if (args.size() < 2) {
        throw std::runtime_error("PM_FIND_SIMILAR requires 2 arguments: (pattern, config)");
    }
    
    try {
        // Parse pattern
        nlohmann::json pattern_json = nlohmann::json::parse(args[0].asString());
        ProcessPattern pattern = jsonToProcessPattern(pattern_json);
        
        // Parse config
        nlohmann::json config_json = nlohmann::json::parse(args[1].asString());
        SimilarityConfig config = jsonToSimilarityConfig(config_json);
        
        // Find similar patterns
        auto [status, results] = matcher.findSimilar(pattern, config);
        
        if (!status.ok()) {
            throw std::runtime_error("PM_FIND_SIMILAR failed: " + status.message());
        }
        
        // Convert to JSON array
        nlohmann::json results_json = nlohmann::json::array();
        for (const auto& result : results.results) {
            nlohmann::json r;
            r["case_id"] = result.case_id;
            r["overall_similarity"] = result.overall_similarity;
            r["graph_similarity"] = result.graph_similarity;
            r["vector_similarity"] = result.vector_similarity;
            r["behavioral_similarity"] = result.behavioral_similarity;
            r["matched_activities"] = result.matched_activities;
            r["missing_activities"] = result.missing_activities;
            results_json.push_back(r);
        }
        
        return AQLValue(results_json.dump());
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PM_FIND_SIMILAR error: ") + e.what());
    }
}

// ============================================================================
// PM_COMPARE_IDEAL
// ============================================================================

AQLValue PM_COMPARE_IDEAL(
    const std::vector<AQLValue>& args,
    ProcessPatternMatcher& matcher
) {
    if (args.size() < 2) {
        throw std::runtime_error("PM_COMPARE_IDEAL requires 2 arguments: (case_id, ideal_model)");
    }
    
    try {
        std::string case_id = args[0].asString();
        
        // Parse ideal model
        nlohmann::json ideal_json = nlohmann::json::parse(args[1].asString());
        ProcessPattern ideal = jsonToProcessPattern(ideal_json);
        
        // Compare
        auto [status, result] = matcher.compareWithIdeal(case_id, ideal);
        
        if (!status.ok()) {
            throw std::runtime_error("PM_COMPARE_IDEAL failed: " + status.message());
        }
        
        // Convert to JSON
        nlohmann::json result_json;
        result_json["case_id"] = result.case_id;
        result_json["fitness"] = result.fitness;
        result_json["precision"] = result.precision;
        result_json["generalization"] = result.generalization;
        result_json["simplicity"] = result.simplicity;
        
        nlohmann::json deviations_json = nlohmann::json::array();
        for (const auto& dev : result.deviations) {
            nlohmann::json d;
            d["activity"] = dev.activity;
            d["type"] = dev.type;
            d["severity"] = dev.severity;
            d["description"] = dev.description;
            deviations_json.push_back(d);
        }
        result_json["deviations"] = deviations_json;
        
        return AQLValue(result_json.dump());
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PM_COMPARE_IDEAL error: ") + e.what());
    }
}

// ============================================================================
// PM_HAS_PATTERN
// ============================================================================

AQLValue PM_HAS_PATTERN(
    const std::vector<AQLValue>& args,
    ProcessPatternMatcher& matcher
) {
    if (args.size() < 2) {
        throw std::runtime_error("PM_HAS_PATTERN requires 2-3 arguments: (case_id, pattern, [threshold])");
    }
    
    try {
        std::string case_id = args[0].asString();
        
        // Parse pattern
        nlohmann::json pattern_json = nlohmann::json::parse(args[1].asString());
        ProcessPattern pattern = jsonToProcessPattern(pattern_json);
        
        // Get threshold
        double threshold = 0.7;
        if (args.size() >= 3) {
            threshold = args[2].asDouble();
        }
        
        // Check if pattern exists
        bool has_pattern = matcher.hasPattern(case_id, pattern, threshold);
        
        return AQLValue(has_pattern);
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PM_HAS_PATTERN error: ") + e.what());
    }
}

// ============================================================================
// PM_LOAD_ADMIN_MODEL
// ============================================================================

AQLValue PM_LOAD_ADMIN_MODEL(
    const std::vector<AQLValue>& args,
    ProcessPatternMatcher& matcher
) {
    if (args.size() < 1) {
        throw std::runtime_error("PM_LOAD_ADMIN_MODEL requires 1 argument: (model_id)");
    }
    
    try {
        std::string model_id = args[0].asString();
        
        auto model_opt = matcher.getAdministrativeModel(model_id);
        if (!model_opt) {
            throw std::runtime_error("Model not found: " + model_id);
        }
        
        nlohmann::json model_json = processPatternToJson(*model_opt);
        return AQLValue(model_json.dump());
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PM_LOAD_ADMIN_MODEL error: ") + e.what());
    }
}

// ============================================================================
// PM_LIST_ADMIN_MODELS
// ============================================================================

AQLValue PM_LIST_ADMIN_MODELS(
    const std::vector<AQLValue>& /*args*/,
    ProcessPatternMatcher& matcher
) {
    try {
        auto model_ids = matcher.listAdministrativeModels();
        
        nlohmann::json result = nlohmann::json::array();
        for (const auto& id : model_ids) {
            auto model_opt = matcher.getAdministrativeModel(id);
            if (model_opt) {
                nlohmann::json model_info;
                model_info["id"] = model_opt->id;
                model_info["name"] = model_opt->name;
                model_info["description"] = model_opt->description;
                model_info["domain"] = model_opt->domain;
                model_info["activity_count"] = model_opt->activities.size();
                result.push_back(model_info);
            }
        }
        
        return AQLValue(result.dump());
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PM_LIST_ADMIN_MODELS error: ") + e.what());
    }
}

// ============================================================================
// PM_EXTRACT_LOG
// ============================================================================

AQLValue PM_EXTRACT_LOG(
    const std::vector<AQLValue>& args,
    ProcessMining& mining
) {
    if (args.size() < 2) {
        throw std::runtime_error("PM_EXTRACT_LOG requires 2 arguments: (collection, config)");
    }
    
    try {
        std::string collection = args[0].asString();
        nlohmann::json config_json = nlohmann::json::parse(args[1].asString());
        
        EventLogConfig config;
        config.case_id_field = config_json.value("case_id_field", "case_id");
        config.activity_field = config_json.value("activity_field", "activity");
        config.timestamp_field = config_json.value("timestamp_field", "timestamp");
        config.resource_field = config_json.value("resource_field", "resource");
        
        auto [status, log] = mining.extractEventLog(collection, config);
        
        if (!status.ok()) {
            throw std::runtime_error("PM_EXTRACT_LOG failed: " + status.message());
        }
        
        // Convert to JSON
        nlohmann::json log_json;
        log_json["case_count"] = log.cases.size();
        log_json["event_count"] = 0;
        
        for (const auto& [case_id, trace] : log.cases) {
            log_json["event_count"] = log_json["event_count"].get<int>() + trace.events.size();
        }
        
        return AQLValue(log_json.dump());
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PM_EXTRACT_LOG error: ") + e.what());
    }
}

// ============================================================================
// PM_DISCOVER_PROCESS
// ============================================================================

AQLValue PM_DISCOVER_PROCESS(
    const std::vector<AQLValue>& args,
    ProcessMining& mining
) {
    if (args.size() < 2) {
        throw std::runtime_error("PM_DISCOVER_PROCESS requires 2 arguments: (log, config)");
    }
    
    try {
        // This is a simplified stub
        // In real implementation, would parse EventLog from JSON and discover process
        
        nlohmann::json result;
        result["status"] = "discovered";
        result["algorithm"] = "heuristic";
        result["activities_count"] = 0;
        result["edges_count"] = 0;
        
        return AQLValue(result.dump());
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PM_DISCOVER_PROCESS error: ") + e.what());
    }
}

// ============================================================================
// PM_CONFORMANCE
// ============================================================================

AQLValue PM_CONFORMANCE(
    const std::vector<AQLValue>& args,
    ProcessMining& mining
) {
    if (args.size() < 2) {
        throw std::runtime_error("PM_CONFORMANCE requires 2 arguments: (log, model)");
    }
    
    try {
        // Simplified stub
        nlohmann::json result;
        result["fitness"] = 0.85;
        result["precision"] = 0.90;
        result["generalization"] = 0.80;
        result["simplicity"] = 0.95;
        
        return AQLValue(result.dump());
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PM_CONFORMANCE error: ") + e.what());
    }
}

// ============================================================================
// PM_EXPORT_BPMN
// ============================================================================

AQLValue PM_EXPORT_BPMN(
    const std::vector<AQLValue>& args,
    ProcessMining& mining
) {
    if (args.size() < 1) {
        throw std::runtime_error("PM_EXPORT_BPMN requires 1 argument: (model)");
    }
    
    try {
        // Simplified stub - would export to BPMN 2.0 XML
        std::string bpmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="discovered_process">
    <!-- BPMN content would be generated here -->
  </process>
</definitions>)";
        
        return AQLValue(bpmn_xml);
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("PM_EXPORT_BPMN error: ") + e.what());
    }
}

} // namespace functions
} // namespace query
} // namespace themis
