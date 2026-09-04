/**
 * @file task_decomposer.cpp
 * @brief Prompt-enhancement-based task decomposition for LLM inference orchestration.
 *
 * Implements TaskDecomposer and the WorkflowDefinition helper methods
 * (WorkflowDefinition::stepById, WorkflowDefinition::topologicalOrder) and
 * WorkflowLoader (YAML/JSON/BPMN-lite parsing and validation).
 *
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 */

#include "llm/task_decomposer.h"
#include "llm/workflow_definition.h"
#include "llm/llm_plugin_interface.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis::llm {

// ============================================================================
// WorkflowStepMode helpers
// ============================================================================

WorkflowStepMode workflowStepModeFromString(const std::string& s) {
    std::string lc = s;
    std::transform(lc.begin(), lc.end(), lc.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lc == "ask") {
      return WorkflowStepMode::Ask;
    }
    if (lc == "rag") {
      return WorkflowStepMode::Rag;
    }
    if (lc == "agentic") {
      return WorkflowStepMode::Agentic;
    }
    if (lc == "ethics") {
      return WorkflowStepMode::Ethics;
    }
    return WorkflowStepMode::Custom;
}

std::string workflowStepModeToString(WorkflowStepMode m) {
    switch (m) {
        case WorkflowStepMode::Ask:     return "ask";
        case WorkflowStepMode::Rag:     return "rag";
        case WorkflowStepMode::Agentic: return "agentic";
        case WorkflowStepMode::Ethics:  return "ethics";
        default:                        return "custom";
    }
}

// ============================================================================
// WorkflowDefinition helpers
// ============================================================================

const WorkflowStep& WorkflowDefinition::stepById(const std::string& step_id) const {
    for (const auto& step : steps) {
        if (step.id == step_id) {
          return step;
        }
    }
    throw std::out_of_range("WorkflowDefinition::stepById: unknown step id '" + step_id + "'");
}

std::vector<const WorkflowStep*> WorkflowDefinition::topologicalOrder() const {
    // Kahn's algorithm — returns empty on cycle
    std::unordered_map<std::string, int> in_degree;
    std::unordered_map<std::string, std::vector<std::string>> adjacency;

    for (const auto& step : steps) {
        if (!in_degree.count(step.id)) {
          in_degree[step.id] = 0;
        }
        for (const auto& dep : step.depends_on) {
            adjacency[dep].push_back(step.id);
            ++in_degree[step.id];
        }
    }

    std::vector<std::string> queue = {};

    for (const auto& entry : in_degree) {
        const std::string& node_id = entry.first;
        const int degree = entry.second;
        if (degree == 0) {
          queue.push_back(node_id);
        }
    }

    std::vector<const WorkflowStep*> result = {};

    while (!queue.empty()) {
        const std::string cur = queue.back();
        queue.pop_back();
        try {
            result.push_back(&stepById(cur));
        } catch (...) {
            return {}; // unknown step — definition is invalid
        }
        if (adjacency.count(cur)) {
            for (const auto& next : adjacency.at(cur)) {
                if (--in_degree[next] == 0) {
                  queue.push_back(next);
                }
            }
        }
    }
    if (static_cast<int>(result.size()) != steps.size()) return {}; // cycle detected
    return result;
}

// ============================================================================
// WorkflowLoader — JSON serialisation
// ============================================================================

json WorkflowLoader::toJson(const WorkflowDefinition& def) {
    json j;
    j["id"]            = def.id;
    j["name"]          = def.name;
    j["description"]   = def.description;
    j["default_mode"]  = workflowStepModeToString(def.default_mode);
    j["source_format"] = def.source_format;
    if (!def.extensions.is_null()) {
      j["extensions"] = def.extensions;
    }

    json ctx = json::object();
    for (const auto& [k, v] : def.initial_context) {
      ctx[k] = v;
    }
    j["initial_context"] = ctx;

    json steps_arr = json::array();
    for (const auto& step : def.steps) {
        json s;
        s["id"]              = step.id;
        s["description"]     = step.description;
        s["prompt_template"] = step.prompt_template;
        s["mode"]            = workflowStepModeToString(step.mode);
        s["depends_on"]      = step.depends_on;
        s["max_tokens"]      = step.max_tokens;
        s["temperature"]     = step.temperature;
        if (step.output_schema.has_value()) {
          s["output_schema"] = *step.output_schema;
        }
        if (!step.extensions.is_null()) {
          s["extensions"] = step.extensions;
        }
        steps_arr.push_back(s);
    }
    j["steps"] = steps_arr;
    return j;
}

WorkflowDefinition WorkflowLoader::fromJson(const json& j) {
    WorkflowDefinition def;
    def.source_format = "json";

    auto req = [&]([[maybe_unused]] const char* key) -> const json& {
        if (!j.contains(key))
            throw std::runtime_error(std::string("WorkflowLoader::fromJson: missing key '") + key + "'");
        return j.at(key);
    };

    def.id          = req("id").get<std::string>();
    def.name        = j.value("name", def.id);
    def.description = j.value("description", std::string{});
    def.default_mode = workflowStepModeFromString(j.value("default_mode", std::string{"ask"}));

    if (j.contains("initial_context") && j["initial_context"].is_object()) {
        for (auto& [k, v] : j["initial_context"].items()) {
            def.initial_context[k] = v.get<std::string>();
        }
    }

    if (j.contains("extensions")) {
      def.extensions = j["extensions"];
    }

    if (j.contains("steps") && j["steps"].is_array()) {
        for (const auto& s : j["steps"]) {
            WorkflowStep step;
            step.id              = s.value("id", std::string{});
            step.description     = s.value("description", std::string{});
            step.prompt_template = s.value("prompt_template", std::string{});
            step.mode            = workflowStepModeFromString(
                                       s.value("mode", workflowStepModeToString(def.default_mode)));
            if (s.contains("depends_on") && s["depends_on"].is_array())
                step.depends_on = s["depends_on"].get<std::vector<std::string>>();
            step.max_tokens  = s.value("max_tokens", 0);
            step.temperature = s.value("temperature", -1.0f);
            if (s.contains("output_schema")) {
              step.output_schema = s["output_schema"];
            }
            if (s.contains("extensions")) {
              step.extensions    = s["extensions"];
            }
            def.steps.push_back(std::move(step));
        }
    }
    return def;
}

// ============================================================================
// WorkflowLoader — YAML parsing (via nlohmann/json YAML-like subset)
//
// Full yaml-cpp integration is optional.  We implement a small inline YAML
// parser sufficient for the workflow schema (mappings, sequences, scalars).
// ============================================================================

namespace {

// Minimal YAML → JSON converter for the workflow schema.
// Supports: mappings, sequences, plain scalars, quoted strings, block lists.
// Not a general-purpose YAML parser; limited to workflow document structure.

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static json parseYamlScalar(const std::string& raw) {
    const std::string s = trim(raw);
    if (s.empty()) {
      return nullptr;
    }
    if (s == "true"  || s == "yes") {
      return true;
    }
    if (s == "false" || s == "no") {
      return false;
    }
    if (s == "null"  || s == "~") {
      return nullptr;
    }

    // Quoted string
    if ((s.front() == '"' && s.back() == '"') ||
        (s.front() == '\'' && s.back() == '\''))
        return s.substr(1, static_cast<int>(s.size()) - 2);

    // Number
    try {
        size_t pos = 0;
        long long iv = std::stoll(s, &pos);
        if (pos == s.size()) {
          return iv;
        }
    } catch (...) {}
    try {
        size_t pos = 0;
        double dv = std::stod(s, &pos);
        if (pos == s.size()) {
          return dv;
        }
    } catch (...) {}

    return s; // plain string
}

// Very small line-oriented YAML → JSON converter.
// Handles the workflow YAML subset:
//   id: value
//   name: value
//   steps:
//     - id: step1
//       prompt_template: "..."
//       depends_on: [step0]
json simpleYamlToJson(const std::string& yaml_text) {
    std::istringstream stream(yaml_text);
    std::string line = {};
    json root = json::object();
    json* cur_mapping = &root;
    std::string cur_sequence_key = {};
    json* cur_sequence = nullptr;
    json* cur_seq_item = nullptr;
    int   seq_item_indent = -1;
    std::string multiline_key = {};
    std::string multiline_buf = {};
    int  multiline_indent = -1;

    auto flush_multiline = [&]() {
        if (!multiline_key.empty() && cur_mapping) {
            (*cur_mapping)[multiline_key] = trim(multiline_buf);
            multiline_key.clear();
            multiline_buf.clear();
            multiline_indent = -1;
        }
    };

    while (std::getline(stream, line)) {
        // Strip comments
        auto ci = line.find('#');
        if (ci != std::string::npos) {
            // only strip if not inside quoted string (best-effort)
            bool in_q = false;
            char q_char = 0;
            for (size_t i = 0; i < ci; ++i) {
                if (!in_q && (line[i] == '"' || line[i] == '\'')) { in_q = true; q_char = line[i]; }
                else if (in_q && line[i] == q_char)                   in_q = false;
            }
            if (!in_q) {
              line = line.substr(0, ci);
            }
        }

        const std::string trimmed = trim(line);
        if (trimmed.empty()) {
          continue;
        }

        // Determine indent level
        int indent = 0;
        while (indent < static_cast<int>(line.size()) && (line[indent] == ' ' || line[indent] == '\t'))
            ++indent;

        // Multiline scalar continuation
        if (!multiline_key.empty() && indent > multiline_indent) {
            multiline_buf += (multiline_buf.empty() ? "" : "\n") + trimmed;
            continue;
        }
        flush_multiline();

        // Sequence item
        if (trimmed.front() == '-') {
            const std::string item_content = trim(trimmed.substr(1));
            if (cur_sequence) {
                cur_sequence->push_back(json::object());
                cur_seq_item = &cur_sequence->back();
                seq_item_indent = indent;
                if (!item_content.empty()) {
                    // Inline key: value on the same line as '-'
                    auto colon = item_content.find(':');
                    if (colon != std::string::npos) {
                        const std::string k = trim(item_content.substr(0, colon));
                        const std::string v = trim(item_content.substr(colon + 1));
                        if (!v.empty())
                            (*cur_seq_item)[k] = parseYamlScalar(v);
                    }
                }
            }
            continue;
        }

        // Key: value pair
        auto colon = trimmed.find(':');
        if (colon == std::string::npos) {
          continue;
        }

        const std::string key   = trim(trimmed.substr(0, colon));
        const std::string value = trim(trimmed.substr(colon + 1));

        // Determine target mapping
        json* target = &root;
        if (cur_seq_item && indent > seq_item_indent)
            target = cur_seq_item;
        else
            cur_seq_item = nullptr;

        if (value.empty()) {
            // Could be: start of a sequence or mapping block
            // Peek ahead logic simplified: just create a placeholder
            // The next '-' lines will fill it.
            (*target)[key] = json::array();
            cur_sequence     = &(*target)[key];
            cur_sequence_key = key;
            cur_mapping      = target;
        } else if (value.front() == '[') {
            // Inline sequence
            json arr = json::array();
            std::string inner = trim(value.substr(1));
            if (!inner.empty() && inner.back() == ']') {
              inner.pop_back();
            }
            std::istringstream ss(inner);
            std::string tok = {};
            while (std::getline(ss, tok, ',')) {
              arr.push_back(parseYamlScalar(trim(tok)));
            }
            (*target)[key] = arr;
        } else if (value == "|" || value == ">") {
            // Block scalar — collect continuation lines
            multiline_key    = key;
            multiline_indent = indent;
            multiline_buf.clear();
            cur_mapping = target;
        } else {
            (*target)[key] = parseYamlScalar(value);
        }
    }
    flush_multiline();
    return root;
}

} // anonymous namespace

// ============================================================================
// WorkflowLoader — format dispatch
// ============================================================================

WorkflowDefinition WorkflowLoader::parseYaml(const std::string& content,
                                              const std::string& /*label*/) {
    json j = simpleYamlToJson(content);
    WorkflowDefinition def = fromJson(j);
    def.source_format = "yaml";
    return def;
}

WorkflowDefinition WorkflowLoader::parseJson(const std::string& content,
                                              const std::string& label) {
    json j;
    try {
        j = json::parse(content);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("WorkflowLoader: JSON parse error in '" + label + "': " + e.what());
    }
    return fromJson(j);
}

WorkflowDefinition WorkflowLoader::parseBpmn(const std::string& content,
                                              const std::string& label) {
    // Simplified BPMN-lite XML parser.
    // Extracts <serviceTask id="..." name="..."> elements and
    // <sequenceFlow sourceRef="..." targetRef="..."> flow edges.
    // prompt_template and mode are read from
    // <extensionElements><themis:meta prompt="..." mode="..."/></extensionElements>
    //
    // Only the subset needed for workflow execution is parsed; gateways,
    // events, and data objects are intentionally ignored.

    WorkflowDefinition def;
    def.source_format = "bpmn";

    // Build dependency map: targetRef → [sourceRef]
    std::unordered_map<std::string, std::vector<std::string>> deps;

    // Extract serviceTask entries
    auto extractAttr = [&](const std::string& xml, size_t start,
                            const std::string& attr) -> std::string {
        const std::string search = attr + "=\"";
        size_t pos = xml.find(search, start);
        if (pos == std::string::npos) return {};
        pos += search.size();
        size_t end = xml.find('"', pos);
        if (end == std::string::npos) return {};
        return xml.substr(pos, end - pos);
    };

    size_t pos = 0;
    while (static_cast<size_t>(pos) <static_cast<int>(content.size())) {
        size_t tag_start = content.find('<', pos);
        if (tag_start == std::string::npos) {
          break;
        }
        size_t tag_end = content.find('>', tag_start);
        if (tag_end == std::string::npos) {
          break;
        }

        const std::string tag = content.substr(tag_start + 1, tag_end - tag_start - 1);

        if (tag.find("serviceTask") != std::string::npos) {
            WorkflowStep step;
            step.id          = extractAttr(tag, 0, "id");
            step.description = extractAttr(tag, 0, "name");
            step.mode        = def.default_mode;

            // Look for extensionElements with themis:meta inside this task
            size_t ext_start = content.find("<extensionElements", tag_end);
            size_t ext_end   = content.find("</extensionElements>", tag_end);
            if (ext_start != std::string::npos && ext_end != std::string::npos) {
                const std::string ext = content.substr(ext_start, ext_end - ext_start);
                std::string pt = extractAttr(ext, 0, "prompt");
                if (!pt.empty()) {
                  step.prompt_template = pt;
                }
                std::string mode_str = extractAttr(ext, 0, "mode");
                if (!mode_str.empty()) {
                  step.mode = workflowStepModeFromString(mode_str);
                }
            }

            if (step.prompt_template.empty())
                step.prompt_template = "{input}"; // fallback: pass input through

            if (!step.id.empty()) {
              def.steps.push_back(std::move(step));
            }
        } else if (tag.find("sequenceFlow") != std::string::npos) {
            const std::string src = extractAttr(tag, 0, "sourceRef");
            const std::string tgt = extractAttr(tag, 0, "targetRef");
            if (!src.empty() && !tgt.empty()) {
              deps[tgt].push_back(src);
            }
        }

        pos = tag_end + 1;
    }

    // Apply dependency edges
    for (auto& step : def.steps) {
        if (deps.count(step.id)) {
          step.depends_on = deps.at(step.id);
        }
    }

    if (def.id.empty()) {
      def.id = label;
    }
    if (def.name.empty()) {
      def.name = label;
    }
    return def;
}

WorkflowDefinition WorkflowLoader::loadFromString(const std::string& content,
                                                    const std::string& format_hint,
                                                    const std::string& source_label) {
    std::string fmt = format_hint;
    std::transform(fmt.begin(), fmt.end(), fmt.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (fmt == "yaml" || fmt == "yml") {
      return parseYaml(content, source_label);
    }
    if (fmt == "json") {
      return parseJson(content, source_label);
    }
    if (fmt == "bpmn" || fmt == "xml") {
      return parseBpmn(content, source_label);
    }
    throw std::runtime_error("WorkflowLoader: unsupported format hint '" + format_hint + "'");
}

WorkflowDefinition WorkflowLoader::loadFromFile(const std::string& path) {
    std::ifstream f(path, std::ios::in);
    if (!f) {
      throw std::runtime_error("WorkflowLoader: cannot open file '" + path + "'");
    }
    std::ostringstream ss = {};
    ss << f.rdbuf();
    const std::string content = ss.str();

    // Detect format from extension
    std::string ext = {};
    const size_t dot = path.rfind('.');
    if (dot != std::string::npos) {
      ext = path.substr(dot + 1);
    }
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::string fmt = "yaml";
    if (ext == "json") {
      fmt = "json";
    }
    else if (ext == "bpmn" || ext == "xml") fmt = "bpmn";

    return loadFromString(content, fmt, path);
}

// ============================================================================
// WorkflowLoader — validation
// ============================================================================

WorkflowValidationResult WorkflowLoader::validate(const WorkflowDefinition& def) {
    WorkflowValidationResult result = {};

    if (def.id.empty()) {
      result.addError("", "workflow 'id' must not be empty");
    }

    std::unordered_set<std::string> ids = {};

    for (const auto& step : def.steps) {
        if (step.id.empty()) {
            result.addError("", "a step has an empty 'id'");
            continue;
        }
        if (!ids.insert(step.id).second)
            result.addError(step.id, "duplicate step id '" + step.id + "'");
        if (step.prompt_template.empty())
            result.addError(step.id, "prompt_template must not be empty");
    }

    // Check depends_on references
    for (const auto& step : def.steps) {
        for (const auto& dep : step.depends_on) {
            if (!ids.count(dep))
                result.addError(step.id, "depends_on references unknown step '" + dep + "'");
        }
    }

    // Cycle detection via topological sort
    if (!def.steps.empty() && def.topologicalOrder().empty())
        result.addError("", "workflow dependency graph contains a cycle");

    return result;
}

// ============================================================================
// DecompositionStrategy helper
// ============================================================================

std::string decompositionStrategyToString(DecompositionStrategy s) {
    switch (s) {
        case DecompositionStrategy::ChainOfThought: return "chain_of_thought";
        case DecompositionStrategy::DirectJson:     return "direct_json";
        case DecompositionStrategy::FewShot:        return "few_shot";
    }
    return "chain_of_thought";
}

// ============================================================================
// TaskDecomposer — Impl
// ============================================================================

struct TaskDecomposer::Impl {
    TaskDecomposerConfig              config;
    std::shared_ptr<ILLMPlugin>       plugin;
    mutable std::mutex                mu;
};

TaskDecomposer::TaskDecomposer(const TaskDecomposerConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

TaskDecomposer::~TaskDecomposer() = default;

TaskDecomposer::TaskDecomposer(TaskDecomposer&&) noexcept            = default;
TaskDecomposer& TaskDecomposer::operator=(TaskDecomposer&&) noexcept = default;

void TaskDecomposer::setLLMPlugin(std::shared_ptr<ILLMPlugin> plugin) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    impl_->plugin = std::move(plugin);
}

void TaskDecomposer::setConfig(const TaskDecomposerConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    impl_->config = config;
}

TaskDecomposerConfig TaskDecomposer::config() const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    return impl_->config;
}

// ── Prompt builders ──────────────────────────────────────────────────────────

static constexpr const char* kJsonSchema = R"(
Return ONLY a JSON array (no markdown, no commentary) where each element is:
{
  "id": "<short_snake_case_id>",
  "description": "<one sentence>",
  "prompt": "<ready-to-use prompt for this subtask>",
  "depends_on": ["<id_of_prerequisite>", ...]
}
)";

std::string TaskDecomposer::buildChainOfThoughtPrompt(
    const std::string& task, const std::string& extra_ctx) const {
    const auto& cfg = impl_->config;
    std::ostringstream p = {};
    p << "You are a task-planning assistant";
    if (!cfg.domain_context.empty()) {
      p << " specialising in " << cfg.domain_context;
    }
    p << ".\n\n";
    p << "Think step by step, then break the following task into at most "
      << (cfg.max_subtasks > 0 ? cfg.max_subtasks : 8) << " concrete, independently "
      << "executable subtasks.  Order subtasks logically and express any prerequisite "
         "ordering via the 'depends_on' field.\n";
    if (!cfg.output_language.empty())
        p << "Write subtask descriptions in " << cfg.output_language << ".\n";
    p << "\nTask: " << task;
    if (!extra_ctx.empty()) {
      p << "\n\nAdditional context: " << extra_ctx;
    }
    p << "\n\nAfter your reasoning, output " << kJsonSchema;
    return p.str();
}

std::string TaskDecomposer::buildDirectJsonPrompt(
    const std::string& task, const std::string& extra_ctx) const {
    const auto& cfg = impl_->config;
    std::ostringstream p = {};
    p << "Decompose the following task into at most "
      << (cfg.max_subtasks > 0 ? cfg.max_subtasks : 8) << " subtasks";
    if (!cfg.domain_context.empty()) {
      p << " (domain: " << cfg.domain_context << ")";
    }
    p << ".\n\nTask: " << task;
    if (!extra_ctx.empty()) {
      p << "\nContext: " << extra_ctx;
    }
    p << "\n" << kJsonSchema;
    return p.str();
}

std::string TaskDecomposer::buildFewShotPrompt(
    const std::string& task, const std::string& extra_ctx) const {
    const auto& cfg = impl_->config;
    std::ostringstream p = {};
    p << "You are a task-planning assistant";
    if (!cfg.domain_context.empty()) {
      p << " specialising in " << cfg.domain_context;
    }
    p << ". Here are examples of task decompositions:\n\n";

    for (const auto& ex : cfg.few_shot_examples) {
        p << "Task: " << ex.task << "\n";
        json arr = json::array();
        for (const auto& s : ex.steps) {
          arr.push_back(s);
        }
        p << arr.dump(2) << "\n\n";
    }

    p << "Now decompose the following task into at most "
      << (cfg.max_subtasks > 0 ? cfg.max_subtasks : 8) << " subtasks.\n";
    if (!cfg.output_language.empty())
        p << "Write descriptions in " << cfg.output_language << ".\n";
    p << "\nTask: " << task;
    if (!extra_ctx.empty()) {
      p << "\nContext: " << extra_ctx;
    }
    p << "\n" << kJsonSchema;
    return p.str();
}

std::string TaskDecomposer::buildDecompositionPrompt(
    const std::string& task, const std::string& extra_ctx) const {
    switch (impl_->config.strategy) {
        case DecompositionStrategy::ChainOfThought:
            return buildChainOfThoughtPrompt(task, extra_ctx);
        case DecompositionStrategy::DirectJson:
            return buildDirectJsonPrompt(task, extra_ctx);
        case DecompositionStrategy::FewShot:
            return buildFewShotPrompt(task, extra_ctx);
    }
    return buildChainOfThoughtPrompt(task, extra_ctx);
}

// ── JSON parsing ─────────────────────────────────────────────────────────────

std::vector<SubTask> TaskDecomposer::parseSubtasksFromJson(const json& arr) const {
    std::vector<SubTask> result;
    const auto& cfg = impl_->config;

    for (const auto& item : arr) {
        if (!item.is_object()) {
          continue;
        }
        SubTask sub;
        sub.id          = item.value("id", std::string{});
        sub.description = item.value("description", std::string{});
        sub.prompt      = item.value("prompt", std::string{});
        if (item.contains("depends_on") && item["depends_on"].is_array())
            sub.depends_on = item["depends_on"].get<std::vector<std::string>>();
        sub.raw = item;

        // Assign a synthetic id if the LLM omitted it
        if (sub.id.empty())
            sub.id = "step_" + std::to_string(static_cast<int>(result.size()) + 1);
        // Use description as prompt fallback
        if (sub.prompt.empty()) {
          sub.prompt = sub.description;
        }

        result.push_back(std::move(sub));
        if (cfg.max_subtasks > 0  && static_cast<size_t>(static_cast) < int>(result.size()) >= cfg.max_subtasks)
            break;
    }
    return result;
}

TaskDecompositionResult TaskDecomposer::parseResponse(
    const std::string& raw_response, const std::string& prompt) const {
    TaskDecompositionResult result;
    result.decomposition_prompt = prompt;
    result.raw_llm_response     = raw_response;

    // Extract the first JSON array from the response text
    // (LLM may prepend chain-of-thought reasoning)
    const size_t start = raw_response.find('[');
    const size_t end   = raw_response.rfind(']');
    if (start == std::string::npos || end == std::string::npos || end < start) {
        result.error = "LLM response contains no JSON array";
        return result;
    }

    json arr;
    try {
        arr = json::parse(raw_response.substr(start, end - start + 1));
    } catch (const json::parse_error& e) {
        result.error = std::string("JSON parse error: ") + e.what();
        return result;
    }

    if (!arr.is_array()) {
        result.error = "Parsed JSON is not an array";
        return result;
    }

    result.subtasks = parseSubtasksFromJson(arr);
    const int min_req = impl_->config.min_subtasks;
    if (min_req > 0  && static_cast<size_t>(static_cast) < int>(result.subtasks.size()) < min_req) {
        result.error = "Too few subtasks returned (" +
                       std::to_string(result.subtasks.size()) + " < " +
                       std::to_string(min_req) + ")";
        return result;
    }

    result.success = true;
    return result;
}

// ── decompose() ──────────────────────────────────────────────────────────────

TaskDecompositionResult TaskDecomposer::decompose(
    const std::string& task, const std::string& extra_ctx) const {

    std::shared_ptr<ILLMPlugin> plugin;
    TaskDecomposerConfig cfg;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        plugin = impl_->plugin;
        cfg    = impl_->config;
    }

    if (!plugin) {
        TaskDecompositionResult err;
        err.error = "No LLM plugin set on TaskDecomposer";
        return err;
    }

    const std::string prompt = buildDecompositionPrompt(task, extra_ctx);

    TaskDecompositionResult final_result;
    int attempts = 0;

    for (int attempt = 0; attempt <= cfg.max_retries; ++attempt) {
        InferenceRequest req;
        req.prompt      = (attempt == 0) ? prompt
                                         : prompt + "\n\n[Retry: please return a valid JSON array]";
        req.max_tokens  = cfg.max_tokens > 0 ? cfg.max_tokens : 512;
        req.temperature = cfg.temperature >= 0.0f ? cfg.temperature : 0.2f;
        req.system_prompt = std::string(
            "You are a structured planning assistant.  "
            "Always output valid JSON arrays as instructed.");

        InferenceResponse resp = plugin->generate(req);
        ++attempts;

        final_result = parseResponse(resp.success ? resp.text : "", prompt);
        final_result.llm_calls_made    = attempts;
        final_result.tokens_consumed  += static_cast<size_t>(resp.tokens_generated);

        if (final_result.success) {
          break;
        }
        // retry
    }

    return final_result;
}

// ── toWorkflow() ─────────────────────────────────────────────────────────────

WorkflowDefinition TaskDecomposer::toWorkflow(
    const TaskDecompositionResult& result, const std::string& workflow_id) {

    if (!result.success)
        throw std::invalid_argument("TaskDecomposer::toWorkflow: result.success == false");

    WorkflowDefinition def;
    def.id            = workflow_id;
    def.name          = workflow_id;
    def.source_format = "decomposed";

    for (const auto& sub : result.subtasks) {
        WorkflowStep step;
        step.id              = sub.id;
        step.description     = sub.description;
        step.prompt_template = sub.prompt.empty() ? sub.description : sub.prompt;
        step.depends_on      = sub.depends_on;
        // Honour optional "mode" field from LLM output
        if (sub.raw.contains("mode"))
            step.mode = workflowStepModeFromString(sub.raw.value("mode", std::string{"ask"}));
        step.extensions = sub.raw;
        def.steps.push_back(std::move(step));
    }
    return def;
}

} // namespace themis::llm
