/**
 * @file mutation_execution_plan.h
 * @brief Execution plan types for AQL mutation operations — EPIC-004 Phase 3.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 3 implementation
 */

#pragma once

#include "query/aql_parser.h"   // ASTNodeType
#include <nlohmann/json.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace query {

// ---------------------------------------------------------------------------
// MutationStepType
// ---------------------------------------------------------------------------

/**
 * @brief Step types in a mutation execution plan.
 *
 * Each value represents a discrete operation that the MutationExecutor
 * performs in sequence when executing a MutationExecutionPlan.
 */
enum class MutationStepType {
    GenerateKeys,       ///< Generate unique _key values for new documents.
    Serialize,          ///< Serialize document(s) to storage format (JSON bytes).
    RocksDbPut,         ///< Write a key-value pair to RocksDB.
    RocksDbDelete,      ///< Delete a key-value pair from RocksDB.
    UpdateIndexes,      ///< Update all secondary indexes for the target collection.
    ValidatePredicate,  ///< Evaluate a WHERE/FILTER predicate before executing.
    AcquireLock,        ///< Acquire the collection write lock.
    ReleaseLock,        ///< Release the collection write lock.
    WriteWAL,           ///< Write the mutation record to the write-ahead log.
};

// ---------------------------------------------------------------------------
// MutationStep
// ---------------------------------------------------------------------------

/**
 * @brief A single step in a mutation execution plan.
 *
 * Steps are executed in order by MutationExecutor::execute().
 * The @c params field carries step-specific configuration as a JSON object
 * (e.g. collection name, key prefix, predicate description).
 */
struct MutationStep {
    MutationStepType type;        ///< Discriminator for executor dispatch.
    std::string      description; ///< Human-readable description for diagnostics.
    nlohmann::json   params;      ///< Step-specific parameters (JSON object).

    /**
     * @brief Serialise the step to a JSON object.
     * @return JSON representation with "type", "description", and "params" keys.
     */
    [[nodiscard]] nlohmann::json toJSON() const;
};

// ---------------------------------------------------------------------------
// MutationResult
// ---------------------------------------------------------------------------

/**
 * @brief Result structure returned after mutation execution.
 *
 * Produced by MutationExecutor::execute() and returned to the caller.
 * On success, @c success is @c true and @c affected_count reflects the number
 * of documents modified.  On failure, @c success is @c false and @c errors
 * plus @c error_code carry the diagnostic information.
 */
struct MutationResult {
    bool                     success        = false; ///< True when execution succeeded.
    int64_t                  affected_count = 0;     ///< Documents affected (inserted/updated/removed).
    std::vector<std::string> inserted_ids;           ///< _key values of newly inserted documents.
    std::vector<std::string> errors;                 ///< Error messages on failure.
    std::string              error_code;             ///< Machine-readable error code (empty on success).

    /**
     * @brief Construct a successful result.
     * @param affected  Number of affected documents.
     * @param ids       Optional list of inserted document keys.
     * @return Populated success result.
     */
    [[nodiscard]] static MutationResult Ok(int64_t                  affected,
                                           std::vector<std::string> ids = {});

    /**
     * @brief Construct a failure result.
     * @param code  Machine-readable error code (e.g. "LOCK_TIMEOUT").
     * @param msg   Human-readable error description.
     * @return Populated failure result.
     */
    [[nodiscard]] static MutationResult Failure(std::string code, std::string msg);

    /**
     * @brief Serialise the result to a JSON object.
     * @return JSON representation.
     */
    [[nodiscard]] nlohmann::json toJSON() const;
};

// ---------------------------------------------------------------------------
// MutationExecutionPlan
// ---------------------------------------------------------------------------

/**
 * @brief Execution plan for a DML mutation operation (EPIC-004 Phase 3).
 *
 * Produced by AqlMutationTranslator::translate() and consumed by
 * MutationExecutor::execute().  Describes the ordered sequence of steps
 * required to execute a mutation atomically against the storage layer.
 *
 * ### Lifecycle
 * ```
 * AQLParser::parseMutation()          → MutationNode
 * AqlMutationValidator::validate()    → MutationValidationResult (errors block progress)
 * AqlMutationTranslator::translate()  → MutationExecutionPlan
 * MutationExecutor::execute()         → MutationResult
 * ```
 */
struct MutationExecutionPlan {
    ASTNodeType                  mutation_type;         ///< INSERT/UPDATE/REMOVE/REPLACE/UPSERT.
    std::string                  collection;             ///< Target collection name.
    std::vector<MutationStep>    steps;                  ///< Ordered execution steps.
    int64_t                      estimated_latency_ms = 0; ///< Estimated latency (0 = unknown).
    bool                         requires_transaction  = false; ///< Whether a transaction context is needed.
    std::optional<int64_t>       affected_limit;         ///< Max rows from LIMIT clause (nullopt = unlimited).

    /**
     * @brief Serialise the plan to a JSON object for debugging/logging.
     * @return JSON representation of the full plan including all steps.
     */
    [[nodiscard]] nlohmann::json toJSON() const;
};

// ---------------------------------------------------------------------------
// Inline / inline-eligible implementations
// ---------------------------------------------------------------------------

inline nlohmann::json MutationStep::toJSON() const {
    // Map enum to string for readability
    static const char* kTypeNames[] = {
        "GenerateKeys", "Serialize", "RocksDbPut", "RocksDbDelete",
        "UpdateIndexes", "ValidatePredicate", "AcquireLock", "ReleaseLock", "WriteWAL"
    };
    const auto idx = static_cast<std::size_t>(type);
    const char* typeName = (idx < 9) ? kTypeNames[idx] : "Unknown";
    return {{"type", typeName}, {"description", description}, {"params", params}};
}

inline MutationResult MutationResult::Ok(int64_t affected, std::vector<std::string> ids) {
    MutationResult r;
    r.success        = true;
    r.affected_count = affected;
    r.inserted_ids   = std::move(ids);
    return r;
}

inline MutationResult MutationResult::Failure(std::string code, std::string msg) {
    MutationResult r;
    r.success    = false;
    r.error_code = std::move(code);
    r.errors.push_back(std::move(msg));
    return r;
}

inline nlohmann::json MutationResult::toJSON() const {
    nlohmann::json j{
        {"success",        success},
        {"affected_count", affected_count},
        {"error_code",     error_code}
    };
    j["inserted_ids"] = inserted_ids;
    j["errors"]       = errors;
    return j;
}

inline nlohmann::json MutationExecutionPlan::toJSON() const {
    // mutation_type → string
    static const char* kNodeTypeNames[] = {
        "Query", "ForNode", "FilterNode", "SortNode", "LimitNode", "ReturnNode",
        "LetNode", "CollectNode", "WithNode",
        "Insert", "Update", "Remove", "Replace", "Upsert"
    };
    const auto idx = static_cast<std::size_t>(mutation_type);
    const char* typeName = (idx < 14) ? kNodeTypeNames[idx] : "Unknown";

    nlohmann::json j{
        {"mutation_type",         typeName},
        {"collection",            collection},
        {"estimated_latency_ms",  estimated_latency_ms},
        {"requires_transaction",  requires_transaction},
    };

    if (affected_limit.has_value()) {
        j["affected_limit"] = *affected_limit;
    } else {
        j["affected_limit"] = nullptr;
    }

    nlohmann::json stepsArr = nlohmann::json::array();
    for (const auto& s : steps) {
      stepsArr.push_back(s.toJSON());
    }
    j["steps"] = stepsArr;

    return j;
}

} // namespace query
} // namespace themis
