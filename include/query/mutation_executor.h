/**
 * @file mutation_executor.h
 * @brief MutationExecutor — executes MutationExecutionPlans against storage.
 * @version 1.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 4 implementation (EPIC-004) — StorageContext::get() added for rollback support.
 */

#pragma once

#include "query/mutation_execution_plan.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace themis {
namespace query {

class MutationExecutor {
public:
    // -----------------------------------------------------------------------
    // StorageContext
    // -----------------------------------------------------------------------

    struct StorageContext {
        /**
         * @brief Storage Context.
         * @return Return value.
         */
        virtual ~StorageContext() = default;

        /**
         * @brief Put.
         * @param[in] collection Input parameter.
         * @param[in] key Input parameter.
         * @param[in] value Input parameter.
         * @return True when the operation succeeds.
         */
        virtual bool put(std::string_view collection,
                         std::string_view key,
                         std::string_view value) = 0;

        /**
         * @brief Remove.
         * @param[in] collection Input parameter.
         * @param[in] key Input parameter.
         * @return True when the operation succeeds.
         */
        virtual bool remove(std::string_view collection,
                            std::string_view key) = 0;

        /**
         * @brief Exists.
         * @param[in] collection Input parameter.
         * @param[in] key Input parameter.
         * @return True when the operation succeeds.
         */
        virtual bool exists(std::string_view collection,
                            std::string_view key) = 0;

        /**
         * @brief Generate Key.
         * @param[in] collection Input parameter.
         * @return Return value.
         */
        virtual std::string generateKey(std::string_view collection) = 0;

        /**
         * @brief Write WAL.
         * @param[in] collection Input parameter.
         * @param[in] entry Input parameter.
         * @return True when the operation succeeds.
         */
        virtual bool writeWAL(std::string_view        collection,
                               const nlohmann::json&  entry) = 0;

        /**
         * @brief Get.
         * @param[in] string_view Input parameter.
         * @param[in] string_view Input parameter.
         * @return Return value.
         * @details Implements get without additional internal calls.
         */
        virtual std::optional<std::string> get(std::string_view /*collection*/,
                                               std::string_view /*key*/) {
            return std::nullopt;
        }
    };

    // -----------------------------------------------------------------------
    // Constructor / Destructor
    // -----------------------------------------------------------------------

    /**
     * @brief Mutation Executor.
     * @return Return value.
     */
    explicit MutationExecutor() = default;
    ~MutationExecutor()         = default;

    // Non-copyable (stateless but there is no reason to copy)
    MutationExecutor(const MutationExecutor&)            = delete;
    MutationExecutor& operator=(const MutationExecutor&) = delete;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    [[nodiscard]] MutationResult execute(const MutationExecutionPlan& plan,
                                          StorageContext&              ctx) const;

private:
    [[nodiscard]] MutationResult executeInsert(const MutationExecutionPlan& plan,
                                                StorageContext&              ctx) const;

    [[nodiscard]] MutationResult executeUpdate(const MutationExecutionPlan& plan,
                                                StorageContext&              ctx) const;

    [[nodiscard]] MutationResult executeRemove(const MutationExecutionPlan& plan,
                                                StorageContext&              ctx) const;

    [[nodiscard]] MutationResult executeReplace(const MutationExecutionPlan& plan,
                                                 StorageContext&              ctx) const;

    [[nodiscard]] MutationResult executeUpsert(const MutationExecutionPlan& plan,
                                                StorageContext&              ctx) const;
};

} // namespace query
} // namespace themis
