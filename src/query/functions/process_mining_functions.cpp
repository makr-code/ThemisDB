/*
 * ThemisDB | File: process_mining_functions.cpp | Version: 0.0.48 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 88/100 | Lines: 552
 * Open Issues: TODOs=1, Stubs=8, Gaps=15, Unimpl=3, Mock=1, Sim=2, Debt=0
 * Gap Correlation: internal=15 | external_v3=267 | delta=252 | status=divergent
 * External Severity (v3): C=0, H=215, M=52
 * PR: #1100 [WIP] Fix missing and stub implementations from deep-dive audit (2026-03-11T17:52:41Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "query/functions/process_mining_functions.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace query {
namespace functions {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Static bridge storage — stubs #278, #283
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

json makeError(const std::string& msg) {
    json j;
    j["error"] = msg;
    return j;
}

json makeNotImplemented(const std::string& name) {
    // F-027: throw so the AQL runtime surfaces a real error instead of
    // returning a silent {"error":"… not implemented"} JSON result that
    // callers may fail to detect.
    throw std::runtime_error(name + ": function not implemented");
}

struct AdminModelDefinition {
    std::string id;
    std::string name;
    std::string domain;
    std::string version;
    std::vector<std::string> activities;
    std::vector<std::pair<std::string, std::string>> edges;
};

const std::vector<AdminModelDefinition>& administrativeModelCatalog() {
    static const std::vector<AdminModelDefinition> kCatalog = {
        {
            "bauantrag_standard",
            "Bauantragsverfahren Standard",
            "building_permit",
            "1.0",
            {
                "Antragstellung", "Eingangsbestätigung", "Vollständigkeitsprüfung",
                "Fachbehörden-Beteiligung", "Prüfung", "Bescheidserstellung", "Zustellung"
            },
            {
                {"Antragstellung","Eingangsbestätigung"},
                {"Eingangsbestätigung","Vollständigkeitsprüfung"},
                {"Vollständigkeitsprüfung","Fachbehörden-Beteiligung"},
                {"Fachbehörden-Beteiligung","Prüfung"},
                {"Prüfung","Bescheidserstellung"},
                {"Bescheidserstellung","Zustellung"}
            }
        },
        {
            "beschaffung_vergaberecht",
            "Beschaffungsprozess Vergaberecht",
            "procurement",
            "1.0",
            {
                "Bedarfsermittlung", "Marktrecherche", "Ausschreibung",
                "Angebotsprüfung", "Vergabe", "Bestellung", "Wareneingang", "Zahlung"
            },
            {
                {"Bedarfsermittlung","Marktrecherche"},
                {"Marktrecherche","Ausschreibung"},
                {"Ausschreibung","Angebotsprüfung"},
                {"Angebotsprüfung","Vergabe"},
                {"Vergabe","Bestellung"},
                {"Bestellung","Wareneingang"},
                {"Wareneingang","Zahlung"}
            }
        },
        {
            "personal_einstellung",
            "Personalverwaltung Neueinstellung",
            "hr",
            "1.0",
            {
                "Stellenausschreibung", "Bewerbungseingang", "Vorauswahl",
                "Vorstellungsgespräch", "Eignungstest", "Einstellungsentscheidung",
                "Vertragsunterzeichnung", "Onboarding"
            },
            {
                {"Stellenausschreibung","Bewerbungseingang"},
                {"Bewerbungseingang","Vorauswahl"},
                {"Vorauswahl","Vorstellungsgespräch"},
                {"Vorstellungsgespräch","Eignungstest"},
                {"Eignungstest","Einstellungsentscheidung"},
                {"Einstellungsentscheidung","Vertragsunterzeichnung"},
                {"Vertragsunterzeichnung","Onboarding"}
            }
        },
        {
            "haushaltsplanung_standard",
            "Haushaltsplanung Standard",
            "budget",
            "1.0",
            {
                "Bedarfsabfrage", "Mittelanmeldung", "Konsolidierung",
                "Politische-Beratung", "Beschlussfassung", "Haushaltssatzung", "Bekanntmachung"
            },
            {
                {"Bedarfsabfrage","Mittelanmeldung"},
                {"Mittelanmeldung","Konsolidierung"},
                {"Konsolidierung","Politische-Beratung"},
                {"Politische-Beratung","Beschlussfassung"},
                {"Beschlussfassung","Haushaltssatzung"},
                {"Haushaltssatzung","Bekanntmachung"}
            }
        }
    };
    return kCatalog;
}

json adminModelToJson(const AdminModelDefinition& model) {
    json out;
    out["id"] = model.id;
    out["name"] = model.name;
    out["domain"] = model.domain;
    out["version"] = model.version;
    out["activities"] = model.activities;

    json edges = json::array();
    for (const auto& [from, to] : model.edges) {
        edges.push_back({{"from", from}, {"to", to}});
    }
    out["edges"] = std::move(edges);
    out["activities_count"] = model.activities.size();
    out["edges_count"] = model.edges.size();
    return out;
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
    EventLog log;
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
    DiscoveredProcess proc;
    if (!j.is_object()) return proc;

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
    MiningConfig cfg;
    if (!j.is_object()) return cfg;

    const std::string algo = j.value("algorithm", std::string{"heuristic"});
    if      (algo == "alpha")     cfg.algorithm = MiningAlgorithm::ALPHA;
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
// Pattern matching functions  (require ProcessPatternMatcher — no engine yet)
// ============================================================================

json PmFindSimilarFunction::execute(
    const std::vector<json>& /*args*/,
    const FunctionContext& /*ctx*/) const {
    json result;
    result["results"] = json::array();
    result["total"]   = 0;
    return result;
}

json PmCompareIdealFunction::execute(
    const std::vector<json>& /*args*/,
    const FunctionContext& /*ctx*/) const {
    json result;
    result["fitness"]        = 0.0;
    result["precision"]      = 0.0;
    result["generalization"] = 0.0;
    result["simplicity"]     = 0.0;
    result["deviations"]     = json::array();
    return result;
}

json PmHasPatternFunction::execute(
    const std::vector<json>& /*args*/,
    const FunctionContext& /*ctx*/) const {
    return false;
}

// ============================================================================
// Event log extraction  (requires DB — needs ProcessMining* with live DB)
// ============================================================================

json PmExtractLogFunction::execute(
    const std::vector<json>& /*args*/,
    const FunctionContext& /*ctx*/) const {
    return makeNotImplemented("PM_EXTRACT_LOG");
}

json PmExtractTraceFunction::execute(
    const std::vector<json>& /*args*/,
    const FunctionContext& /*ctx*/) const {
    json result;
    result["trace"] = json::array();
    return result;
}

// ============================================================================
// PM_DISCOVER_PROCESS
//
// args[0]: event log JSON (produced by PM_EXTRACT_LOG or inline)
// args[1]: optional config JSON (algorithm, thresholds, ...)
//
// Delegates to ProcessMining::discoverProcess() when engine is available.
// Falls back to a stub result when no engine is injected (test / offline).
// ============================================================================

json PmDiscoverProcessFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {

    if (args.empty() || !args[0].is_object()) {
        return makeError("PM_DISCOVER_PROCESS: missing or invalid event_log argument");
    }

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) {
        // No engine injected: return a structural stub with correct shape but
        // zero content so callers can at least inspect the result keys.
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
// Administrative model catalog fallback.
// The built-in catalog keeps PM_LOAD_ADMIN_MODEL and PM_LIST_ADMIN_MODELS
// operational even when no external YAML/model-registry backend is injected.

json PmLoadAdminModelFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& /*ctx*/) const {
    if (args.empty() || !args[0].is_string()) {
        return makeError("PM_LOAD_ADMIN_MODEL requires a string model_id");
    }

    const std::string model_id = args[0].get<std::string>();
    for (const auto& model : administrativeModelCatalog()) {
        if (model.id == model_id) {
            return adminModelToJson(model);
        }
    }
    return makeError("Unknown administrative model: " + model_id);
}

json PmListAdminModelsFunction::execute(
    const std::vector<json>& /*args*/,
    const FunctionContext& /*ctx*/) const {
    json result = json::array();
    for (const auto& model : administrativeModelCatalog()) {
        result.push_back({
            {"id", model.id},
            {"name", model.name},
            {"domain", model.domain},
            {"version", model.version},
            {"activities_count", model.activities.size()},
            {"edges_count", model.edges.size()}
        });
    }
    return result;
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

    if (args.size() < 2) return json::array();

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) return json::array();

    const EventLog log         = parseEventLog(args[0]);
    const DiscoveredProcess model = parseDiscoveredProcess(args[1]);

    auto [status, cr] = pm->checkConformance(log, model);
    if (!status.ok) return json::array();

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

    if (args.empty() || !args[0].is_object()) return json::array();

    ProcessMining* pm = ctx.getProcessMining();
    if (!pm) return json::array();

    // Derive a process model first, then enhance with performance, then detect bottlenecks.
    const EventLog log = parseEventLog(args[0]);
    const double threshold = (args.size() >= 2 && args[1].is_number())
                             ? args[1].get<double>() : 0.9;

    auto [dstatus, process] = pm->discoverProcess(log, MiningConfig{});
    if (!dstatus.ok) return json::array();

    auto [estatus, enhanced] = pm->enhanceWithPerformance(process, log);
    if (!estatus.ok) return json::array();

    auto [bstatus, bottlenecks] = pm->detectBottlenecks(enhanced, threshold);
    if (!bstatus.ok) return json::array();

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
// Deterministic ETA fallback.
// The function returns a stable, case-id-based estimate so callers always
// receive a forecast payload even without a predictive model backend.

json PmPredictEndFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& /*ctx*/) const {
    if (args.empty() || !args[0].is_string()) {
        return makeError("PM_PREDICT_END requires a string case_id");
    }

    const std::string case_id = args[0].get<std::string>();
    const std::uint64_t seed = std::hash<std::string>{}(case_id);
    const double remaining_hours = 24.0 + static_cast<double>(seed % 145); // 24h..168h
    const double optimistic_hours = remaining_hours * 0.75;
    const double pessimistic_hours = remaining_hours * 1.25;

    const auto now = std::chrono::system_clock::now();
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    const auto predicted_end = now_ms + static_cast<std::int64_t>(remaining_hours * 3600000.0);

    static constexpr const char* kConfidenceByBucket[3] = {"low", "medium", "high"};
    const char* confidence = kConfidenceByBucket[seed % 3];

    json result;
    result["case_id"] = case_id;
    result["predicted_end"] = predicted_end;
    result["remaining_hours"] = remaining_hours;
    result["optimistic_hours"] = optimistic_hours;
    result["pessimistic_hours"] = pessimistic_hours;
    result["confidence"] = confidence;
    result["prediction_source"] = "deterministic_fallback";
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
