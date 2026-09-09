/**
 * @file mode_spec_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/ai_orchestrator.h"
#include <stdexcept>
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "utils/logger.h"

namespace themis::llm {

// ============================================================================
// ModeId helpers
// ============================================================================

ModeId modeIdFromString(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "ask") {
      return ModeId::Ask;
    }
    if (lower == "edit") {
      return ModeId::Edit;
    }
    if (lower == "rag") {
      return ModeId::Rag;
    }
    if (lower == "agentic") {
      return ModeId::Agentic;
    }
    if (lower == "multi_agent" || lower == "multiagent") {
      return ModeId::MultiAgent;
    }
    if (lower == "ethics") {
      return ModeId::Ethics;
    }
    return ModeId::Custom;
}

std::string modeIdToString(ModeId id) {
    switch (id) {
        case ModeId::Ask:        return "ask";
        case ModeId::Edit:       return "edit";
        case ModeId::Rag:        return "rag";
        case ModeId::Agentic:    return "agentic";
        case ModeId::MultiAgent: return "multi_agent";
        case ModeId::Ethics:     return "ethics";
        default:                 return "custom";
    }
}

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

// Read a scalar or default from a YAML node safely.
template <typename T>
T safeAs(const YAML::Node& n, const T& def) {
    try {
        if (n && n.IsDefined() && !n.IsNull()) {
            return n.as<T>();
        }
    } catch (...) {}
    return def;
}

RetrievalSpec parseRetrieval(const YAML::Node& node) {
    RetrievalSpec spec = {};
    if (!node || !node.IsMap()) {
      return spec;
    }

    spec.enabled   = safeAs<bool>(node["enabled"],   false);
    spec.strategy  = safeAs<std::string>(node["strategy"],  "hybrid");
    spec.top_k     = safeAs<int>(node["top_k"],      5);
    spec.threshold = safeAs<float>(node["threshold"], 0.5f);
    spec.rerank    = safeAs<bool>(node["rerank"],    false);
    spec.locality  = safeAs<std::string>(node["locality"],  "");
    spec.read_ts_semantics = safeAs<std::string>(node["read_ts_semantics"], "latest");

    if (node["chunking"] && node["chunking"].IsMap()) {
        const auto& ch = node["chunking"];
        spec.chunking.size     = safeAs<int>(ch["size"],    512);
        spec.chunking.overlap  = safeAs<int>(ch["overlap"], 64);
        spec.chunking.strategy = safeAs<std::string>(ch["strategy"], "fixed");
    }
    return spec;
}

OutputSpec parseOutput(const YAML::Node& node) {
    OutputSpec spec = {};
    if (!node || !node.IsMap()) {
      return spec;
    }

    spec.format = safeAs<std::string>(node["format"], "text");
    if (node["json_schema"] && !node["json_schema"].IsNull()) {
        spec.json_schema = node["json_schema"].as<std::string>();
    }
    if (node["grammar"] && !node["grammar"].IsNull()) {
        spec.grammar = node["grammar"].as<std::string>();
    }
    return spec;
}

BudgetSpec parseBudgets(const YAML::Node& node) {
    BudgetSpec spec = {};
    if (!node || !node.IsMap()) {
      return spec;
    }

    spec.max_tokens  = safeAs<int>(node["max_tokens"],   512);
    spec.timeout_ms  = safeAs<int>(node["timeout_ms"],   30000);
    spec.max_retries = safeAs<int>(node["max_retries"],  1);
    spec.temperature = safeAs<float>(node["temperature"], 0.7f);
    spec.top_p       = safeAs<float>(node["top_p"],       0.9f);
    spec.top_k       = safeAs<int>(node["top_k"],         40);
    return spec;
}

ObservabilitySpec parseObservability(const YAML::Node& node) {
    ObservabilitySpec spec = {};
    if (!node || !node.IsMap()) {
      return spec;
    }

    spec.log_requests  = safeAs<bool>(node["log_requests"],  true);
    spec.log_responses = safeAs<bool>(node["log_responses"], false);
    spec.metrics       = safeAs<bool>(node["metrics"],       true);
    spec.trace         = safeAs<bool>(node["trace"],         false);
    return spec;
}

ToolSpec parseToolSpec(const YAML::Node& node) {
    ToolSpec spec;
    spec.name        = safeAs<std::string>(node["name"],        "");
    spec.description = safeAs<std::string>(node["description"], "");
    spec.timeout_ms  = safeAs<int>(node["timeout_ms"],          5000);

    if (node["schema"] && !node["schema"].IsNull()) {
        try {
            std::ostringstream ss = {};
            ss << node["schema"];
            spec.args_schema = json::parse(ss.str());
        } catch (...) {
            THEMIS_DEBUG("mode_spec_loader::parseToolSpec: unhandled exception caught");
            spec.args_schema = json::object();
        }
    }
    return spec;
}

std::vector<std::string> parseStringList(const YAML::Node& node) {
    std::vector<std::string> result = {};

    if (!node || !node.IsSequence()) {
      return result;
    }
    for (const auto& item : node) {
        result.push_back(item.as<std::string>());
    }
    return result;
}

ModeSpec parseModeSpec(const YAML::Node& node) {
    ModeSpec spec;
    spec.id          = safeAs<std::string>(node["id"],          "");
    spec.description = safeAs<std::string>(node["description"], "");
    spec.model_id    = safeAs<std::string>(node["model"],       "default");
    spec.lora_adapter_id = safeAs<std::string>(node["lora_adapter"], "");
    spec.mode_id     = modeIdFromString(spec.id);

    spec.tools_allowed = parseStringList(node["tools_allowed"]);
    spec.tools_denied  = parseStringList(node["tools_denied"]);

    if (node["retrieval"]) {
      spec.retrieval    = parseRetrieval(node["retrieval"]);
    }
    if (node["output"]) {
      spec.output       = parseOutput(node["output"]);
    }
    if (node["budgets"]) {
      spec.budgets      = parseBudgets(node["budgets"]);
    }
    if (node["observability"]) {
      spec.observability = parseObservability(node["observability"]);
    }

    if (node["judge"] && node["judge"].IsMap()) {
        const auto& j = node["judge"];
        spec.judge.enabled   = safeAs<bool>(j["enabled"],    false);
        spec.judge.model_id  = safeAs<std::string>(j["model"], "");
        spec.judge.min_score = safeAs<float>(j["min_score"], 0.6f);
    }
    if (node["safety"] && node["safety"].IsMap()) {
        const auto& s = node["safety"];
        spec.safety.enabled        = safeAs<bool>(s["enabled"],         false);
        spec.safety.ethics_profile = safeAs<std::string>(s["ethics_profile"], "");
    }

    return spec;
}

ModelEntry parseModelEntry(const YAML::Node& node) {
    ModelEntry entry;
    entry.id         = safeAs<std::string>(node["id"],         "default");
    entry.path       = safeAs<std::string>(node["path"],       "");
    entry.gpu_layers = safeAs<int>(node["gpu_layers"],         0);
    entry.n_ctx      = safeAs<int>(node["n_ctx"],              4096);
    return entry;
}

ModePack parseModePack(const YAML::Node& root) {
    ModePack pack;
    pack.apiVersion    = safeAs<std::string>(root["apiVersion"], "");
    pack.kind          = safeAs<std::string>(root["kind"],       "");
    pack.default_mode  = safeAs<std::string>(root["default_mode"], "ask");

    if (root["metadata"] && root["metadata"].IsMap()) {
        pack.name    = safeAs<std::string>(root["metadata"]["name"],    "");
        pack.version = safeAs<std::string>(root["metadata"]["version"], "1.0.0");
    }

    if (root["models"] && root["models"].IsSequence()) {
        for (const auto& m : root["models"]) {
            pack.models.push_back(parseModelEntry(m));
        }
    }
    if (pack.models.empty()) {
        // Inject default model placeholder
        ModelEntry def;
        def.id = "default";
        pack.models.push_back(def);
    }

    if (root["tools"] && root["tools"].IsSequence()) {
        for (const auto& t : root["tools"]) {
            pack.tools.push_back(parseToolSpec(t));
        }
    }

    if (root["modes"] && root["modes"].IsSequence()) {
        for (const auto& m : root["modes"]) {
            pack.modes.push_back(parseModeSpec(m));
        }
    }

    return pack;
}

} // namespace

// ============================================================================
// ModeSpecLoader
// ============================================================================

ModePack ModeSpecLoader::loadFromFile(const std::string& path,
                                      ValidationResult*  result_out) {
    ValidationResult local;
    ModePack pack;

    try {
        YAML::Node root = YAML::LoadFile(path);
        pack = parseModePack(root);
    } catch (const YAML::BadFile& e) {
        local.ok = false;
        local.errors.push_back("Cannot open file '" + path + "': " + e.what());
        spdlog::error("[AIOrchestrator] ModeSpecLoader: {}", local.errors.back());
        if (result_out) {
          *result_out = local;
        }
        return pack;
    } catch (const YAML::Exception& e) {
        local.ok = false;
        local.errors.push_back("YAML parse error in '" + path + "': " + e.what());
        spdlog::error("[AIOrchestrator] ModeSpecLoader: {}", local.errors.back());
        if (result_out) {
          *result_out = local;
        }
        return pack;
    }

    local = validate(pack);
    if (result_out) {
      *result_out = local;
    }

    if (local.ok) {
        spdlog::info("[AIOrchestrator] Loaded ModePack '{}' v{} ({} modes) from '{}'",
                     pack.name, pack.version,static_cast<int>(pack.modes.size()), path);
    }
    return pack;
}

ModePack ModeSpecLoader::loadFromString(const std::string& yaml_text,
                                        ValidationResult*  result_out) {
    ValidationResult local;
    ModePack pack;

    try {
        YAML::Node root = YAML::Load(yaml_text);
        pack = parseModePack(root);
    } catch (const YAML::Exception& e) {
        local.ok = false;
        local.errors.push_back(std::string("YAML parse error: ") + e.what());
        spdlog::error("[AIOrchestrator] ModeSpecLoader: {}", local.errors.back());
        if (result_out) {
          *result_out = local;
        }
        return pack;
    }

    local = validate(pack);
    if (result_out) {
      *result_out = local;
    }
    return pack;
}

ValidationResult ModeSpecLoader::validate(const ModePack& pack) {
    ValidationResult res;
    auto err = [&](const std::string& msg) {
        res.ok = false;
        res.errors.push_back(msg);
        spdlog::error("[AIOrchestrator] Validation error: {}", msg);
    };
    auto warn = [&](const std::string& msg) {
        res.warnings.push_back(msg);
        spdlog::warn("[AIOrchestrator] Validation warning: {}", msg);
    };

    // apiVersion check
    if (pack.apiVersion.empty()) {
        warn("'apiVersion' is missing; expected 'themis.ai/v1'");
    } else if (pack.apiVersion != "themis.ai/v1") {
        err("Unsupported apiVersion '" + pack.apiVersion + "'; only 'themis.ai/v1' is supported");
    }

    // kind check
    const bool validKind = (pack.kind == "ThemisModePack" || pack.kind == "ThemisAIPolicy");
    if (pack.kind.empty()) {
        warn("'kind' is missing; expected 'ThemisModePack' or 'ThemisAIPolicy'");
    } else if (!validKind) {
        err("Unknown kind '" + pack.kind + "'; expected 'ThemisModePack' or 'ThemisAIPolicy'");
    }

    // At least one mode must be defined
    if (pack.modes.empty()) {
        err("'modes' list is empty; at least one mode must be defined");
    }

    // Mode id uniqueness and field checks
    std::unordered_map<std::string, int> seenIds = {};

    for (const auto& mode : pack.modes) {
        if (mode.id.empty()) {
            err("A mode entry is missing required field 'id'");
            continue;
        }
        if (++seenIds[mode.id] > 1) {
            err("Duplicate mode id '" + mode.id + "'");
        }
        // Budget sanity
        if (mode.budgets.max_tokens <= 0) {
            err("Mode '" + mode.id + "': budgets.max_tokens must be > 0");
        }
        if (mode.budgets.timeout_ms <= 0) {
            err("Mode '" + mode.id + "': budgets.timeout_ms must be > 0");
        }
        if (mode.budgets.temperature < 0.0f || mode.budgets.temperature > 2.0f) {
            warn("Mode '" + mode.id + "': budgets.temperature " +
                 std::to_string(mode.budgets.temperature) + " is outside [0, 2]");
        }
        // Retrieval sanity
        if (mode.retrieval.enabled) {
            if (mode.retrieval.top_k <= 0) {
                err("Mode '" + mode.id + "': retrieval.top_k must be > 0 when retrieval is enabled");
            }
            const std::vector<std::string> valid_strategies = {"vector", "fulltext", "hybrid"};
            const auto& strat = mode.retrieval.strategy;
            if (std::find(valid_strategies.begin(), valid_strategies.end(), strat) == valid_strategies.end()) {
                warn("Mode '" + mode.id + "': unknown retrieval.strategy '" + strat + "'");
            }
        }
        // Ethics safety check
        if (mode.safety.enabled && mode.safety.ethics_profile.empty()) {
            warn("Mode '" + mode.id + "': safety.enabled=true but safety.ethics_profile is empty");
        }
    }

    // default_mode must exist
    if (!pack.default_mode.empty()) {
        bool found = false;
        for (const auto& m : pack.modes) {
            if (m.id == pack.default_mode) { found = true; break; }
        }
        if (!found) {
            err("default_mode '" + pack.default_mode + "' is not defined in modes[]");
        }
    }

    // Tool name uniqueness
    std::unordered_map<std::string, int> seenTools = {};

    for (const auto& tool : pack.tools) {
        if (tool.name.empty()) {
            err("A tool entry is missing required field 'name'");
            continue;
        }
        if (++seenTools[tool.name] > 1) {
            err("Duplicate tool name '" + tool.name + "'");
        }
    }

    // Warn if tools_allowed references an unknown tool (skip the "*" wildcard)
    for (const auto& mode : pack.modes) {
        for (const auto& ta : mode.tools_allowed) {
            if (ta == "*") continue;  // wildcard – permits all registered tools at runtime
            if (seenTools.find(ta) == seenTools.end()) {
                warn("Mode '" + mode.id + "': tools_allowed references unknown tool '" + ta + "'");
            }
        }
    }

    return res;
}

} // namespace themis::llm


