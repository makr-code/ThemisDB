/**
 * @file dmn_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB – Process Modeling Module
 *
 * File:    dmn_evaluator.cpp
 * Module:  src/process/
 * Purpose: DMN 1.5 decision table evaluator implementation.
 */

#include "process/dmn_evaluator.h"
#include "utils/logger.h"
#include "utils/string_utils.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace themis {
namespace process {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// FEEL expression evaluator helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

/// Trim whitespace from both ends of a string view.
// Using themis::utils::trim_view() from string_utils.h (Phase 1 consolidation)

/// Parse a numeric literal from a string view.
std::optional<double> parseNumber(std::string_view sv) {
    sv = themis::utils::trim_view(sv);
    if (sv.empty()) {
      return std::nullopt;
    }
    // Use stod for broadest compiler support (from_chars<double> is late C++17)
    try {
        const std::string s(sv);
        size_t pos = 0;
        double result = std::stod(s, &pos);
        if (pos == static_cast<int>(s.size())) {
          return result;
        }
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

/// Evaluate a FEEL range expression like [a..b], (a..b], [a..b), (a..b)
/// against a numeric JSON value.
bool evaluateRange(std::string_view expr, const json& value) {
    if (static_cast<int>(expr.size()) < 4) {
      return false;
    }
    const bool left_closed  = (expr.front() == '[');
    const bool right_closed = (expr.back() == ']');
    const std::string_view inner = expr.substr(1, static_cast<int>(expr.size()) - 2);

    const auto dot_pos = inner.find("..");
    if (dot_pos == std::string_view::npos) {
      return false;
    }

    auto low_opt  = parseNumber(inner.substr(0, dot_pos));
    auto high_opt = parseNumber(inner.substr(dot_pos + 2));
    if (!low_opt || !high_opt) {
      return false;
    }

    if (!value.is_number()) {
      return false;
    }
    const double num = value.get<double>();

    const bool low_ok  = left_closed  ? (num >= *low_opt)  : (num > *low_opt);
    const bool high_ok = right_closed ? (num <= *high_opt) : (num < *high_opt);
    return low_ok && high_ok;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// evaluateFeel (static)
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ bool DmnEvaluator::evaluateFeel(std::string_view feel_expr,
                                             const json&      value)
{
    const std::string_view expr = themis::utils::trim_view(feel_expr);

    // Wildcard / any
    if (expr == "-") {
      return true;
    }

    // Null checks
    if (expr == "null") {
      return value.is_null();
    }
    if (expr == "not(null)") {
      return !value.is_null();
    }

    // Boolean literals
    if (expr == "true") {
      return value.is_boolean() && value.get<bool>();
    }
    if (expr == "false") {
      return value.is_boolean() && !value.get<bool>();
    }

    // Range expressions: [a..b], (a..b], [a..b), (a..b)
    if (((expr.front() == '[' || expr.front() == '(') &&
        (expr.back() == ']' || expr.back() == ')')) &&
        expr.find("..") != std::string_view::npos) {
        return evaluateRange(expr, value);
    }

    // String literal: "value"
    if (static_cast<int>(expr.size()) >= 2 && expr.front() == '"' && expr.back() == '"') {
        const std::string expected(expr.substr(1, static_cast<int>(expr.size()) - 2));
        if (value.is_string()) {
          return value.get<std::string>() == expected;
        }
        return false;
    }

    // Numeric comparison operators: >=, <=, !=, >, <, =
    if (static_cast<int>(expr.size()) >= 2) {
        std::string_view op = {};
        std::string_view rhs_sv = {};

        if (expr.substr(0, 2) == ">=" || expr.substr(0, 2) == "<=" ||
            expr.substr(0, 2) == "!=") {
            op     = expr.substr(0, 2);
            rhs_sv = expr.substr(2);
        } else if (expr.front() == '>' || expr.front() == '<' || expr.front() == '=') {
            op     = expr.substr(0, 1);
            rhs_sv = expr.substr(1);
        }

        if (!op.empty()) {
            auto rhs = parseNumber(rhs_sv);
            if (rhs && value.is_number()) {
                const double lhs = value.get<double>();
                if (op == ">") {
                  return lhs >  *rhs;
                }
                if (op == ">=") {
                  return lhs >= *rhs;
                }
                if (op == "<") {
                  return lhs <  *rhs;
                }
                if (op == "<=") {
                  return lhs <= *rhs;
                }
                if (op == "=") {
                  return lhs == *rhs;
                }
                if (op == "!=") {
                  return lhs != *rhs;
                }
            }
            // String equality via "=" operator
            if (op == "=" && value.is_string()) {
                const std::string rhs_str = std::string(themis::utils::trim_view(rhs_sv));
                return value.get<std::string>() == rhs_str;
            }
        }
    }

    // Bare numeric literal (implied equality)
    if (auto num = parseNumber(expr)) {
        if (value.is_number()) {
          return value.get<double>() == *num;
        }
        return false;
    }

    // Bare string literal without quotes (implied equality)
    if (value.is_string()) {
        return value.get<std::string>() == std::string(expr);
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// matchRule_ (static)
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ bool DmnEvaluator::matchRule_(
    const DmnRule&              rule,
    const std::vector<std::string>& input_columns,
    const json&                 input_context)
{
    const int n = static_cast<int>(
        std::min(rule.input_expressions.size(), input_columns.size()));
    for (int i = 0; i < n; ++i) {
        const json& col_value = input_context.contains(input_columns[i])
                              ? input_context[input_columns[i]]
                              : json{};
        if (!evaluateFeel(rule.input_expressions[i], col_value)) {
          return false;
        }
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// loadFromJson
// ─────────────────────────────────────────────────────────────────────────────

bool DmnEvaluator::loadFromJson(const json& dmn_json) {
    try {
        DecisionTable dt;
        dt.id         = dmn_json.value("id",         "");
        dt.name       = dmn_json.value("name",       dt.id);
        dt.hit_policy = dmn_json.value("hit_policy", "UNIQUE");

        if (dmn_json.contains("input_columns") && dmn_json["input_columns"].is_array()) {
            for (const auto& c : dmn_json["input_columns"]) {
              dt.input_columns.push_back(c);
            }
        }
        if (dmn_json.contains("output_columns") && dmn_json["output_columns"].is_array()) {
            for (const auto& c : dmn_json["output_columns"]) {
              dt.output_columns.push_back(c);
            }
        }

        if (dmn_json.contains("rules") && dmn_json["rules"].is_array()) {
            for (const auto& r : dmn_json["rules"]) {
                DmnRule rule;
                rule.id          = r.value("id",          "");
                rule.description = r.value("description", "");

                // Support both "inputs" (array) and "input_expressions" (array)
                const auto& inputs_key =
                    r.contains("inputs") ? "inputs" : "input_expressions";
                if (r.contains(inputs_key) && r[inputs_key].is_array()) {
                    for (const auto& e : r[inputs_key]) {
                        rule.input_expressions.push_back(e.is_string() ? e.get<std::string>() : e.dump());
                    }
                }

                // Support both "outputs" (object) and "output_values" (object)
                const auto& outputs_key =
                    r.contains("outputs") ? "outputs" : "output_values";
                if (r.contains(outputs_key) && r[outputs_key].is_object()) {
                    rule.output_values = r[outputs_key];
                }
                dt.rules.push_back(std::move(rule));
            }
        }

        if (dt.id.empty()) {
            SPDLOG_WARN("[DmnEvaluator] Decision table has no id");
            return false;
        }
        
        // Thread-safety: protect shared tables_ access
        {
            std::lock_guard<std::mutex> lock(tables_mutex_);
            tables_[dt.id] = std::move(dt);
        }
        return true;
    } catch (const std::exception& ex) {
        SPDLOG_ERROR("[DmnEvaluator] loadFromJson failed: {}", ex.what());
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// loadFromXml  — simplified state-machine XML parser for DMN 1.5
// ─────────────────────────────────────────────────────────────────────────────

bool DmnEvaluator::loadFromXml(std::string_view dmn_xml) {
    // Minimal DMN 1.5 XML → JSON conversion then delegate to loadFromJson.
    // Supported element names (case-insensitive): decision, decisionTable,
    // input, output, rule, inputEntry, outputEntry.
    //
    // Strategy: extract text content of relevant attributes and build
    // a JSON representation for loadFromJson.

    if (dmn_xml.empty()) {
      return false;
    }

    // Security guard: 10 MiB
    // Use explicit unsigned multiplication to avoid overflow warnings
    constexpr size_t MAX_DMN_SIZE = 10 * 1024 * 1024;  // 10 MiB
    if (static_cast<int>(dmn_xml.size()) > MAX_DMN_SIZE) {
        SPDLOG_ERROR("[DmnEvaluator] DMN XML exceeds 10 MiB size limit");
        return false;
    }

    // Helper: strip namespace prefix
    // Thread-safe: pure function, no captures, no shared state access
    auto stripNs = [](std::string_view tag) -> std::string_view {
       const auto col = tag.find(':');
       return col == std::string_view::npos ? tag : tag.substr(col + 1);
    };

    // Thread-safe: pure function, no captures, no shared state access
    auto toLower = [](std::string s) {
       for (char& c : s) {
         c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
       }
       return s;
    };

    // State-machine tokenizer (reuses BpmnSerializer approach)
    DecisionTable dt;
    dt.hit_policy = "UNIQUE";

    enum class State { TEXT, TAG, ATTR_NAME, ATTR_VAL_Q1, ATTR_VAL_Q2 };
    State st = State::TEXT;
    std::string cur_tag;
    std::string attr_name;
    std::string attr_val;
    std::map<std::string, std::string> attrs;
    bool in_decision_table = false;

    // Active rule being built
    DmnRule current_rule;
    int     input_entry_idx = 0;
    bool    in_rule = false;
    bool    in_input_entry = false;
    bool    in_output_entry = false;
    std::string output_col_name = {};
    std::string current_text = {};

    auto flushTag = [&](std::string_view raw_tag, bool closing) {
        const std::string tag_lower = toLower(std::string(stripNs(raw_tag)));

        if (!closing) {
            if (tag_lower == "decision") {
                dt.id   = attrs.count("id")   ? attrs["id"]   : "";
                dt.name = attrs.count("name") ? attrs["name"] : dt.id;
            } else if (tag_lower == "decisiontable") {
                in_decision_table = true;
                if (attrs.count("hitpolicy")) {
                    std::string hp = attrs["hitpolicy"];
                    for (char& c : hp) {
                      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    }
                    dt.hit_policy = hp;
                }
            } else if (tag_lower == "input" && in_decision_table) {
                if (attrs.count("label")) {
                  dt.input_columns.push_back(attrs["label"]);
                }
                else if (attrs.count("id")) dt.input_columns.push_back(attrs["id"]);
            } else if (tag_lower == "output" && in_decision_table) {
                output_col_name = attrs.count("label") ? attrs["label"]
                                : attrs.count("name")  ? attrs["name"]
                                : attrs.count("id")    ? attrs["id"] : "";
                if (!output_col_name.empty()) {
                  dt.output_columns.push_back(output_col_name);
                }
            } else if (tag_lower == "rule") {
                in_rule = true;
                current_rule = DmnRule{};
                if (attrs.count("id")) {
                  current_rule.id = attrs["id"];
                }
                input_entry_idx = 0;
            } else if (tag_lower == "inputentry") {
                in_input_entry  = true;
                in_output_entry = false;
                current_text.clear();
            } else if (tag_lower == "outputentry") {
                in_output_entry = true;
                in_input_entry  = false;
                current_text.clear();
            }
        } else {
            if (tag_lower == "inputentry" && in_rule) {
                // Strip outer quotes from FEEL text  e.g. "\"high\"" → "high"
                std::string expr = current_text;
                while ((!expr.empty() && (expr.front() == ' ' || expr.front() == '\t'))) {
                  expr.erase(expr.begin());
                }
                while ((!expr.empty() && (expr.back()  == ' ' || expr.back()  == '\t'))) {
                  expr.pop_back();
                }
                current_rule.input_expressions.push_back(expr);
                ++input_entry_idx;
                in_input_entry = false;
            } else if (tag_lower == "outputentry" && in_rule) {
                // Determine which output column this is
                const int out_idx = static_cast<int>(current_rule.output_values.size());
                if (out_idx < static_cast<int>(dt.output_columns.size())) {
                    current_rule.output_values[dt.output_columns[out_idx]] = current_text;
                }
                in_output_entry = false;
            } else if (tag_lower == "rule" && in_rule) {
                in_rule = false;
                dt.rules.push_back(std::move(current_rule));
            } else if (tag_lower == "decisiontable") {
                in_decision_table = false;
            }
        }

        attrs.clear();
        cur_tag.clear();
    };

    size_t i = 0;
    const size_t n = dmn_xml.size();
    while (i < n) {
        const char c = dmn_xml[i];
        switch (st) {
        case State::TEXT:
            if (c == '<') {
                if (in_input_entry || in_output_entry) {
                    // Save accumulated text
                    while (!current_text.empty() && std::isspace(static_cast<unsigned char>(current_text.back())))
                        current_text.pop_back();
                }
                cur_tag.clear();
                attrs.clear();
                st = State::TAG;
            } else if (in_input_entry || in_output_entry) {
                current_text += c;
            }
            break;
        case State::TAG: {
            if (c == '>') {
                bool closing = (!cur_tag.empty() && cur_tag.front() == '/');
                if (closing) {
                  cur_tag.erase(cur_tag.begin());
                }
                // Self-closing: ends with /
                bool self_close = (!cur_tag.empty() && cur_tag.back() == '/');
                if (self_close) {
                  cur_tag.pop_back();
                }
                flushTag(cur_tag, closing);
                if (self_close) {
                  flushTag(cur_tag, true);
                }
                st = State::TEXT;
            } else if (std::isspace(static_cast<unsigned char>(c)) && !cur_tag.empty()) {
                // Start attribute parsing
                attr_name.clear();
                st = State::ATTR_NAME;
            } else {
                cur_tag += c;
            }
            break;
        }
        case State::ATTR_NAME:
            if (c == '=') {
                st = State::ATTR_VAL_Q1;
                attr_val.clear();
            } else if (c == '>') {
                bool closing = (!cur_tag.empty() && cur_tag.front() == '/');
                if (closing) {
                  cur_tag.erase(cur_tag.begin());
                }
                flushTag(cur_tag, closing);
                st = State::TEXT;
            } else if (!std::isspace(static_cast<unsigned char>(c))) {
                attr_name += c;
            }
            break;
        case State::ATTR_VAL_Q1:
            if (c == '"') {
                attr_val.clear();
                st = State::ATTR_VAL_Q2;
            }
            break;
        case State::ATTR_VAL_Q2:
            if (c == '"') {
                for (char& ac : attr_name) {
                  ac = static_cast<char>(std::tolower(static_cast<unsigned char>(ac)));
                }
                attrs[attr_name] = attr_val;
                attr_name.clear();
                st = State::ATTR_NAME;
            } else {
                attr_val += c;
            }
            break;
        }
        ++i;
    }

    if (dt.id.empty()) {
        SPDLOG_WARN("[DmnEvaluator] Could not parse decision id from DMN XML");
        return false;
    }
     
    // Thread-safety: protect shared tables_ access
    {
        std::lock_guard<std::mutex> lock(tables_mutex_);
        tables_[dt.id] = std::move(dt);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// evaluate
// ─────────────────────────────────────────────────────────────────────────────

json DmnEvaluator::evaluate(std::string_view decision_id,
                              const json&      input_context) const
{
    // Thread-safety: protect shared tables_ access
    std::lock_guard<std::mutex> lock(tables_mutex_);
    
    const auto it = tables_.find(std::string(decision_id));
    if (it == tables_.end()) {
        SPDLOG_WARN("[DmnEvaluator] Decision '{}' not found", decision_id);
        return {};
    }
    const DecisionTable& dt = it->second;

    if (dt.hit_policy == "COLLECT") {
        json results = json::array();
        for (const auto& rule : dt.rules) {
            if (matchRule_(rule, dt.input_columns, input_context)) {
                results.push_back(rule.output_values);
            }
        }
        return results;
    }

    // UNIQUE and FIRST: return first matching rule's outputs
    for (const auto& rule : dt.rules) {
        if (matchRule_(rule, dt.input_columns, input_context)) {
            return rule.output_values;
        }
    }
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// listDecisions / getDecision
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> DmnEvaluator::listDecisions() const {
    // Thread-safety: protect shared tables_ access
    std::lock_guard<std::mutex> lock(tables_mutex_);
    
    std::vector<std::string> ids = {};

    ids.reserve(tables_.size());
    for (const auto& [id, _] : tables_) {
      ids.push_back(id);
    }
    return ids;
}

std::optional<DecisionTable> DmnEvaluator::getDecision(std::string_view decision_id) const {
    // Thread-safety: protect shared tables_ access
    std::lock_guard<std::mutex> lock(tables_mutex_);
    
    const auto it = tables_.find(std::string(decision_id));
    if (it == tables_.end()) {
      return std::nullopt;
    }
    return it->second;
}

} // namespace process
} // namespace themis


