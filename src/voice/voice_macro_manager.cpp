/**
 * @file voice_macro_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=21, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/voice_macro.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace themis {
namespace voice {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/** Normalise text for trigger matching: lower-case, strip leading/trailing whitespace. */
std::string normalise(const std::string& s) {
    std::string out = {};
    out.reserve(s.size());
    for (unsigned char c : s) {
        out.push_back(static_cast<char>(std::tolower(c)));
    }
    // strip leading/trailing spaces
    size_t start = out.find_first_not_of(' ');
    if (start == std::string::npos) {
      return "";
    }
    size_t end = out.find_last_not_of(' ');
    return out.substr(start, end - start + 1);
}

/** Generate a simple time-based unique ID. */
std::string generateID() {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::ostringstream ss = {};
    ss << "macro:" << std::hex << now;
    return ss.str();
}

int64_t macroNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// ---------------------------------------------------------------------------
// JSON helpers for MacroStep
// ---------------------------------------------------------------------------

json stepToJson(const MacroStep& step) {
    json j;
    j["type"] = static_cast<int>(step.type);
    j["action"] = step.action;
    json params = json::object();
    for (const auto& kv : step.parameters) {
        params[kv.first] = kv.second;
    }
    j["parameters"] = params;
    json sub = json::array();
    for (const auto& s : step.sub_steps) {
        sub.push_back(stepToJson(s));
    }
    j["sub_steps"] = sub;
    return j;
}

MacroStep stepFromJson(const json& j) {
    MacroStep step;
    step.type   = static_cast<StepType>(j.value("type", 0));
    step.action = j.value("action", "");
    if (j.contains("parameters") && j["parameters"].is_object()) {
        for (auto it = j["parameters"].begin(); it != j["parameters"].end(); ++it) {
            step.parameters[it.key()] = it.value().get<std::string>();
        }
    }
    if (j.contains("sub_steps") && j["sub_steps"].is_array()) {
        for (const auto& s : j["sub_steps"]) {
            step.sub_steps.push_back(stepFromJson(s));
        }
    }
    return step;
}

json macroInfoToJson(const MacroInfo& m) {
    json j;
    j["macro_id"]       = m.macro_id;
    j["name"]           = m.name;
    j["trigger_phrase"] = m.trigger_phrase;
    j["description"]    = m.description;
    j["tags"]           = m.tags;
    j["created_at"]     = m.created_at;
    j["last_used"]      = m.last_used;
    j["use_count"]      = m.use_count;
    j["enabled"]        = m.enabled;

    json steps = json::array();
    for (const auto& s : m.steps) {
        steps.push_back(stepToJson(s));
    }
    j["steps"] = steps;

    json opts;
    opts["require_confirmation"]  = m.options.require_confirmation;
    opts["required_permissions"]  = m.options.required_permissions;
    opts["max_execution_time_ms"] = m.options.max_execution_time_ms;
    opts["log_execution"]         = m.options.log_execution;
    opts["priority"]              = static_cast<int>(m.options.priority);
    j["options"] = opts;

    return j;
}

MacroInfo macroInfoFromJson(const json& j) {
    MacroInfo m;
    m.macro_id       = j.value("macro_id", "");
    m.name           = j.value("name", "");
    m.trigger_phrase = j.value("trigger_phrase", "");
    m.description    = j.value("description", "");
    m.created_at     = j.value("created_at", 0LL);
    m.last_used      = j.value("last_used", 0LL);
    m.use_count      = j.value("use_count", 0);
    m.enabled        = j.value("enabled", true);

    if (j.contains("tags") && j["tags"].is_array()) {
        m.tags = j["tags"].get<std::vector<std::string>>();
    }
    if (j.contains("steps") && j["steps"].is_array()) {
        for (const auto& s : j["steps"]) {
            m.steps.push_back(stepFromJson(s));
        }
    }
    if (j.contains("options") && j["options"].is_object()) {
        const auto& opts     = j["options"];
        m.options.require_confirmation  = opts.value("require_confirmation", false);
        m.options.max_execution_time_ms = opts.value("max_execution_time_ms", 30000);
        m.options.log_execution         = opts.value("log_execution", true);
        m.options.priority              = static_cast<Priority>(opts.value("priority", 1));
        if (opts.contains("required_permissions") && opts["required_permissions"].is_array()) {
            m.options.required_permissions =
                opts["required_permissions"].get<std::vector<std::string>>();
        }
    }
    return m;
}

// ---------------------------------------------------------------------------
// Step execution
// ---------------------------------------------------------------------------

/** Execute a single step and return a StepResult. */
StepResult executeStep(int index,
                       const MacroStep& step,
                       const std::map<std::string, std::string>& runtime_params)
{
    StepResult result;
    result.step_index = index;

    auto t0 = std::chrono::steady_clock::now();

    try {
        switch (step.type) {
        case StepType::QUERY: {
            // Substitute bind variables in the AQL template.
            std::string aql = step.action;
            // Apply step-level parameters first, then runtime overrides.
            auto apply = [&](const std::map<std::string, std::string>& params) {
                for (const auto& kv : params) {
                    std::string token = "@" + kv.first;
                    size_t pos = 0;
                    while ((pos = aql.find(token, pos)) != std::string::npos) {
                        aql.replace(pos,static_cast<int>(token.size()), kv.second);
                        pos += kv.second.size();
                    }
                }
            };
            apply(step.parameters);
            apply(runtime_params);
            result.output  = "AQL: " + aql;
            result.success = true;
            break;
        }
        case StepType::COMMAND: {
            result.output  = "COMMAND: " + step.action;
            result.success = true;
            break;
        }
        case StepType::WAIT: {
            // action stores the delay in ms as a string
            int delay_ms = 0;
            try {
                delay_ms = std::stoi(step.action);
            } catch (const std::invalid_argument&) {
                delay_ms = 0;
            } catch (const std::out_of_range&) {
                delay_ms = 0;
            } catch (const std::string&) {
                delay_ms = 0;
            } catch (const char*) {
                delay_ms = 0;
            } catch (...) {
                delay_ms = 0;
            }
            if (delay_ms > 0 && delay_ms <= 60000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
            result.output  = "Waited " + std::to_string(delay_ms) + " ms";
            result.success = true;
            break;
        }
        case StepType::NOTIFY: {
            result.output  = step.action;
            result.success = true;
            break;
        }
        case StepType::CONDITION:
        [[fallthrough]];
        case StepType::LOOP:
        [[fallthrough]];
        default:
            result.output  = "Step type not yet supported: " + std::to_string(static_cast<int>(step.type));
            result.success = false;
            result.error_message = result.output;
            break;
        }
    } catch (const std::exception& e) {
        result.success       = false;
        result.error_message = e.what();
    }

    auto t1 = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    return result;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Impl (private state)
// ---------------------------------------------------------------------------

struct VoiceMacroManager::Impl {
    mutable std::mutex mutex;
    std::unordered_map<MacroID, MacroInfo> macros;
    uint64_t total_executions = 0;
};

// ---------------------------------------------------------------------------
// VoiceMacroManager
// ---------------------------------------------------------------------------

VoiceMacroManager::VoiceMacroManager()
    : impl_(std::make_unique<Impl>()) {}

VoiceMacroManager::~VoiceMacroManager() = default;

MacroID VoiceMacroManager::createMacro(
    const std::string& trigger_phrase,
    const std::vector<MacroStep>& steps,
    const MacroOptions& options)
{
    if (trigger_phrase.empty()) {
        return {};
    }

    MacroInfo info;
    info.macro_id       = generateID();
    info.trigger_phrase = normalise(trigger_phrase);
    info.name           = trigger_phrase;  // default name == trigger phrase
    info.steps          = steps;
    info.options        = options;
    info.created_at     = macroNowMs();
    info.enabled        = true;

    std::lock_guard<std::mutex> lock(impl_->mutex);
    MacroID id = info.macro_id;
    impl_->macros[id] = std::move(info);
    return id;
}

std::optional<MacroInfo> VoiceMacroManager::getMacro(const MacroID& macro_id) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto it = impl_->macros.find(macro_id);
    if (it == impl_->macros.end()) {
      return std::nullopt;
    }
    return it->second;   // return a copy while the lock is held
}

std::vector<MacroInfo> VoiceMacroManager::listMacros(
    const std::string& /*user_id*/,
    const std::vector<std::string>& tags) const
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    std::vector<MacroInfo> result = {};

    result.reserve(impl_->macros.size());

    for (const auto& kv : impl_->macros) {
        if (tags.empty()) {
            result.push_back(kv.second);
            continue;
        }
        // Include if the macro has at least one of the requested tags.
        const auto& mt = kv.second.tags;
        bool matched = false;
        for (const auto& tag : tags) {
            if (std::find(mt.begin(), mt.end(), tag) != mt.end()) {
                matched = true;
                break;
            }
        }
        if (matched) {
          result.push_back(kv.second);
        }
    }
    return result;
}

bool VoiceMacroManager::setMacroMeta(
    const MacroID& macro_id,
    const std::string& name,
    const std::string& description,
    const std::vector<std::string>& tags,
    bool enabled)
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto it = impl_->macros.find(macro_id);
    if (it == impl_->macros.end()) {
      return false;
    }
    it->second.name        = name;
    it->second.description = description;
    it->second.tags        = tags;
    it->second.enabled     = enabled;
    return true;
}

bool VoiceMacroManager::updateMacro(
    const MacroID& macro_id,
    const std::vector<MacroStep>& steps,
    const MacroOptions& options)
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto it = impl_->macros.find(macro_id);
    if (it == impl_->macros.end()) {
      return false;
    }
    it->second.steps   = steps;
    it->second.options = options;
    return true;
}

bool VoiceMacroManager::deleteMacro(const MacroID& macro_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->macros.erase(macro_id) > 0;
}

MacroResult VoiceMacroManager::executeMacro(
    const MacroID& macro_id,
    const std::map<std::string, std::string>& parameters)
{
    MacroResult result;
    result.macro_id = macro_id;

    MacroInfo info;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        auto it = impl_->macros.find(macro_id);
        if (it == impl_->macros.end()) {
            result.success       = false;
            result.output        = "Macro not found: " + macro_id;
            return result;
        }
        if (!it->second.enabled) {
            result.success = false;
            result.output  = "Macro is disabled";
            return result;
        }
        info = it->second;  // copy for unlocked execution
    }

    auto t0 = std::chrono::steady_clock::now();

    std::string combined_output = {};
    bool all_ok = true;

    for (size_t i = 0; i < info.steps.size(); ++i) {
        auto sr = executeStep(static_cast<int>(i), info.steps[i], parameters);
        if (!combined_output.empty()) {
          combined_output += '\n';
        }
        combined_output += sr.output;
        if (!sr.success) {
            all_ok = false;
        }
        result.step_results.push_back(std::move(sr));
    }

    auto t1 = std::chrono::steady_clock::now();
    result.execution_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    result.success = all_ok;
    result.output  = combined_output;

    // Update usage stats
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        auto it = impl_->macros.find(macro_id);
        if (it != impl_->macros.end()) {
            it->second.use_count++;
            it->second.last_used = macroNowMs();
        }
        impl_->total_executions++;
    }

    return result;
}

MacroID VoiceMacroManager::matchTrigger(const std::string& utterance) const {
    if (utterance.empty()) return {};

    const std::string norm_utt = normalise(utterance);

    std::lock_guard<std::mutex> lock(impl_->mutex);
    for (const auto& kv : impl_->macros) {
        const MacroInfo& m = kv.second;
        if (!m.enabled) {
          continue;
        }
        if (norm_utt.find(m.trigger_phrase) != std::string::npos) {
            return m.macro_id;   // return a copy of the ID while the lock is held
        }
    }
    return {};
}

std::string VoiceMacroManager::exportMacros(const std::vector<MacroID>& macro_ids) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    json arr = json::array();
    if (macro_ids.empty()) {
        for (const auto& kv : impl_->macros) {
            arr.push_back(macroInfoToJson(kv.second));
        }
    } else {
        for (const auto& id : macro_ids) {
            auto it = impl_->macros.find(id);
            if (it != impl_->macros.end()) {
                arr.push_back(macroInfoToJson(it->second));
            }
        }
    }
    return arr.dump(2);
}

std::vector<MacroID> VoiceMacroManager::importMacros(const std::string& json_str) {
    std::vector<MacroID> imported_ids;

    json arr;
    try {
        arr = json::parse(json_str);
    } catch (const nlohmann::json::exception&) {
        return imported_ids;
    } catch (const std::string&) {
        return imported_ids;
    } catch (const char*) {
        return imported_ids;
    } catch (...) {
        return imported_ids;
    }

    if (!arr.is_array()) {
      return imported_ids;
    }

    std::lock_guard<std::mutex> lock(impl_->mutex);
    for (const auto& item : arr) {
        try {
            MacroInfo m = macroInfoFromJson(item);
            if (m.trigger_phrase.empty()) {
              continue;
            }
            if (m.macro_id.empty()) {
                m.macro_id = generateID();
            }
            m.trigger_phrase = normalise(m.trigger_phrase);
            MacroID id = m.macro_id;
            impl_->macros[id] = std::move(m);
            imported_ids.push_back(id);
        } catch (const nlohmann::json::exception&) {
            // Skip malformed entries
        } catch (const std::string&) {
            // Skip malformed entries
        } catch (const char*) {
            // Skip malformed entries
        } catch (...) {
            // Skip malformed entries
        }
    }
    return imported_ids;
}

json VoiceMacroManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    json stats;
    stats["total_macros"]     = impl_->macros.size();
    stats["total_executions"] = impl_->total_executions;
    size_t enabled_count = 0;
    for (const auto& kv : impl_->macros) {
        if (kv.second.enabled) {
          ++enabled_count;
        }
    }
    stats["enabled_macros"] = enabled_count;
    return stats;
}

} // namespace voice
} // namespace themis


