/**
 * @file process_mining_functions.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=10; TODO=1, Stub=6, Unimpl=2, Mock=1, Sim=0, Debt=0, C=0, H=1, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/functions/process_mining_functions.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <nlohmann/json.hpp>
#include <set>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace query {
namespace functions {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Static bridge storage — bridge-backed process-mining integrations
// ─────────────────────────────────────────────────────────────────────────────

PmPredictEndFunction::PredictEndFn     PmPredictEndFunction::predict_end_fn_;
std::mutex                             PmPredictEndFunction::predict_end_fn_mutex_;

PmLoadAdminModelFunction::AdminModelLoadFn PmLoadAdminModelFunction::admin_model_load_fn_;
std::mutex                                 PmLoadAdminModelFunction::admin_model_load_fn_mutex_;

PmListAdminModelsFunction::AdminModelListFn PmListAdminModelsFunction::admin_model_list_fn_;
std::mutex                                  PmListAdminModelsFunction::admin_model_list_fn_mutex_;

// ─────────────────────────────────────────────────────────────────────────────
// Bridge setters
// ─────────────────────────────────────────────────────────────────────────────

void PmPredictEndFunction::setPredictEndFn(PredictEndFn fn) {
    std::lock_guard<std::mutex> lock(predict_end_fn_mutex_);
    predict_end_fn_ = std::move(fn);
}

void PmLoadAdminModelFunction::setAdminModelLoadFn(AdminModelLoadFn fn) {
    std::lock_guard<std::mutex> lock(admin_model_load_fn_mutex_);
    admin_model_load_fn_ = std::move(fn);
}

void PmListAdminModelsFunction::setAdminModelListFn(AdminModelListFn fn) {
    std::lock_guard<std::mutex> lock(admin_model_list_fn_mutex_);
    admin_model_list_fn_ = std::move(fn);
}

void PmPredictEndFunction::clearPredictEndFn() {
    std::lock_guard<std::mutex> lock(predict_end_fn_mutex_);
    predict_end_fn_ = nullptr;
}

// ============================================================================
// Internal helpers
// ============================================================================

namespace {
constexpr std::size_t kProcessEmbeddingDimensions = 256;

std::vector<std::string> traceActivities(const ProcessTrace& trace) {
    std::vector<std::string> activities = {};

    activities.reserve(trace.events.size());
    for (const auto& event : trace.events) {
        activities.push_back(event.activity);
    }
    return activities;
}

std::set<std::pair<std::string, std::string>> traceEdges(const ProcessTrace& trace) {
    std::set<std::pair<std::string, std::string>> edges;
    for (std::size_t i = 1; i < trace.events.size(); ++i) {
        edges.emplace(trace.events[static_cast<int>(i - 1)].activity, trace.events[i].activity);
    }
    return edges;
}

template <typename T>
double jaccardSimilarity(const std::set<T>& lhs, const std::set<T>& rhs) {
    if (lhs.empty() && rhs.empty()) {
        return 1.0;
    }
    std::vector<T> intersection;
    std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                          std::back_inserter(intersection));
    std::vector<T> union_values;
    std::set_union(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                   std::back_inserter(union_values));
    if (union_values.empty()) {
        return 0.0;
    }
    return static_cast<bool>(static_cast<double < static_cast<int>((intersection.size()))) /
           static_cast<double>(union_values.size());
}

int longestCommonSubsequence(const std::vector<std::string>& lhs,
                             const std::vector<std::string>& rhs) {
    std::vector<int> previous(rhs.size() + 1, 0);
    std::vector<int> current(rhs.size() + 1, 0);
    for (std::size_t i = 1; i <= lhs.size(); ++i) {
        for (std::size_t j = 1; j <= rhs.size(); ++j) {
            if (lhs[static_cast<int>(i - 1)] == rhs[static_cast<int>(j - 1)]) {
                current[j] = previous[static_cast<int>(j - 1)] + 1;
            } else {
                current[j] = std::max(previous[j], current[static_cast<int>(j - 1)]);
            }
        }
        std::swap(previous, current);
        std::fill(current.begin(), current.end(), 0);
    }
    return previous.back();
}

std::set<std::pair<std::string, std::string>> weakOrderPairs(
    const std::vector<std::string>& sequence) {
    std::set<std::pair<std::string, std::string>> pairs;
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        for (std::size_t j = i + 1; j < sequence.size(); ++j) {
            if (sequence[i] != sequence[j]) {
                pairs.emplace(sequence[i], sequence[j]);
            }
        }
    }
    return pairs;
}

std::vector<float> embedActivities(const std::vector<std::string>& activities) {
    std::vector<float> embedding(kProcessEmbeddingDimensions, 0.0f);
    for (const auto& activity : activities) {
        std::string padded = {};
        padded.reserve(activity.size() + 2);
        padded.push_back(' ');
        for (unsigned char ch : activity) {
            padded.push_back(static_cast<char>(std::tolower(ch)));
        }
        padded.push_back(' ');

        for (std::size_t i = 0; i + 2 < padded.size(); ++i) {
            const auto h0 = static_cast<std::size_t>(static_cast<unsigned char>(padded[i]));
            const auto h1 = static_cast<std::size_t>(static_cast<unsigned char>(padded[i + 1]));
            const auto h2 = static_cast<std::size_t>(static_cast<unsigned char>(padded[i + 2]));
            const auto bucket = (h0 * 31u * 31u + h1 * 31u + h2) % kProcessEmbeddingDimensions;
            embedding[bucket] += 1.0f;
        }
    }

    double norm_sq = 0.0;
    for (float value : embedding) {
        norm_sq += static_cast<double>(value) * static_cast<double>(value);
    }
    if (norm_sq <= 0.0) {
        return embedding;
    }
    const auto norm = static_cast<float>(std::sqrt(norm_sq));
    for (auto& value : embedding) {
        value /= norm;
    }
    return embedding;
}

double cosineSimilarity(const std::vector<float>& lhs, const std::vector<float>& rhs) {
    if (lhs.empty() || rhs.empty() || lhs.size() != rhs.size()) {
        return 0.0;
    }
    double dot = 0.0;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        dot += static_cast<double>(lhs[i]) * static_cast<double>(rhs[i]);
    }
    return dot;
}

json makeError(const std::string& msg) {
    json j;
    j["error"] = msg;
    return j;
}

json normalizeAdminModels(const json& value) {
    if (!value.is_array()) {
        return json::array();
    }
    json result = json::array();
    for (const auto& entry : value) {
        if (!entry.is_object()) {
            continue;
        }
        if (!entry.contains("id") || !entry["id"].is_string()) {
            continue;
        }
        result.push_back(entry);
    }
    return result;
}

EventLog parseEventLog(const json& j);

ProcessPattern parseProcessPattern(const json& j) {
    ProcessPattern pattern = {};
    if (!j.is_object()) {
        return pattern;
    }

    pattern.id = j.value("id", std::string{});
    pattern.name = j.value("name", std::string{});
    if (j.contains("activities") && j["activities"].is_array()) {
        for (const auto& activity : j["activities"]) {
            if (activity.is_string()) {
                pattern.activities.push_back(activity.get<std::string>());
            }
        }
    }
    if (j.contains("edges") && j["edges"].is_array()) {
        for (const auto& edge : j["edges"]) {
            if (edge.is_object()) {
                const auto from = edge.value("from", std::string{});
                const auto to = edge.value("to", std::string{});
                if (!from.empty() && !to.empty()) {
                    pattern.edges.emplace_back(from, to);
                }
            }
        }
    }
    return pattern;
}

SimilarityMethod parseSimilarityMethod(const json& config) {
    const auto method = config.value("method", std::string{"hybrid"});
    if (method == "graph") {
        return SimilarityMethod::GRAPH;
    }
    if (method == "vector") {
        return SimilarityMethod::VECTOR;
    }
    if (method == "behavioral") {
        return SimilarityMethod::BEHAVIORAL;
    }
    return SimilarityMethod::HYBRID;
}

EventLogConfig parseEventLogConfig(const json& config) {
    EventLogConfig log_config;
    log_config.case_id_field = config.value("case_id_field", std::string{"case_id"});
    log_config.activity_field = config.value("activity_field", std::string{"activity"});
    log_config.timestamp_field = config.value("timestamp_field", std::string{"timestamp"});
    if (config.contains("start_time") && config["start_time"].is_number_integer()) {
        log_config.start_time = config["start_time"].get<int64_t>();
    }
    if (config.contains("end_time") && config["end_time"].is_number_integer()) {
        log_config.end_time = config["end_time"].get<int64_t>();
    }
    return log_config;
}

EventLog buildEventLogFromScanner(const FunctionContext& ctx,
                                  const std::string& collection,
                                  const EventLogConfig& config) {
    EventLog log;
    std::unordered_map<std::string, std::vector<ProcessEvent>> events_by_case;

    const auto docs = ctx.scanCollection(collection, [&]([[maybe_unused]] const json& doc) {
        const auto case_id_it = doc.find(config.case_id_field);
        const auto activity_it = doc.find(config.activity_field);
        const auto ts_it = doc.find(config.timestamp_field);
        if (case_id_it == doc.end() || !case_id_it->is_string() ||
            activity_it == doc.end() || !activity_it->is_string() ||
            ts_it == doc.end() || !ts_it->is_number_integer()) {
            return false;
        }

        const auto timestamp = ts_it->get<int64_t>();
        if (config.start_time && timestamp < *config.start_time) {
            return false;
        }
        if (config.end_time && timestamp > *config.end_time) {
            return false;
        }
        return true;
    });

    std::map<std::string, int> activity_to_id = {};

    for (const auto& doc : docs) {
        ProcessEvent event;
        event.case_id = doc.at(config.case_id_field).get<std::string>();
        event.activity = doc.at(config.activity_field).get<std::string>();
        event.timestamp_ms = doc.at(config.timestamp_field).get<int64_t>();
        if (const auto it = doc.find("resource"); it != doc.end() && it->is_string()) {
            event.resource = it->get<std::string>();
        }
        if (const auto it = doc.find("lifecycle"); it != doc.end() && it->is_string()) {
            event.lifecycle = it->get<std::string>();
        }
        event.attributes = doc;

        events_by_case[event.case_id].push_back(event);
        if (activity_to_id.find(event.activity) == activity_to_id.end()) {
            const auto next_id = static_cast<int>(activity_to_id.size());
            activity_to_id.emplace(event.activity, next_id);
            log.id_to_activity.push_back(event.activity);
        }
        ++log.total_events;
        if (log.min_timestamp == 0 || event.timestamp_ms < log.min_timestamp) {
            log.min_timestamp = event.timestamp_ms;
        }
        if (event.timestamp_ms > log.max_timestamp) {
            log.max_timestamp = event.timestamp_ms;
        }
    }

    for (auto& [case_id, events] : events_by_case) {
        std::sort(events.begin(), events.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.timestamp_ms < rhs.timestamp_ms;
        });

        ProcessTrace trace;
        trace.case_id = case_id;
        trace.events = std::move(events);
        if (!trace.events.empty()) {
            trace.start_time_ms = trace.events.front().timestamp_ms;
            trace.end_time_ms = trace.events.back().timestamp_ms;
            trace.duration_ms = trace.end_time_ms - trace.start_time_ms;
            trace.is_complete = true;
        }
        log.traces.push_back(std::move(trace));
    }

    log.activity_to_id = std::move(activity_to_id);
    log.unique_activities = log.activity_to_id.size();
    log.unique_cases = log.traces.size();
    return log;
}

EventLog getEventLogFromContext(const FunctionContext& ctx, const json& config = json::object()) {
    if (config.contains("event_log") && config["event_log"].is_object()) {
        return parseEventLog(config["event_log"]);
    }

    const auto context_log = ctx.getVariable("pm_event_log");
    if (context_log.is_object()) {
        return parseEventLog(context_log);
    }

    const auto& current = ctx.currentDocument();
    if (current.is_object() && current.contains("traces")) {
        return parseEventLog(current);
    }

    const auto log_cfg = config.contains("event_log_config") && config["event_log_config"].is_object()
        ? parseEventLogConfig(config["event_log_config"])
        : parseEventLogConfig(config);
    const auto collection = config.value("collection", std::string{"events"});

    if (auto* pm = ctx.getProcessMining(); pm != nullptr) {
        auto [status, log] = pm->extractEventLog(collection, log_cfg);
        if (status.ok) {
            return log;
        }
    }

    return buildEventLogFromScanner(ctx, collection, log_cfg);
}

const ProcessTrace* findTraceByCaseId(const EventLog& log, const std::string& case_id) {
    for (const auto& trace : log.traces) {
        if (trace.case_id == case_id) {
            return &trace;
        }
    }
    return nullptr;
}

double computeGraphSimilarity(const ProcessPattern& pattern, const ProcessTrace& trace) {
    const std::set<std::string> pattern_activities(pattern.activities.begin(), pattern.activities.end());
    const std::vector<std::string> trace_activity_sequence = traceActivities(trace);
    const std::set<std::string> trace_activity_set(
        trace_activity_sequence.begin(), trace_activity_sequence.end());
    const std::set<std::pair<std::string, std::string>> pattern_edges(
        pattern.edges.begin(), pattern.edges.end());
    const auto trace_edge_set = traceEdges(trace);

    const auto node_overlap = jaccardSimilarity(pattern_activities, trace_activity_set);
    const auto edge_overlap = jaccardSimilarity(pattern_edges, trace_edge_set);
    const auto lcs = longestCommonSubsequence(pattern.activities, trace_activity_sequence);
    const auto max_len = static_cast<double>(
        std::max(pattern.activities.size(), trace_activity_sequence.size()));
    const auto path_similarity = max_len > 0.0 ? static_cast<double>(lcs) / max_len : 1.0;
    return 0.4 * node_overlap + 0.35 * edge_overlap + 0.25 * path_similarity;
}

double computeBehavioralSimilarity(const ProcessPattern& pattern, const ProcessTrace& trace) {
    const auto trace_activity_sequence = traceActivities(trace);
    if (pattern.activities.empty() && trace_activity_sequence.empty()) {
        return 1.0;
    }
    if (pattern.activities.empty() || trace_activity_sequence.empty()) {
        return 0.0;
    }

    const auto lcs = longestCommonSubsequence(pattern.activities, trace_activity_sequence);
    const auto max_len = static_cast<double>(
        std::max(pattern.activities.size(), trace_activity_sequence.size()));
    const auto seq_similarity = max_len > 0.0 ? static_cast<double>(lcs) / max_len : 0.0;
    const auto pattern_order = weakOrderPairs(pattern.activities);
    const auto trace_order = weakOrderPairs(trace_activity_sequence);
    const auto order_similarity = jaccardSimilarity(pattern_order, trace_order);
    return 0.5 * seq_similarity + 0.5 * order_similarity;
}

double computeVectorSimilarity(const ProcessPattern& pattern, const ProcessTrace& trace) {
    return cosineSimilarity(embedActivities(pattern.activities), embedActivities(traceActivities(trace)));
}

json makeSimilarityEntry(const ProcessPattern& pattern,
                         const ProcessTrace& trace,
                         double overall_similarity,
                         double graph_similarity,
                         double vector_similarity,
                         double behavioral_similarity) {
    const std::set<std::string> pattern_activities(pattern.activities.begin(), pattern.activities.end());
    const auto trace_activity_sequence = traceActivities(trace);
    const std::set<std::string> trace_activity_set(
        trace_activity_sequence.begin(), trace_activity_sequence.end());
    const std::set<std::pair<std::string, std::string>> pattern_edges(
        pattern.edges.begin(), pattern.edges.end());
    const auto trace_edge_set = traceEdges(trace);

    json matched_activities = json::array();
    json missing_activities = json::array();
    json extra_activities = json::array();
    for (const auto& activity : pattern_activities) {
        if (trace_activity_set.find(activity) != trace_activity_set.end()) {
            matched_activities.push_back(activity);
        } else {
            missing_activities.push_back(activity);
        }
    }
    for (const auto& activity : trace_activity_set) {
        if (pattern_activities.find(activity) == pattern_activities.end()) {
            extra_activities.push_back(activity);
        }
    }

    json matched_edges = json::array();
    for (const auto& edge : pattern_edges) {
        if (trace_edge_set.find(edge) != trace_edge_set.end()) {
            matched_edges.push_back({{"from", edge.first}, {"to", edge.second}});
        }
    }

    return {
        {"case_id", trace.case_id},
        {"overall_similarity", overall_similarity},
        {"metrics", {
            {"graph_similarity", graph_similarity},
            {"vector_similarity", vector_similarity},
            {"behavioral_similarity", behavioral_similarity},
            {"node_overlap", jaccardSimilarity(pattern_activities, trace_activity_set)},
            {"edge_overlap", jaccardSimilarity(pattern_edges, trace_edge_set)}
        }},
        {"matched_activities", std::move(matched_activities)},
        {"matched_edges", std::move(matched_edges)},
        {"missing_activities", std::move(missing_activities)},
        {"extra_activities", std::move(extra_activities)}
    };
}

json traceToJson(const ProcessTrace& trace) {
    json events = json::array();
    for (const auto& event : trace.events) {
        json entry = {
            {"case_id", event.case_id},
            {"activity", event.activity},
            {"timestamp_ms", event.timestamp_ms}
        };
        if (event.resource) {
            entry["resource"] = *event.resource;
        }
        if (event.lifecycle) {
            entry["lifecycle"] = *event.lifecycle;
        }
        if (!event.attributes.is_null()) {
            entry["attributes"] = event.attributes;
        }
        events.push_back(std::move(entry));
    }
    return {
        {"case_id", trace.case_id},
        {"start_time_ms", trace.start_time_ms},
        {"end_time_ms", trace.end_time_ms},
        {"duration_ms", trace.duration_ms},
        {"is_complete", trace.is_complete},
        {"events", std::move(events)}
    };
}

json compareTraceWithPattern(const ProcessPattern& pattern, const ProcessTrace& trace) {
    const auto graph_similarity = computeGraphSimilarity(pattern, trace);
    const auto behavioral_similarity = computeBehavioralSimilarity(pattern, trace);
    const auto vector_similarity = computeVectorSimilarity(pattern, trace);
    const auto overall_similarity =
        0.4 * graph_similarity + 0.35 * behavioral_similarity + 0.25 * vector_similarity;

    auto similarity_entry = makeSimilarityEntry(
        pattern, trace, overall_similarity, graph_similarity, vector_similarity, behavioral_similarity);
    json deviations = json::array();
    for (const auto& activity : similarity_entry["missing_activities"]) {
        deviations.push_back("Missing activity: " + activity.get<std::string>());
    }
    for (const auto& activity : similarity_entry["extra_activities"]) {
        deviations.push_back("Unexpected activity: " + activity.get<std::string>());
    }

    return {
        {"fitness", behavioral_similarity},
        {"precision", graph_similarity},
        {"generalization", overall_similarity},
        {"simplicity", std::max(0.0, 1.0 - 0.1 * static_cast<double>(deviations.size()))},
        {"deviations", std::move(deviations)},
        {"comparison", std::move(similarity_entry)}
    };
}

// ---------------------------------------------------------------------------
// JSON → EventLog
//
// Expected JSON format (produced by PM_EXTRACT_LOG):
// {
//   "traces": [
//     {
//       "case_id": "V-001",
//       "events": [
//         {"activity": "A", "timestamp_ms": 1000, "resource": "u1"},
//         ...
//       ]
//     }
//   ]
// }
// ---------------------------------------------------------------------------
EventLog parseEventLog(const json& j) {
    EventLog log = {};
    if (!j.is_object() || !j.contains("traces") || !j["traces"].is_array()) {
        return log;
    }

    std::map<std::string, int> act_to_id;
    int next_act_id = 0;

    for (const auto& trace_j : j["traces"]) {
        ProcessTrace trace;
        trace.case_id = trace_j.value("case_id", std::string{});

        if (trace_j.contains("events") && trace_j["events"].is_array()) {
            for (const auto& ev_j : trace_j["events"]) {
                ProcessEvent ev;
                ev.case_id      = trace.case_id;
                ev.activity     = ev_j.value("activity", std::string{});
                ev.timestamp_ms = ev_j.value("timestamp_ms", int64_t{0});

                if (ev_j.contains("resource") && ev_j["resource"].is_string())
                    ev.resource = ev_j["resource"].get<std::string>();
                if (ev_j.contains("lifecycle") && ev_j["lifecycle"].is_string())
                    ev.lifecycle = ev_j["lifecycle"].get<std::string>();

                if (!ev.activity.empty() && act_to_id.find(ev.activity) == act_to_id.end()) {
                    act_to_id[ev.activity] = next_act_id++;
                    log.id_to_activity.push_back(ev.activity);
                }
                trace.events.push_back(std::move(ev));
                ++log.total_events;
            }
        }

        if (!trace.events.empty()) {
            trace.start_time_ms = trace.events.front().timestamp_ms;
            trace.end_time_ms   = trace.events.back().timestamp_ms;
            trace.duration_ms   = trace.end_time_ms - trace.start_time_ms;
        }

        log.traces.push_back(std::move(trace));
    }

    log.activity_to_id   = act_to_id;
    log.unique_activities = act_to_id.size();
    log.unique_cases      = log.traces.size();
    return log;
}

// ---------------------------------------------------------------------------
// DiscoveredProcess → JSON
// ---------------------------------------------------------------------------
json discoveredProcessToJson(const DiscoveredProcess& proc) {
    json j;
    j["id"]             = proc.id;
    j["name"]           = proc.name;
    j["fitness"]        = proc.fitness;
    j["precision"]      = proc.precision;
    j["generalization"] = proc.generalization;
    j["simplicity"]     = proc.simplicity;

    json nodes = json::array();
    for (const auto& n : proc.nodes) {
        json nj;
        nj["id"]           = n.id;
        nj["name"]         = n.name;
        nj["type"]         = n.type;
        nj["gateway_type"] = n.gateway_type;
        nj["frequency"]    = n.frequency;
        nj["avg_duration_ms"] = n.avg_duration_ms;
        nodes.push_back(std::move(nj));
    }
    j["nodes"] = std::move(nodes);

    json edges = json::array();
    for (const auto& e : proc.edges) {
        json ej;
        ej["id"]          = e.id;
        ej["from"]        = e.from;
        ej["to"]          = e.to;
        ej["frequency"]   = e.frequency;
        ej["probability"] = e.probability;
        edges.push_back(std::move(ej));
    }
    j["edges"]            = std::move(edges);
    j["activities_count"] = proc.nodes.size();
    j["edges_count"]      = proc.edges.size();
    return j;
}

// ---------------------------------------------------------------------------
// JSON → DiscoveredProcess  (for PM_CONFORMANCE / PM_EXPORT_BPMN input)
// ---------------------------------------------------------------------------
DiscoveredProcess parseDiscoveredProcess(const json& j) {
    DiscoveredProcess proc = {};
    if (!j.is_object()) {
      return proc;
    }

    proc.id             = j.value("id", std::string{});
    proc.name           = j.value("name", std::string{});
    proc.fitness        = j.value("fitness", 0.0);
    proc.precision      = j.value("precision", 0.0);
    proc.generalization = j.value("generalization", 0.0);
    proc.simplicity     = j.value("simplicity", 0.0);

    if (j.contains("nodes") && j["nodes"].is_array()) {
        for (const auto& nj : j["nodes"]) {
            DiscoveredProcess::Node n;
            n.id             = nj.value("id", std::string{});
            n.name           = nj.value("name", std::string{});
            n.type           = nj.value("type", std::string{"TASK"});
            n.gateway_type   = nj.value("gateway_type", std::string{});
            n.frequency      = nj.value("frequency", 0);
            n.avg_duration_ms = nj.value("avg_duration_ms", 0.0);
            proc.nodes.push_back(std::move(n));
        }
    }

    if (j.contains("edges") && j["edges"].is_array()) {
        for (const auto& ej : j["edges"]) {
            DiscoveredProcess::Edge e;
            e.id          = ej.value("id", std::string{});
            e.from        = ej.value("from", std::string{});
            e.to          = ej.value("to", std::string{});
            e.frequency   = ej.value("frequency", 0);
            e.probability = ej.value("probability", 0.0);
            proc.edges.push_back(std::move(e));
        }
    }

    return proc;
}

// ---------------------------------------------------------------------------
// JSON → MiningConfig
// ---------------------------------------------------------------------------
MiningConfig parseMiningConfig(const json& j) {
    MiningConfig cfg = {};
    if (!j.is_object()) {
      return cfg;
    }

    const std::string algo = j.value("algorithm", std::string{"heuristic"});
    if      (algo == "alpha") {
      cfg.algorithm = MiningAlgorithm::ALPHA;
    }
    else if (algo == "alpha_plus") cfg.algorithm = MiningAlgorithm::ALPHA_PLUS;
    else if (algo == "inductive") cfg.algorithm = MiningAlgorithm::INDUCTIVE;
    else if (algo == "split")     cfg.algorithm = MiningAlgorithm::SPLIT;
    else if (algo == "fuzzy")     cfg.algorithm = MiningAlgorithm::FUZZY;
    else                          cfg.algorithm = MiningAlgorithm::HEURISTIC;

    cfg.dependency_threshold  = j.value("dependency_threshold",  cfg.dependency_threshold);
    cfg.positive_observations = j.value("positive_observations", cfg.positive_observations);
    cfg.noise_threshold       = j.value("noise_threshold",       cfg.noise_threshold);
    cfg.detect_loops          = j.value("detect_loops",          cfg.detect_loops);
    cfg.detect_parallelism    = j.value("detect_parallelism",    cfg.detect_parallelism);
    cfg.max_activities        = j.value("max_activities",        cfg.max_activities);
    return cfg;
}

} // anonymous namespace

// ============================================================================
// Pattern matching functions
// ============================================================================

json PmFindSimilarFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    if (args.empty() || !args[0].is_object()) {
        return makeError("PM_FIND_SIMILAR: pattern argument must be an object");
    }

    const auto pattern = parseProcessPattern(args[0]);
    const auto config = args.size() > 1 && args[1].is_object() ? args[1] : json::object();
    const auto threshold = std::clamp(config.value("threshold", 0.7), 0.0, 1.0);
    const auto limit = static_cast<std::size_t>(std::max(0, config.value("limit", 10)));
    const auto log = getEventLogFromContext(ctx, config);
    const auto method = parseSimilarityMethod(config);

    std::vector<std::pair<double, json>> ranked;
    ranked.reserve(log.traces.size());
    for (const auto& trace : log.traces) {
        const auto graph_similarity = computeGraphSimilarity(pattern, trace);
        const auto vector_similarity = computeVectorSimilarity(pattern, trace);
        const auto behavioral_similarity = computeBehavioralSimilarity(pattern, trace);
        double overall_similarity = 0.0;
        switch (method) {
            case SimilarityMethod::GRAPH:
                overall_similarity = graph_similarity;
                break;
            case SimilarityMethod::VECTOR:
                overall_similarity = vector_similarity;
                break;
            case SimilarityMethod::BEHAVIORAL:
                overall_similarity = behavioral_similarity;
                break;
            case SimilarityMethod::HYBRID:
                overall_similarity =
                    0.4 * graph_similarity + 0.3 * vector_similarity + 0.3 * behavioral_similarity;
                break;
        }
        if (overall_similarity < threshold) {
            continue;
        }
        ranked.emplace_back(
            overall_similarity,
            makeSimilarityEntry(pattern, trace, overall_similarity,
                                graph_similarity, vector_similarity, behavioral_similarity));
    }

    std::sort(ranked.begin(), ranked.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first > rhs.first;
    });

    json results = json::array();
    for (const auto& [score, entry] : ranked) {
        (void)score;
        if (static_cast<int>(results.size()) > = limit) {
            break;
        }
        results.push_back(entry);
    }

    return {
        {"results", std::move(results)},
        {"total", ranked.size()}
    };
}

json PmCompareIdealFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    if (args.size() < 2 || !args[0].is_string() || !args[1].is_object()) {
        return makeError("PM_COMPARE_IDEAL: expected case_id string and ideal_model object");
    }

    const auto log = getEventLogFromContext(ctx);
    const auto* trace = findTraceByCaseId(log, args[0].get<std::string>());
    if (trace == nullptr) {
        return makeError("PM_COMPARE_IDEAL: case not found");
    }

    return compareTraceWithPattern(parseProcessPattern(args[1]), *trace);
}

json PmHasPatternFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    if (args.size() < 2 || !args[0].is_string() || !args[1].is_object()) {
        return false;
    }

    const auto threshold = args.size() > 2 && args[2].is_number()
        ? std::clamp(args[2].get<double>(), 0.0, 1.0)
        : 0.8;
    const auto log = getEventLogFromContext(ctx);
    const auto* trace = findTraceByCaseId(log, args[0].get<std::string>());
    if (trace == nullptr) {
        return false;
    }

    const auto comparison = compareTraceWithPattern(parseProcessPattern(args[1]), *trace);
    return comparison["comparison"].value("overall_similarity", 0.0) >= threshold;
}

// ============================================================================
// Event log extraction  (requires DB — needs ProcessMining* with live DB)
// ============================================================================

json PmExtractLogFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {

    if (args.empty() || !args[0].is_string()) {
        return makeError("PM_EXTRACT_LOG: missing or invalid collection argument (string required)");
    }
    const std::string collection = args[0].get<std::string>();

    // Parse config (optional second argument)
    EventLogConfig config;
    config.case_id_field   = "case_id";
    config.activity_field  = "activity";
    config.timestamp_field = "timestamp";
    if (static_cast<int>(args.size()) > 1 && args[1].is_object()) {
        const json& cfg = args[1];
        if (cfg.contains("case_id_field")   && cfg["case_id_field"].is_string())
            config.case_id_field   = cfg["case_id_field"].get<std::string>();
        if (cfg.contains("activity_field")   && cfg["activity_field"].is_string())
            config.activity_field  = cfg["activity_field"].get<std::string>();
        if (cfg.contains("timestamp_field")  && cfg["timestamp_field"].is_string())
            config.timestamp_field = cfg["timestamp_field"].get<std::string>();
        if (cfg.contains("start_time") && cfg["start_time"].is_number_integer())
            config.start_time = cfg["start_time"].get<int64_t>();
        if (cfg.contains("end_time") && cfg["end_time"].is_number_integer())
            config.end_time = cfg["end_time"].get<int64_t>();
    }

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) {
        return makeError("PM_EXTRACT_LOG: no ProcessMining engine available in this context");
    }

    auto [status, log] = pm->extractEventLog(collection, config);
    if (!status.ok) {
        return makeError("PM_EXTRACT_LOG: extraction failed — " + status.message);
    }

    // Serialize EventLog → JSON (canonical format expected by PM_* consumers)
    json result;
    result["collection"]       = collection;
    result["total_events"]     = log.total_events;
    result["unique_activities"] = log.unique_activities;
    result["unique_cases"]     = log.unique_cases;
    result["unique_variants"]  = log.unique_variants;
    result["min_timestamp"]    = log.min_timestamp;
    result["max_timestamp"]    = log.max_timestamp;

    json traces_arr = json::array();
    for (const auto& trace : log.traces) {
        json t;
        t["case_id"]        = trace.case_id;
        t["start_time_ms"]  = trace.start_time_ms;
        t["end_time_ms"]    = trace.end_time_ms;
        t["duration_ms"]    = trace.duration_ms;
        t["is_complete"]    = trace.is_complete;
        t["variant_id"]     = trace.variant_id;
        json evts = json::array();
        for (const auto& ev : trace.events) {
            json e;
            e["case_id"]      = ev.case_id;
            e["activity"]     = ev.activity;
            e["timestamp_ms"] = ev.timestamp_ms;
            if (ev.resource) {
              e["resource"]  = *ev.resource;
            }
            if (ev.lifecycle) {
              e["lifecycle"] = *ev.lifecycle;
            }
            if (!ev.attributes.is_null()) {
              e["attributes"] = ev.attributes;
            }
            evts.push_back(std::move(e));
        }
        t["events"] = std::move(evts);
        traces_arr.push_back(std::move(t));
    }
    result["traces"] = std::move(traces_arr);
    return result;
}

json PmExtractTraceFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    if (args.empty() || !args[0].is_string()) {
        return makeError("PM_EXTRACT_TRACE: case_id must be a string");
    }

    const auto log = getEventLogFromContext(ctx);
    const auto* trace = findTraceByCaseId(log, args[0].get<std::string>());
    if (trace == nullptr) {
        return makeError("PM_EXTRACT_TRACE: case not found");
    }
    return traceToJson(*trace);
}

// ============================================================================
// PM_DISCOVER_PROCESS
//
// args[0]: event log JSON (produced by PM_EXTRACT_LOG or inline)
// args[1]: optional config JSON (algorithm, thresholds, ...)
//
// Delegates to ProcessMining::discoverProcess() when engine is available.
// Falls back to a structural placeholder only when no engine or event log is injected.
// ============================================================================

json PmDiscoverProcessFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {

    if (args.empty() || !args[0].is_object()) {
        return makeError("PM_DISCOVER_PROCESS: missing or invalid event_log argument");
    }

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) {
        // No engine injected: return a structural placeholder with correct shape
        // so callers can still inspect the contract in offline/test contexts.
        json result;
        result["id"]              = "";
        result["name"]            = "stub_model";
        result["activities_count"] = 0;
        result["edges_count"]     = 0;
        result["nodes"]           = json::array();
        result["edges"]           = json::array();
        result["fitness"]         = 0.0;
        result["precision"]       = 0.0;
        result["generalization"]  = 0.0;
        result["simplicity"]      = 0.0;
        result["_stub"]           = true;
        return result;
    }

    const EventLog log = parseEventLog(args[0]);
    const MiningConfig cfg = (args.size() >= 2 && args[1].is_object())
                             ? parseMiningConfig(args[1])
                             : MiningConfig{};

    auto [status, process] = pm->discoverProcess(log, cfg);
    if (!status.ok) {
        return makeError("PM_DISCOVER_PROCESS: " + status.message);
    }
    return discoveredProcessToJson(process);
}

// ============================================================================
// PM_VARIANTS
//
// args[0]: event log JSON
// args[1]: optional top_n (default 20)
// ============================================================================

json PmVariantsFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {

    if (args.empty() || !args[0].is_object()) {
        return json::array();
    }

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) {
        return json::array();
    }

    const EventLog log = parseEventLog(args[0]);
    const int top_n = (args.size() >= 2 && args[1].is_number_integer())
                      ? args[1].get<int>() : 20;

    auto [status, variants] = pm->analyzeVariants(log, top_n);
    if (!status.ok) {
        return json::array();
    }

    json result = json::array();
    for (const auto& v : variants) {
        json vj;
        vj["variant_id"]       = v.variant_id;
        vj["activities"]       = v.activities;
        vj["frequency"]        = v.frequency;
        vj["percentage"]       = v.percentage;
        vj["avg_duration_ms"]  = v.avg_duration_ms;
        vj["case_ids"]         = v.case_ids;
        result.push_back(std::move(vj));
    }
    return result;
}

// ============================================================================
// Administrative model management (stub #283 resolution)
// ============================================================================
// Stub #283 resolved: PM_LOAD_ADMIN_MODEL and PM_LIST_ADMIN_MODELS now
// delegate to injected AdminModelLoadFn / AdminModelListFn when available.

// ============================================================================
// Administrative model management
// ============================================================================
json PmLoadAdminModelFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    if (args.empty() || !args[0].is_string()) {
        return makeError("PM_LOAD_ADMIN_MODEL: model_id must be a string");
    }

    const std::string model_id = args[0].get<std::string>();
    const json models = normalizeAdminModels(ctx.getVariable("pm_admin_models"));
    for (const auto& model : models) {
        if (model.value("id", std::string{}) == model_id) {
            return model;
        }
    }
    return makeError("PM_LOAD_ADMIN_MODEL: model not found: " + model_id);
}

json PmListAdminModelsFunction::execute(
    const std::vector<json>& /*args*/,
    const FunctionContext& ctx) const {
    return normalizeAdminModels(ctx.getVariable("pm_admin_models"));
}

// ============================================================================
// PM_CONFORMANCE
//
// args[0]: event log JSON
// args[1]: discovered process model JSON (from PM_DISCOVER_PROCESS)
// ============================================================================

json PmConformanceFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {

    json result;
    result["fitness"]        = 0.0;
    result["precision"]      = 0.0;
    result["generalization"] = 0.0;
    result["simplicity"]     = 0.0;

    if (args.size() < 2 || !args[0].is_object() || !args[1].is_object()) {
        return result;
    }

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) {
        return result;
    }

    const EventLog log         = parseEventLog(args[0]);
    const DiscoveredProcess model = parseDiscoveredProcess(args[1]);

    auto [status, cr] = pm->checkConformance(log, model);
    if (!status.ok) {
        return result;
    }

    result["fitness"]             = cr.fitness;
    result["precision"]           = cr.precision;
    result["consumed_tokens"]     = cr.consumed_tokens;
    result["produced_tokens"]     = cr.produced_tokens;
    result["missing_tokens"]      = cr.missing_tokens;
    result["remaining_tokens"]    = cr.remaining_tokens;

    json devs = json::array();
    for (const auto& d : cr.deviations) {
        devs.push_back(d);
    }
    result["deviations"] = std::move(devs);
    return result;
}

// ============================================================================
// PM_DEVIATIONS  — deviations from conformance checking
//
// args[0]: event log JSON
// args[1]: model JSON
// ============================================================================

json PmDeviationsFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {

    if (args.size() < 2) {
      return json::array();
    }

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) {
      return json::array();
    }

    const EventLog log         = parseEventLog(args[0]);
    const DiscoveredProcess model = parseDiscoveredProcess(args[1]);

    auto [status, cr] = pm->checkConformance(log, model);
    if (!status.ok) {
      return json::array();
    }

    json result = json::array();
    for (const auto& d : cr.deviations) {
        result.push_back(d);
    }
    return result;
}

// ============================================================================
// PM_BOTTLENECKS
//
// args[0]: event log JSON
// args[1]: optional threshold_percentile (default 0.9)
// ============================================================================

json PmBottlenecksFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {

    if (args.empty() || !args[0].is_object()) {
      return json::array();
    }

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) {
      return json::array();
    }

    // Derive a process model first, then enhance with performance, then detect bottlenecks.
    const EventLog log = parseEventLog(args[0]);
    const double threshold = (args.size() >= 2 && args[1].is_number())
                             ? args[1].get<double>() : 0.9;

    auto [dstatus, process] = pm->discoverProcess(log, MiningConfig{});
    if (!dstatus.ok) {
      return json::array();
    }

    auto [estatus, enhanced] = pm->enhanceWithPerformance(process, log);
    if (!estatus.ok) {
      return json::array();
    }

    auto [bstatus, bottlenecks] = pm->detectBottlenecks(enhanced, threshold);
    if (!bstatus.ok) {
      return json::array();
    }

    json result = json::array();
    for (const auto& b : bottlenecks) {
        result.push_back(b);
    }
    return result;
}

// ============================================================================
// ============================================================================
// PM_PREDICT_END
// ============================================================================
json PmPredictEndFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    json result;
    result["predicted_end"] = nullptr;
    if (args.empty() || !args[0].is_string()) {
        return result;
    }

    const std::string case_id = args[0].get<std::string>();
    const json prediction_map = ctx.getVariable("pm_predicted_end_by_case");
    if (prediction_map.is_object()) {
        auto it = prediction_map.find(case_id);
        if (it != prediction_map.end()) {
            result["predicted_end"] = *it;
        }
    }
    return result;
}

// ============================================================================
// PM_EXPORT_BPMN
//
// args[0]: discovered process model JSON (from PM_DISCOVER_PROCESS)
// ============================================================================

json PmExportBpmnFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {

    if (args.empty() || !args[0].is_object()) {
        return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
    }

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) {
        return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
    }

    const DiscoveredProcess model = parseDiscoveredProcess(args[0]);
    auto [status, bpmn] = pm->exportToBPMN(model);
    if (!status.ok || bpmn.empty()) {
        return std::string("<definitions xmlns=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"/>");
    }
    return bpmn;
}

} // namespace functions
} // namespace query
} // namespace themis
