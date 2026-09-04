/**
 * @file voice_macro.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace voice {

using json = nlohmann::json;

/** @brief Opaque macro identifier (UUID-like string). */
using MacroID = std::string;

// ---------------------------------------------------------------------------
// Step types
// ---------------------------------------------------------------------------

/**
 * @brief Type of a single macro step.
 */
enum class StepType {
    QUERY,    ///< Execute an AQL query (action = AQL template)
    COMMAND,  ///< Execute a built-in system command
    CONDITION,///< Conditional branching
    LOOP,     ///< Iteration over results
    WAIT,     ///< Delay execution (action = milliseconds as string)
    NOTIFY    ///< Send a notification / speak a message
};

// ---------------------------------------------------------------------------
// MacroStep
// ---------------------------------------------------------------------------

/**
 * @brief One step in a macro execution sequence.
 */
struct MacroStep {
    StepType type = StepType::QUERY;
    /** Action payload.  For QUERY this is an AQL template string.
     *  For WAIT it is the delay in milliseconds (as a string).
     *  For NOTIFY it is the message text.
     *  For COMMAND it is the command identifier.
     */
    std::string action = {};
    /** Named parameters bound into @p action (e.g. AQL bind variables). */
    std::map<std::string, std::string> parameters;
    /** Nested steps used by CONDITION and LOOP types. */
    std::vector<MacroStep> sub_steps;
};

// ---------------------------------------------------------------------------
// MacroOptions
// ---------------------------------------------------------------------------

/**
 * @brief Execution priority for a macro.
 */
enum class Priority {
    LOW,
    NORMAL,
    HIGH,
    CRITICAL
};

/**
 * @brief Options controlling macro execution behaviour.
 */
struct MacroOptions {
    /** If true the assistant asks for verbal confirmation before executing. */
    bool require_confirmation = false;
    /** Permission tokens that the calling session must hold. */
    std::vector<std::string> required_permissions;
    /** Hard timeout for the entire macro execution (ms). */
    int max_execution_time_ms = 30000;
    /** Whether to write an entry to the audit log on each execution. */
    bool log_execution = true;
    /** Execution priority. */
    Priority priority = Priority::NORMAL;
};

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

/**
 * @brief Result of one macro step.
 */
struct StepResult {
    int step_index = 0;         ///< 0-based index into MacroInfo::steps
    bool success = false;
    std::string output;         ///< Step output (e.g. query result summary)
    int64_t duration_ms = 0;
    std::string error_message;
};

/**
 * @brief Aggregate result of a complete macro execution.
 */
struct MacroResult {
    MacroID macro_id;
    bool success = false;
    std::vector<StepResult> step_results;
    int64_t execution_time_ms = 0;
    std::string output;         ///< Combined human-readable output
};

// ---------------------------------------------------------------------------
// MacroInfo  (stored metadata)
// ---------------------------------------------------------------------------

/**
 * @brief Persistent metadata for a user-defined macro.
 */
struct MacroInfo {
    MacroID macro_id;
    std::string name;           ///< Human-readable label
    std::string trigger_phrase; ///< Voice phrase that activates the macro
    std::string description;
    std::vector<std::string> tags;
    std::vector<MacroStep> steps;
    MacroOptions options;
    int64_t created_at = 0;
    int64_t last_used = 0;
    int use_count = 0;
    bool enabled = true;
};

// ---------------------------------------------------------------------------
// VoiceMacroManager
// ---------------------------------------------------------------------------

/**
 * @brief Manages user-defined voice command macros.
 *
 * Thread-safe.  Macros are stored in memory; persistence to ThemisDB
 * can be added via importMacros / exportMacros once the storage layer
 * is wired up.
 *
 * Trigger-phrase matching is case-insensitive and uses substring lookup:
 * the user utterance is normalised to lower-case, and the registered
 * phrase is treated as a sub-string that must appear in the utterance.
 */
class VoiceMacroManager {
public:
    VoiceMacroManager();
    ~VoiceMacroManager();

    // -----------------------------------------------------------------------
    // CRUD
    // -----------------------------------------------------------------------

    /**
     * @brief Register a new macro.
     * @param trigger_phrase  Normalised voice phrase (e.g. "morning report").
     * @param steps           Ordered execution steps.
     * @param options         Execution options.
     * @return New macro ID, or empty string on error.
     */
    MacroID createMacro(
        const std::string& trigger_phrase,
        const std::vector<MacroStep>& steps,
        const MacroOptions& options = {});

    /**
     * @brief Retrieve a copy of metadata for a single macro.
     *
     * Returns a value copy so callers are not exposed to the internal map
     * storage (which could be invalidated by concurrent modifications).
     *
     * @param macro_id  Macro identifier.
     * @return Copy of MacroInfo, or std::nullopt if not found.
     */
    std::optional<MacroInfo> getMacro(const MacroID& macro_id) const;

    /**
     * @brief List all macros, optionally filtered by tag.
     * @param user_id  (Reserved for future per-user isolation.)
     * @param tags     Only return macros that carry at least one of these tags.
     * @return Vector of MacroInfo copies.
     */
    std::vector<MacroInfo> listMacros(
        const std::string& user_id = "",
        const std::vector<std::string>& tags = {}) const;

    /**
     * @brief Update the human-readable metadata of a macro without touching steps or options.
     *
     * Allows callers to rename, re-describe, re-tag, or enable/disable a macro.
     *
     * @return true on success, false if the macro was not found.
     */
    bool setMacroMeta(
        const MacroID& macro_id,
        const std::string& name,
        const std::string& description,
        const std::vector<std::string>& tags,
        bool enabled);

    /**
     * @brief Replace the steps and options of an existing macro.
     * @return true on success, false if the macro was not found.
     */
    bool updateMacro(
        const MacroID& macro_id,
        const std::vector<MacroStep>& steps,
        const MacroOptions& options);

    /**
     * @brief Remove a macro.
     * @return true on success, false if not found.
     */
    bool deleteMacro(const MacroID& macro_id);

    // -----------------------------------------------------------------------
    // Execution
    // -----------------------------------------------------------------------

    /**
     * @brief Execute a macro by its ID.
     * @param macro_id    Macro to run.
     * @param parameters  Runtime parameter overrides (merged with step params).
     * @return MacroResult; result.success == false when the macro is unknown.
     */
    MacroResult executeMacro(
        const MacroID& macro_id,
        const std::map<std::string, std::string>& parameters = {});

    /**
     * @brief Try to match @p utterance against registered trigger phrases.
     *
     * Returns a copy of the matched macro's ID so callers are not exposed to
     * internal storage.  The empty string signals no match.
     *
     * @return MacroID of the first matching enabled macro, or empty string.
     */
    MacroID matchTrigger(const std::string& utterance) const;

    // -----------------------------------------------------------------------
    // Import / Export (JSON)
    // -----------------------------------------------------------------------

    /**
     * @brief Serialise one or more macros to a JSON string.
     * @param macro_ids  Macros to include (empty = all).
     * @return JSON array serialisation.
     */
    std::string exportMacros(const std::vector<MacroID>& macro_ids = {}) const;

    /**
     * @brief Import macros from a JSON string produced by exportMacros().
     * @return IDs of successfully imported macros.
     */
    std::vector<MacroID> importMacros(const std::string& json_str);

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    /** @brief Return statistics object (macro count, total executions). */
    json getStatistics() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace voice
} // namespace themis
