/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            dspy_module.h                                      ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-14 06:54:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     322                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 696d2d349b  2026-03-24  fix: address 7 Copilot review comments (docs, beam_width ... ║
    • b87706b26d  2026-03-24  feat(prompt_engineering): implement ToT reasoner, ProTeGi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file dspy_module.h
 * @brief DSPy-compatible prompt declaration layer.
 *
 * Provides a typed signature system inspired by the DSPy framework
 * (Khattab et al., 2023).  A @c DspySignature declares the semantic
 * contract of an LLM call: which input fields the model receives and
 * which output fields it must produce.  Modules (@c DspyPredict,
 * @c DspyChainOfThought) consume signatures to build prompts
 * automatically and parse structured responses.
 *
 * Design goals:
 *  - **Declarative** – field names, descriptions, and types are
 *    declared once; prompt construction and response parsing are
 *    derived automatically.
 *  - **Composable** – modules can be nested; the output of one module
 *    becomes the input context of the next.
 *  - **Provider-agnostic** – LLM execution is injected via
 *    @c IDspyLLMProvider, enabling unit tests without a live model.
 *
 * Reference:
 *   O. Khattab et al., "DSPy: Compiling Declarative Language Model
 *   Calls into Self-Improving Pipelines," arXiv:2310.03714, 2023.
 *   Available: https://arxiv.org/abs/2310.03714
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>
#include <stdexcept>

namespace themis {
namespace prompt_engineering {

// ---------------------------------------------------------------------------
// Field types
// ---------------------------------------------------------------------------

/**
 * @brief Semantic type of a DSPy signature field.
 */
enum class DspyFieldType {
    STRING,    ///< Arbitrary text.
    INT,       ///< Whole number.
    FLOAT,     ///< Floating-point number.
    BOOL,      ///< True / False.
    LIST,      ///< Comma-separated list of items.
    JSON       ///< Structured JSON object.
};

// ---------------------------------------------------------------------------
// Field declaration
// ---------------------------------------------------------------------------

/**
 * @brief A single declared field in a DSPy signature.
 */
struct DspyField {
    std::string  name;                        ///< Identifier used in prompts and context maps.
    std::string  description;                 ///< Human-readable description shown to the model.
    DspyFieldType type  = DspyFieldType::STRING; ///< Expected value type.
    bool         required = true;             ///< Whether the field must be present.
    std::string  default_value;               ///< Default used when field is absent and not required.

    /**
     * @brief Convenience constructor.
     */
    DspyField(std::string nm,
              std::string desc,
              DspyFieldType t       = DspyFieldType::STRING,
              bool req              = true,
              std::string def_val   = "");
};

// ---------------------------------------------------------------------------
// Signature
// ---------------------------------------------------------------------------

/**
 * @brief Declares the typed input→output contract of an LLM call.
 *
 * A signature acts as a structured specification: callers describe which
 * fields are inputs and which are outputs; the associated module builds
 * the prompt from the inputs and extracts the outputs from the response.
 *
 * Example:
 * @code
 * DspySignature sig("SentimentAnalysis",
 *                   "Classify the sentiment of the given text.");
 * sig.addInput ({"text",      "The text to classify."});
 * sig.addOutput({"sentiment", "One of: positive, negative, neutral.",
 *                DspyFieldType::STRING});
 * sig.addOutput({"confidence","Confidence score between 0 and 1.",
 *                DspyFieldType::FLOAT});
 * @endcode
 */
class DspySignature {
public:
    /**
     * @brief Construct a named signature with a task description.
     * @param name        Short identifier, e.g. "SentimentAnalysis".
     * @param description One-sentence description of the task.
     */
    DspySignature(std::string name, std::string description = "");

    /** @brief Add an input field. */
    DspySignature& addInput(DspyField field);

    /** @brief Add an output field. */
    DspySignature& addOutput(DspyField field);

    /** @brief Return signature name. */
    const std::string& getName() const;

    /** @brief Return task description. */
    const std::string& getDescription() const;

    /** @brief Return all declared input fields. */
    const std::vector<DspyField>& inputs() const;

    /** @brief Return all declared output fields. */
    const std::vector<DspyField>& outputs() const;

    /**
     * @brief Build the prompt string from a context map.
     *
     * Renders each input field as a labelled section and appends output
     * field labels so the model knows what to produce.
     *
     * @param context  Map from field name → value string for each input.
     * @return Formatted prompt.
     * @throws std::invalid_argument when a required input field is missing.
     */
    std::string buildPrompt(
        const std::unordered_map<std::string, std::string>& context) const;

    /**
     * @brief Parse the model response into a field→value map.
     *
     * Looks for each output field label ("FieldName:") in the response
     * and extracts the text following it.  Absent optional fields receive
     * their @c default_value.
     *
     * @param response  Raw model response text.
     * @return Map from output field name → extracted value string.
     */
    std::unordered_map<std::string, std::string> parseResponse(
        const std::string& response) const;

private:
    std::string          name_;
    std::string          description_;
    std::vector<DspyField> inputs_;
    std::vector<DspyField> outputs_;
};

// ---------------------------------------------------------------------------
// LLM provider interface
// ---------------------------------------------------------------------------

/**
 * @brief Interface used by DSPy modules to execute LLM calls.
 */
class IDspyLLMProvider {
public:
    virtual ~IDspyLLMProvider() = default;

    /**
     * @brief Execute the prompt and return the raw model response.
     * @param prompt  Formatted prompt produced by @c DspySignature::buildPrompt.
     * @return Model response text.
     */
    virtual std::string complete(const std::string& prompt) = 0;
};

// ---------------------------------------------------------------------------
// Built-in echo provider (for unit tests)
// ---------------------------------------------------------------------------

/**
 * @brief Echo LLM provider that returns a synthetic response without an LLM.
 *
 * For each output field declared in the prompt, the provider generates a
 * placeholder value of the form "<field_name>: [echo]".  Suitable for
 * testing signature building and response parsing.
 */
class EchoDspyLLMProvider : public IDspyLLMProvider {
public:
    std::string complete(const std::string& prompt) override;
};

// ---------------------------------------------------------------------------
// Module base class
// ---------------------------------------------------------------------------

/**
 * @brief Base class for all DSPy-compatible modules.
 *
 * A module wraps a @c DspySignature and a @c IDspyLLMProvider and exposes a
 * single @c forward() method that drives the full inference cycle:
 *   1. Build the prompt.
 *   2. Call the LLM provider.
 *   3. Parse the response.
 */
class DspyModule {
public:
    explicit DspyModule(DspySignature signature);
    virtual ~DspyModule() = default;

    /**
     * @brief Inject the LLM provider.  Must be called before @c forward().
     */
    DspyModule& setLLMProvider(std::shared_ptr<IDspyLLMProvider> provider);

    /**
     * @brief Return the declared signature.
     */
    const DspySignature& getSignature() const;

    /**
     * @brief Execute the module: build prompt → call LLM → parse outputs.
     *
     * @param context  Input field values.
     * @return Map from output field name → extracted value.
     * @throws std::runtime_error when the LLM provider has not been injected.
     * @throws std::invalid_argument when a required input field is missing.
     */
    virtual std::unordered_map<std::string, std::string> forward(
        const std::unordered_map<std::string, std::string>& context);

protected:
    DspySignature                       signature_;
    std::shared_ptr<IDspyLLMProvider>   llm_provider_;
};

// ---------------------------------------------------------------------------
// Predict module (basic zero-shot prediction)
// ---------------------------------------------------------------------------

/**
 * @brief Basic DSPy predictor.
 *
 * Directly forwards the signature inputs to the LLM and returns parsed
 * outputs with no additional scaffolding.
 */
class DspyPredict : public DspyModule {
public:
    using DspyModule::DspyModule;
};

// ---------------------------------------------------------------------------
// ChainOfThought module (adds CoT reasoning step)
// ---------------------------------------------------------------------------

/**
 * @brief DSPy module that prepends chain-of-thought reasoning instructions.
 *
 * Automatically injects a "Reasoning" output field before the declared
 * output fields so the model reasons step-by-step before producing its
 * final outputs.
 *
 * The intermediate reasoning is captured in the "Reasoning" key of the
 * returned map alongside the declared output fields.
 */
class DspyChainOfThought : public DspyModule {
public:
    /**
     * @brief Construct by cloning the given signature and injecting the
     *        "Reasoning" output field at position 0.
     */
    explicit DspyChainOfThought(DspySignature signature);

    std::unordered_map<std::string, std::string> forward(
        const std::unordered_map<std::string, std::string>& context) override;
};

// ---------------------------------------------------------------------------
// Exception
// ---------------------------------------------------------------------------

/**
 * @brief Thrown when a required field is missing from the context.
 */
class DspyMissingFieldError : public std::invalid_argument {
public:
    explicit DspyMissingFieldError(const std::string& field_name);
    const std::string& fieldName() const;

private:
    std::string field_name_;
};

} // namespace prompt_engineering
} // namespace themis
