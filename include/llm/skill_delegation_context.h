/**
 * @file skill_delegation_context.h
 * @brief Attenuating, host-created execution context for Agent Skills.
 */

#pragma once

#include "utils/expected.h"

#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace themis::llm {

/**
 * @brief One exact action/resource capability; identifiers are never patterns.
 */
struct SkillPermission {
    /// Exact host-defined capability verb; wildcard syntax is not supported.
    std::string action;
    /// Exact canonical resource identifier; wildcard syntax is not supported.
    std::string resource;

    friend bool operator<(const SkillPermission& left,
                          const SkillPermission& right) {
        return std::tie(left.action, left.resource) <
               std::tie(right.action, right.resource);
    }
};

/** @brief Ordered set of exact capabilities used for subset attenuation. */
using SkillPermissionSet = std::set<SkillPermission>;

/**
 * @brief Aggregate resource ceiling shared by a skill and all its descendants.
 */
struct SkillBudgetLimits {
    /// Maximum tool calls across the root and all derived contexts.
    std::size_t max_tool_calls = 0;
    /// Maximum candidates processed across the root and all derived contexts.
    std::size_t max_candidates = 0;
    /// Maximum content bytes processed across the root and all derived contexts.
    std::size_t max_content_bytes = 0;
};

/**
 * @brief Resource amounts consumed by one operation.
 */
struct SkillBudgetUsage {
    /// Tool calls charged by this operation.
    std::size_t tool_calls = 0;
    /// Candidates charged by this operation.
    std::size_t candidates = 0;
    /// Content bytes charged by this operation.
    std::size_t content_bytes = 0;
};

/**
 * @brief Immutable identity and attenuating permissions for a skill execution.
 *
 * A trusted host must create the root context from authenticated identity and
 * effective policy decisions. Skill text, model output, and tool arguments
 * must never be used to create or expand a root context. Child contexts retain
 * the same principal, tenant, and shared aggregate budget; they may only remove
 * exact capabilities or shorten the steady-clock expiry.
 *
 * @note ToolRegistry enforces this context only for tools registered with its
 *       delegated handler API. Existing handlers and RAG execution are not
 *       constrained by this type and must not be considered authorized by it.
 * @note Identity and grants are immutable. Budget operations on contexts that
 *       share a root are synchronized and safe to call concurrently.
 */
class SkillDelegationContext final {
public:
    /** @brief Monotonic clock used to avoid wall-clock adjustments. */
    using Clock = std::chrono::steady_clock;
    /** @brief Expiry point in the monotonic clock domain. */
    using TimePoint = Clock::time_point;
    /**
     * @brief Host policy check for one exact grant and authenticated identity.
     *
     * The host must bind this callback to its authenticated request context,
     * reject identities that do not match that context, and return false for
     * policy denial or evaluation failure. The callback is invoked
     * synchronously and must not retain the string views. Standard exceptions
     * are converted to an access-denied result.
     */
    using PermissionAuthorizer = std::function<bool(
        std::string_view principal_id,
        std::string_view tenant_id,
        const SkillPermission& permission)>;

    /**
     * @brief Issue a root context after host policy approves every exact grant.
     * @param principal_id Authenticated principal identifier; must not be empty.
     * @param tenant_id Authenticated tenant identifier; must not be empty.
     * @param permissions Exact grants to check against host policy.
     * @param expires_at Monotonic expiry; must be in the future.
     * @param budget Aggregate ceilings shared with all derived contexts.
     * @param permission_authorizer Trusted host callback that confirms each
     *        grant for the supplied principal and tenant; must be non-empty.
     * @return A root context, or an invalid-argument error for malformed input.
     *         Returns `ERR_QUERY_ACCESS_DENIED` if any grant is denied.
     * @note The callback is called once per permission in set order. A false
     *       decision or standard exception fails issuance closed.
     */
    [[nodiscard]] static Result<SkillDelegationContext> createRoot(
        std::string principal_id,
        std::string tenant_id,
        SkillPermissionSet permissions,
        TimePoint expires_at,
        SkillBudgetLimits budget,
        const PermissionAuthorizer& permission_authorizer) {
        if (principal_id.empty() || tenant_id.empty() ||
            expires_at <= Clock::now() || !permission_authorizer) {
            return Err<SkillDelegationContext>(
                errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                "skill delegation requires identity, future expiry, and an authorizer");
        }

        for (const auto& permission : permissions) {
            if (!isValidPermission(permission)) {
                return Err<SkillDelegationContext>(
                    errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                    "skill permissions require non-empty exact identifiers");
            }
        }

        for (const auto& permission : permissions) {
            bool authorized = false;
            try {
                authorized =
                    permission_authorizer(principal_id, tenant_id, permission);
            } catch (const std::exception&) {
                return Err<SkillDelegationContext>(
                    errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
                    "host policy evaluation failed for a skill delegation permission");
            }
            if (!authorized) {
                return Err<SkillDelegationContext>(
                    errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
                    "host policy denied a skill delegation permission");
            }
        }

        return Ok(SkillDelegationContext(
            std::move(principal_id), std::move(tenant_id),
            std::move(permissions), expires_at,
            std::make_shared<BudgetState>(budget)));
    }

    /**
     * @brief Derive a child context without changing identity or resetting budget.
     * @param permissions Requested exact grants, which must be a subset.
     * @param expires_at Requested expiry, which must not exceed the parent.
     * @return The attenuated child, or an explicit invalid/denied error.
     */
    [[nodiscard]] Result<SkillDelegationContext> attenuate(
        SkillPermissionSet permissions,
        TimePoint expires_at) const {
        if (!isActive() || expires_at <= Clock::now() ||
            expires_at > expires_at_) {
            return Err<SkillDelegationContext>(
                errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
                "skill delegation expiry is invalid or exceeds its parent");
        }

        for (const auto& permission : permissions) {
            if (!isValidPermission(permission)) {
                return Err<SkillDelegationContext>(
                    errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                    "skill permissions require non-empty exact identifiers");
            }
            if (permissions_.find(permission) == permissions_.end()) {
                return Err<SkillDelegationContext>(
                    errors::ErrorCode::ERR_QUERY_ACCESS_DENIED,
                    "requested skill permission exceeds its parent delegation");
            }
        }

        return Ok(SkillDelegationContext(
            principal_id_, tenant_id_, std::move(permissions), expires_at,
            budget_state_));
    }

    /**
     * @brief Check whether this exact capability is present and not expired.
     * @param action Exact capability verb.
     * @param resource Exact canonical resource identifier.
     * @return True only for an exact grant while the context is active.
     */
    [[nodiscard]] bool allows(std::string_view action,
                              std::string_view resource) const {
        if (!isActive()) {
            return false;
        }
        for (const auto& permission : permissions_) {
            if (std::string_view(permission.action) == action &&
                std::string_view(permission.resource) == resource) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Check expiry using the monotonic clock.
     * @return True only while the current monotonic time is before expiry.
     */
    [[nodiscard]] bool isActive() const {
        return Clock::now() < expires_at_;
    }

    /**
     * @brief Atomically charge the shared budget, rejecting an over-limit use.
     *
     * No counters are changed when any requested dimension would exceed its
     * ceiling. Expired contexts cannot consume budget.
     * @param usage Resource amounts requested by one operation.
     * @return True if all dimensions were charged; false with no changes if
     *         the context expired or any ceiling would be exceeded.
     */
    [[nodiscard]] bool consumeBudget(const SkillBudgetUsage& usage) const {
        std::lock_guard lock(budget_state_->mutex);
        if (Clock::now() >= expires_at_ ||
            budget_state_->consumed.tool_calls >
                budget_state_->limits.max_tool_calls ||
            budget_state_->consumed.candidates >
                budget_state_->limits.max_candidates ||
            budget_state_->consumed.content_bytes >
                budget_state_->limits.max_content_bytes ||
            usage.tool_calls > budget_state_->limits.max_tool_calls -
                                   budget_state_->consumed.tool_calls ||
            usage.candidates > budget_state_->limits.max_candidates -
                                   budget_state_->consumed.candidates ||
            usage.content_bytes > budget_state_->limits.max_content_bytes -
                                      budget_state_->consumed.content_bytes) {
            return false;
        }

        budget_state_->consumed.tool_calls += usage.tool_calls;
        budget_state_->consumed.candidates += usage.candidates;
        budget_state_->consumed.content_bytes += usage.content_bytes;
        return true;
    }

    /**
     * @brief Return the remaining shared budget.
     * @return Remaining aggregate ceilings after all root/descendant charges.
     */
    [[nodiscard]] SkillBudgetLimits remainingBudget() const {
        std::lock_guard lock(budget_state_->mutex);
        return {
            budget_state_->consumed.tool_calls >
                    budget_state_->limits.max_tool_calls
                ? 0
                : budget_state_->limits.max_tool_calls -
                      budget_state_->consumed.tool_calls,
            budget_state_->consumed.candidates >
                    budget_state_->limits.max_candidates
                ? 0
                : budget_state_->limits.max_candidates -
                      budget_state_->consumed.candidates,
            budget_state_->consumed.content_bytes >
                    budget_state_->limits.max_content_bytes
                ? 0
                : budget_state_->limits.max_content_bytes -
                      budget_state_->consumed.content_bytes,
        };
    }

    /** @brief Return the immutable authenticated principal identifier. */
    [[nodiscard]] const std::string& principalId() const noexcept {
        return principal_id_;
    }

    /** @brief Return the immutable authenticated tenant identifier. */
    [[nodiscard]] const std::string& tenantId() const noexcept {
        return tenant_id_;
    }

    /** @brief Return the immutable exact grants held by this context. */
    [[nodiscard]] const SkillPermissionSet& permissions() const noexcept {
        return permissions_;
    }

    /** @brief Return the monotonic expiry inherited by this context. */
    [[nodiscard]] TimePoint expiresAt() const noexcept {
        return expires_at_;
    }

private:
    struct BudgetState final {
        explicit BudgetState(SkillBudgetLimits initial_limits)
            : limits(initial_limits) {}

        mutable std::mutex mutex;
        SkillBudgetLimits limits;
        SkillBudgetUsage consumed{};
    };

    SkillDelegationContext(std::string principal_id,
                           std::string tenant_id,
                           SkillPermissionSet permissions,
                           TimePoint expires_at,
                           std::shared_ptr<BudgetState> budget_state)
        : principal_id_(std::move(principal_id)),
          tenant_id_(std::move(tenant_id)),
          permissions_(std::move(permissions)),
          expires_at_(expires_at),
          budget_state_(std::move(budget_state)) {}

    static bool isValidPermission(const SkillPermission& permission) {
        return !permission.action.empty() && !permission.resource.empty() &&
               permission.action.find('*') == std::string::npos &&
               permission.resource.find('*') == std::string::npos;
    }

    std::string principal_id_;
    std::string tenant_id_;
    SkillPermissionSet permissions_;
    TimePoint expires_at_;
    std::shared_ptr<BudgetState> budget_state_;
};

} // namespace themis::llm
