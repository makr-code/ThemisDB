/**
 * @file dspy_module.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=14, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/dspy_module.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// DspyField
// ============================================================================

DspyField::DspyField(std::string nm,
                     std::string desc,
                     DspyFieldType t,
                     bool req,
                     std::string def_val)
    : name(std::move(nm))
    , description(std::move(desc))
    , type(t)
    , required(req)
    , default_value(std::move(def_val))
{}

// ============================================================================
// DspySignature
// ============================================================================

DspySignature::DspySignature(std::string name, std::string description)
    : name_(std::move(name))
    , description_(std::move(description))
{}

DspySignature& DspySignature::addInput(DspyField field)
{
    inputs_.push_back(std::move(field));
    return *this;
}

DspySignature& DspySignature::addOutput(DspyField field)
{
    outputs_.push_back(std::move(field));
    return *this;
}

const std::string& DspySignature::getName() const        { return name_;        }
const std::string& DspySignature::getDescription() const { return description_; }
const std::vector<DspyField>& DspySignature::inputs()  const { return inputs_;  }
const std::vector<DspyField>& DspySignature::outputs() const { return outputs_; }

std::string DspySignature::buildPrompt(
    const std::unordered_map<std::string, std::string>& context) const
{
    std::ostringstream out = {};

    // Task description header
    if (!description_.empty()) {
        out << description_ << "\n\n";
    }

    // Render input fields
    for (const auto& field : inputs_) {
        auto it = context.find(field.name);
        if (it == context.end()) {
            if (field.required) {
                throw DspyMissingFieldError(field.name);
            }
            // Use default value for optional missing fields
            out << field.name << ": " << field.default_value << "\n";
        } else {
            out << field.name << ": " << it->second << "\n";
        }
    }

    out << "\n";

    // Render output field labels (model must fill these in)
    for (const auto& field : outputs_) {
        out << field.name << ":";
        if (!field.description.empty()) {
            out << " # " << field.description;
        }
        out << "\n";
    }

    return out.str();
}

std::unordered_map<std::string, std::string> DspySignature::parseResponse(
    const std::string& response) const
{
    std::unordered_map<std::string, std::string> parsed;

    for (const auto& field : outputs_) {
        // Look for "FieldName:" or "FieldName :" (case-sensitive)
        std::string marker = field.name + ":";
        auto pos = response.find(marker);
        if (pos == std::string::npos) {
            // Try with a space before the colon
            marker = field.name + " :";
            pos = response.find(marker);
        }

        if (pos == std::string::npos) {
            // Field not found in response
            if (field.required) {
                THEMIS_WARN("DspySignature::parseResponse: required output '{}' not found in response",
                            field.name);
            }
            parsed[field.name] = field.default_value;
            continue;
        }

        // Extract text after the marker up to the next field label or end
        size_t value_start = pos + marker.size();

        // Skip leading whitespace / newline
        while (value_start < response.size() &&
               (response[value_start] == ' ' || response[value_start] == '\t')) {
            ++value_start;
        }

        // Find where the next output field starts (if any).
        // Support both "OtherField:" and "OtherField :" as boundary markers.
        size_t value_end = response.size();
        for (const auto& other : outputs_) {
            if (other.name == field.name) {
              continue;
            }
            // Check "OtherField:" and "OtherField :" variants
            for (const auto& suffix : {std::string(":"), std::string(" :")}) {
                std::string other_marker = other.name + suffix;
                auto other_pos = response.find(other_marker, value_start);
                if (other_pos != std::string::npos && other_pos < value_end) {
                    value_end = other_pos;
                }
            }
        }

        // Trim trailing whitespace
        std::string value = response.substr(value_start, value_end - value_start);
        while (!value.empty() && (value.back() == '\n' || value.back() == '\r' ||
                                   value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }

        parsed[field.name] = value;
    }

    return parsed;
}

// ============================================================================
// EchoDspyLLMProvider
// ============================================================================

std::string EchoDspyLLMProvider::complete(const std::string& prompt)
{
    std::ostringstream response = {};

    // Scan the prompt for output field labels (lines ending with ":")
    std::istringstream iss(prompt);
    std::string line = {};
    while (std::getline(iss, line)) {
        // Strip leading spaces
        size_t start = 0;
        while (start < line.size() && line[start] == ' ') {
          ++start;
        }
        line = line.substr(start);

        // Detect "FieldName:" or "FieldName: # description"
        if (line.empty() || line.back() == '\n') {
          continue;
        }

        // Look for a colon that is not inside a comment marker
        auto colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
          continue;
        }

        // The token before the colon must look like an identifier
        std::string token = line.substr(0, colon_pos);
        bool looks_like_field = !token.empty();
        for (char c : token) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
                looks_like_field = false;
                break;
            }
        }
        if (!looks_like_field) {
          continue;
        }

        // Check if it's in a "# description" context (it's an output label)
        bool is_output = (line.find("# ") != std::string::npos ||
                          line.size() <= colon_pos + 2);

        if (is_output) {
            response << token << ": [echo]\n";
        }
    }

    std::string result = response.str();
    if (result.empty()) {
        result = "[echo: no output fields detected]\n";
    }
    return result;
}

// ============================================================================
// DspyModule
// ============================================================================

DspyModule::DspyModule(DspySignature signature)
    : signature_(std::move(signature))
{}

DspyModule& DspyModule::setLLMProvider(std::shared_ptr<IDspyLLMProvider> provider)
{
    llm_provider_ = std::move(provider);
    return *this;
}

const DspySignature& DspyModule::getSignature() const
{
    return signature_;
}

std::unordered_map<std::string, std::string> DspyModule::forward(
    const std::unordered_map<std::string, std::string>& context)
{
    if (!llm_provider_) {
        throw std::runtime_error(
            "DspyModule::forward: no LLM provider set; call setLLMProvider() first.");
    }

    std::string prompt   = signature_.buildPrompt(context);
    std::string response = llm_provider_->complete(prompt);

    THEMIS_DEBUG("DspyModule::forward [{}]: prompt_len={}, response_len={}",
                 signature_.getName(), prompt.size(), response.size());

    return signature_.parseResponse(response);
}

// ============================================================================
// DspyChainOfThought
// ============================================================================

DspyChainOfThought::DspyChainOfThought(DspySignature signature)
    : DspyModule([&]() -> DspySignature {
        // Clone signature and inject a "Reasoning" output at the front
        DspySignature augmented = signature;
        DspyField reasoning_field(
            "Reasoning",
            "Step-by-step reasoning before producing the final answer.",
            DspyFieldType::STRING,
            false,
            "");

        // Rebuild outputs with Reasoning prepended
        std::vector<DspyField> new_outputs;
        new_outputs.push_back(std::move(reasoning_field));
        for (const auto& f : signature.outputs()) {
            new_outputs.push_back(f);
        }

        DspySignature result(signature.getName(), signature.getDescription());
        for (const auto& f : signature.inputs()) {
            result.addInput(f);
        }
        for (auto& f : new_outputs) {
            result.addOutput(f);
        }
        return result;
    }())
{}

std::unordered_map<std::string, std::string> DspyChainOfThought::forward(
    const std::unordered_map<std::string, std::string>& context)
{
    // Use DspyModule::forward which now includes the Reasoning field
    return DspyModule::forward(context);
}

// ============================================================================
// DspyMissingFieldError
// ============================================================================

DspyMissingFieldError::DspyMissingFieldError(const std::string& field_name)
    : std::invalid_argument("Required DSPy field missing: " + field_name)
    , field_name_(field_name)
{}

const std::string& DspyMissingFieldError::fieldName() const
{
    return field_name_;
}

} // namespace prompt_engineering
} // namespace themis
