/**
 * @file llm_reproducibility.h
 * @brief LLM Reproducibility and Governance Standard for ThemisDB.
 *
 * @note **Configuration/Standards Header**: Defines reproducibility modes and governance.
 *       No .cpp implementation needed. Used by consumers for reproducibility control.
 *
 * ## Motivation (P2.3)
 *
 * LLM inference is inherently probabilistic.  In a database system this
 * creates auditability and reproducibility challenges:
 *
 *  - The same query with the same retrieved documents may produce different
 *    answers on different runs, hardware, or after model updates.
 *  - Regulated domains (legal, finance, medical) require that each answer
 *    can be reproduced or at least traced back to its exact generation
 *    parameters.
 *  - Debugging incidents is impossible without knowing the sampling
 *    parameters used during the failing request.
 *
 * This header defines:
 *  1. `LLMReproducibilityMode` — named modes that map to concrete sampler
 *     parameters, making per-request parameter bundles explicit.
 *  2. `LLMInferenceParameterSnapshot` — an immutable record of every
 *     sampler parameter used for a given request, suitable for storage in
 *     `AIDecisionAudit` and for exact replay.
 *  3. `LLMReproducibilityPolicy` — a per-deployment configuration that
 *     constrains which modes are permitted and enforces minimum audit coverage.
 *
 * ## Usage contract
 *
 * 1. Before calling the inference engine, callers MUST build an
 *    `LLMInferenceParameterSnapshot` from the `InferenceRequest` and store
 *    it alongside the `AIDecisionAudit` for that request.
 * 2. The snapshot MUST be round-trippable to/from JSON so it can be
 *    persisted and replayed.
 * 3. `LLMReproducibilityMode::Deterministic` requires `temperature == 0.0`
 *    and a fixed `rng_seed`.  The inference engine MUST honour this or
 *    return an error.
 *
 * @see include/llm/ai_decision_auditor.h   — Audit record storage
 * @see include/llm/llm_correlation_context.h — Trace correlation
 * @see include/llm/llm_plugin_interface.h  — InferenceRequest
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

// ============================================================================
// LLMReproducibilityMode
// ============================================================================

/**
 * @brief Named reproducibility modes for LLM inference.
 *
 * Each mode maps to a concrete set of sampler parameter defaults.  The mode
 * name is persisted in `LLMInferenceParameterSnapshot::mode_name` and in
 * `AIDecisionAudit` so that historical records can be interpreted correctly.
 *
 * | Mode          | temperature | top_p | top_k | rng_seed | Use case                      |
 * |---------------|-------------|-------|-------|----------|-------------------------------|
 * | Deterministic | 0.0         | 1.0   | 1     | required | Audit, regression testing     |
 * | Audit         | 0.1         | 0.95  | 10    | optional | Regulated domains             |
 * | Balanced      | 0.7         | 0.9   | 40    | optional | Default production            |
 * | Creative      | 1.2         | 0.95  | 0     | none     | Open-ended generation         |
 * | Custom        | (caller-set)| -     | -     | -        | Explicit parameter override   |
 */
enum class LLMReproducibilityMode : uint8_t {
    Deterministic = 0, ///< temperature=0, top_k=1, fixed seed — maximally reproducible
    Audit         = 1, ///< Low temperature, fixed seed required — regulated domains
    Balanced      = 2, ///< Default production mode
    Creative      = 3, ///< High temperature, open-ended generation
    Custom        = 4, ///< Caller supplies all parameters explicitly
};

/** @return Short canonical name string for the mode (persisted in audit records). */
[[nodiscard]] inline const char* reproducibilityModeName(
    LLMReproducibilityMode mode) noexcept
{
    switch (mode) {
        case LLMReproducibilityMode::Deterministic: return "deterministic";
        case LLMReproducibilityMode::Audit:         return "audit";
        case LLMReproducibilityMode::Balanced:      return "balanced";
        case LLMReproducibilityMode::Creative:      return "creative";
        case LLMReproducibilityMode::Custom:        return "custom";
    }
    return "unknown";
}

/** @return Parse a mode name string back to enum. Returns `Custom` if unrecognised. */
[[nodiscard]] inline LLMReproducibilityMode parseReproducibilityMode(
    const std::string& name) noexcept
{
    if (name == "deterministic") return LLMReproducibilityMode::Deterministic;
    if (name == "audit")         return LLMReproducibilityMode::Audit;
    if (name == "balanced")      return LLMReproducibilityMode::Balanced;
    if (name == "creative")      return LLMReproducibilityMode::Creative;
    return LLMReproducibilityMode::Custom;
}

// ============================================================================
// LLMInferenceParameterSnapshot
// ============================================================================

/**
 * @brief Immutable snapshot of all sampler parameters used for one inference call.
 *
 * Stored alongside `AIDecisionAudit` to enable post-hoc audit and replay.
 *
 * ## Invariants
 *
 * - `mode == Deterministic` implies `temperature == 0.0f && top_k == 1 && rng_seed.has_value()`.
 * - `mode == Audit`         implies `rng_seed.has_value()`.
 * - All float values are finite (no NaN/Inf).
 */
struct LLMInferenceParameterSnapshot {
    LLMReproducibilityMode mode = LLMReproducibilityMode::Balanced;

    float    temperature       = 0.7f;  ///< Sampling temperature (0.0 = greedy)
    float    top_p             = 0.9f;  ///< Nucleus sampling probability
    int      top_k             = 40;    ///< Top-K sampling limit (0 = disabled)
    float    repetition_penalty = 1.1f; ///< Repetition penalty factor
    int      max_tokens        = 512;   ///< Maximum generated tokens

    /// RNG seed for reproducible sampling.  Must be set for Deterministic/Audit modes.
    std::optional<uint64_t> rng_seed;

    /// Model identifier used (e.g. "qwen2.5-coder:14b" or a GGUF path hash).
    std::string model_id;

    /// The mode name string — matches `reproducibilityModeName(mode)`.
    std::string mode_name;

    // -----------------------------------------------------------------------
    // Factory helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Build a Deterministic snapshot.
     * @param seed    Mandatory RNG seed for full reproducibility.
     * @param model   Model identifier.
     */
    [[nodiscard]] static LLMInferenceParameterSnapshot makeDeterministic(
        uint64_t seed,
        const std::string& model = {})
    {
        LLMInferenceParameterSnapshot s;
        s.mode              = LLMReproducibilityMode::Deterministic;
        s.mode_name         = reproducibilityModeName(s.mode);
        s.temperature       = 0.0f;
        s.top_p             = 1.0f;
        s.top_k             = 1;
        s.repetition_penalty = 1.0f;
        s.rng_seed          = seed;
        s.model_id          = model;
        return s;
    }

    /**
     * @brief Build an Audit snapshot (low temperature, mandatory seed).
     * @param seed    Mandatory RNG seed.
     * @param model   Model identifier.
     */
    [[nodiscard]] static LLMInferenceParameterSnapshot makeAudit(
        uint64_t seed,
        const std::string& model = {})
    {
        LLMInferenceParameterSnapshot s;
        s.mode              = LLMReproducibilityMode::Audit;
        s.mode_name         = reproducibilityModeName(s.mode);
        s.temperature       = 0.1f;
        s.top_p             = 0.95f;
        s.top_k             = 10;
        s.repetition_penalty = 1.05f;
        s.rng_seed          = seed;
        s.model_id          = model;
        return s;
    }

    /**
     * @brief Build a Balanced (default production) snapshot.
     * @param model   Model identifier.
     */
    [[nodiscard]] static LLMInferenceParameterSnapshot makeBalanced(
        const std::string& model = {})
    {
        LLMInferenceParameterSnapshot s;
        s.mode      = LLMReproducibilityMode::Balanced;
        s.mode_name = reproducibilityModeName(s.mode);
        s.model_id  = model;
        return s;
    }

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * @brief Validate invariants.
     * @throws std::invalid_argument if any invariant is violated.
     */
    void validate() const {
        // Enforce finite-value invariant: all float fields must be finite (no NaN/Inf).
        if (!std::isfinite(temperature)) {
            throw std::invalid_argument("temperature must be finite (no NaN/Inf)");
        }
        if (!std::isfinite(top_p)) {
            throw std::invalid_argument("top_p must be finite (no NaN/Inf)");
        }
        if (!std::isfinite(repetition_penalty)) {
            throw std::invalid_argument("repetition_penalty must be finite (no NaN/Inf)");
        }
        if (mode == LLMReproducibilityMode::Deterministic) {
            if (temperature != 0.0f || top_k != 1 || !rng_seed.has_value()) {
                throw std::invalid_argument(
                    "Deterministic mode requires temperature=0, top_k=1, and rng_seed set");
            }
        }
        if (mode == LLMReproducibilityMode::Audit && !rng_seed.has_value()) {
            throw std::invalid_argument("Audit mode requires rng_seed to be set");
        }
        if (temperature < 0.0f || temperature > 10.0f) {
            throw std::invalid_argument("temperature must be in [0, 10]");
        }
        if (top_p <= 0.0f || top_p > 1.0f) {
            throw std::invalid_argument("top_p must be in (0, 1]");
        }
        if (max_tokens <= 0) {
            throw std::invalid_argument("max_tokens must be positive");
        }
    }

    // -----------------------------------------------------------------------
    // JSON serialisation
    // -----------------------------------------------------------------------

    /**
     * @brief Serialise the snapshot to JSON for audit storage.
     *
     * The JSON object is self-describing: all fields are present so the
     * snapshot can be replayed without any additional context.
     */
    [[nodiscard]] json toJson() const {
        json j;
        j["mode"]               = mode_name.empty()
                                      ? reproducibilityModeName(mode)
                                      : mode_name;
        j["temperature"]        = temperature;
        j["top_p"]              = top_p;
        j["top_k"]              = top_k;
        j["repetition_penalty"] = repetition_penalty;
        j["max_tokens"]         = max_tokens;
        j["model_id"]           = model_id;
        if (rng_seed.has_value()) {
            j["rng_seed"] = *rng_seed;
        } else {
            j["rng_seed"] = nullptr;
        }
        return j;
    }

    /**
     * @brief Deserialise a snapshot from a JSON object (tolerates missing keys).
     * @throws nlohmann::json::exception on type mismatches.
     */
    [[nodiscard]] static LLMInferenceParameterSnapshot fromJson(const json& j) {
        LLMInferenceParameterSnapshot s;
        const std::string mode_str = j.value("mode", "balanced");
        s.mode                     = parseReproducibilityMode(mode_str);
        s.mode_name                = mode_str;
        s.temperature              = j.value("temperature", 0.7f);
        s.top_p                    = j.value("top_p", 0.9f);
        s.top_k                    = j.value("top_k", 40);
        s.repetition_penalty       = j.value("repetition_penalty", 1.1f);
        s.max_tokens               = j.value("max_tokens", 512);
        s.model_id                 = j.value("model_id", std::string{});
        if (j.contains("rng_seed") && !j["rng_seed"].is_null()) {
            s.rng_seed = j["rng_seed"].get<uint64_t>();
        }
        return s;
    }
};

// ============================================================================
// LLMReproducibilityPolicy
// ============================================================================

/**
 * @brief Per-deployment policy governing which reproducibility modes are permitted.
 *
 * Checked before each inference call.  Violations are hard errors (fail-closed).
 *
 * ## Typical configurations
 *
 * | Deployment         | allowed_modes                           | require_audit_snapshot |
 * |--------------------|------------------------------------------|------------------------|
 * | Default production | Balanced, Creative, Custom               | false                  |
 * | Regulated domain   | Deterministic, Audit                     | true                   |
 * | CI/regression      | Deterministic                            | true                   |
 */
struct LLMReproducibilityPolicy {
    /// Modes that may be used in this deployment.
    /// Empty means all modes are permitted (default open).
    std::vector<LLMReproducibilityMode> allowed_modes;

    /// When true, every inference call MUST produce and store an audit snapshot.
    bool require_audit_snapshot = false;

    /// When true, `Creative` mode is blocked even if present in `allowed_modes`.
    bool disallow_creative_in_production = false;

    // -----------------------------------------------------------------------
    // Enforcement
    // -----------------------------------------------------------------------

    /**
     * @brief Check whether a snapshot satisfies this policy.
     * @param snap  The parameter snapshot to check.
     * @throws std::runtime_error if the snapshot violates the policy.
     */
    void enforce(const LLMInferenceParameterSnapshot& snap) const {
        if (disallow_creative_in_production &&
            snap.mode == LLMReproducibilityMode::Creative)
        {
            throw std::runtime_error(
                "Creative mode is disallowed in production by LLMReproducibilityPolicy");
        }
        if (!allowed_modes.empty()) {
            bool found = false;
            for (const auto& m : allowed_modes) {
                if (m == snap.mode) { found = true; break; }
            }
            if (!found) {
                throw std::runtime_error(
                    std::string("LLMReproducibilityPolicy: mode '") +
                    reproducibilityModeName(snap.mode) +
                    "' is not in the allowed_modes list for this deployment");
            }
        }
        // Validate snapshot invariants (throws on constraint violation).
        snap.validate();
    }

    /**
     * @brief Check the policy without throwing; return human-readable error or "".
     */
    [[nodiscard]] std::string check(const LLMInferenceParameterSnapshot& snap) const noexcept {
        try {
            enforce(snap);
            return {};
        } catch (const std::exception& e) {
            return e.what();
        }
    }
};

} // namespace llm
} // namespace themis
