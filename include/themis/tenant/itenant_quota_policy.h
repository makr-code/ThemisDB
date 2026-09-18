/**
 * @file itenant_quota_policy.h
 * @brief Abstract per-tenant resource-quota policy for ThemisDB multi-tenant deployments.
 *
 * Governs storage, document count, collection count, concurrent query slot
 * allocation, and per-tenant request rates.  Sits in Tier 2 of the
 * four-tier resource-governance chain:
 *
 * @code
 *   compile-time constexpr (edition.h)    ← absolute ceiling, never overridable
 *   RuntimeLicenseGate                    ← edition-tier ceiling
 *   ITenantQuotaPolicy  (this file)       ← signed-plugin fine-tuning
 *   TenantConfig                          ← per-deployment operational tuning
 * @endcode
 *
 * All implementations must be individually thread-safe.
 *
 * @note This interface is part of the edition-policy plugin contract
 *       (IEditionPolicyPlugin::createTenantQuotaPolicy).  Claimed limits are
 *       validated against the compile-time ceilings in edition.h
 *       (TENANT_MAX_STORAGE_BYTES, TENANT_MAX_DOCUMENTS, etc.) before
 *       EditionManager accepts the policy.
 */

#pragma once

#include <cstdint>
#include <string>

namespace themis {
namespace tenant {

/**
 * @brief Abstract per-tenant resource-quota policy.
 *
 * Controls five resource axes per tenant:
 *  - **Storage** — maximum bytes persisted on behalf of a tenant.
 *  - **Documents** — maximum document count per tenant.
 *  - **Collections** — maximum collection count per tenant.
 *  - **Concurrent queries** — maximum simultaneous query slots per tenant.
 *  - **Request rate** — maximum requests per second per tenant (token-bucket).
 *
 * Implementations are installed into EditionManager via
 * `EditionManager::installTenantQuotaPolicy()` and consulted by the server
 * layer before admitting a tenant request.
 */
class ITenantQuotaPolicy {
public:
    virtual ~ITenantQuotaPolicy() = default;

    // Non-copyable, non-movable by default.
    ITenantQuotaPolicy(const ITenantQuotaPolicy&)            = delete;
    ITenantQuotaPolicy& operator=(const ITenantQuotaPolicy&) = delete;

    // -------------------------------------------------------------------------
    // Storage quota
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff @p tenant_id may store @p additional_bytes more bytes.
     *
     * Does not modify accounting state.  Returns true unconditionally when
     * maxStorageBytes() == 0 (unlimited).
     *
     * @param tenant_id        Tenant whose quota is checked.
     * @param additional_bytes Bytes the operation intends to add.
     */
    [[nodiscard]] virtual bool canAllocateStorage(const std::string& tenant_id,
                                                  uint64_t additional_bytes) const = 0;

    /**
     * @brief Maximum storage bytes allowed per tenant by this policy.
     *
     * @return Byte limit; 0 signals unlimited.
     */
    [[nodiscard]] virtual uint64_t maxStorageBytes() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Document and collection quotas
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff @p tenant_id may create @p count more documents.
     *
     * @param tenant_id  Tenant whose quota is checked.
     * @param count      Number of documents the caller intends to insert.
     */
    [[nodiscard]] virtual bool canAddDocuments(const std::string& tenant_id,
                                               uint64_t count) const = 0;

    /**
     * @brief Maximum document count per tenant; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t maxDocuments() const noexcept = 0;

    /**
     * @brief Return true iff @p tenant_id may create one more collection.
     *
     * @param tenant_id  Tenant whose quota is checked.
     */
    [[nodiscard]] virtual bool canAddCollection(const std::string& tenant_id) const = 0;

    /**
     * @brief Maximum collection count per tenant; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t maxCollections() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Concurrent-query slot
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff @p tenant_id may dispatch one more concurrent query.
     *
     * Does not modify accounting state — call onQueryStarted() only after the
     * query has actually been admitted.
     *
     * @param tenant_id  Tenant whose slot availability is checked.
     */
    [[nodiscard]] virtual bool canStartQuery(const std::string& tenant_id) const = 0;

    /**
     * @brief Notify the policy that @p tenant_id started a query.
     *
     * Updates concurrent-query accounting.  Thread-safe.
     *
     * @param tenant_id  Tenant identifier.
     */
    virtual void onQueryStarted(const std::string& tenant_id) = 0;

    /**
     * @brief Notify the policy that @p tenant_id's query has finished.
     *
     * Releases the concurrent-query slot.  Implementations must clamp to zero
     * on mismatched calls.  Thread-safe.
     *
     * @param tenant_id  Tenant identifier.
     */
    virtual void onQueryFinished(const std::string& tenant_id) = 0;

    /**
     * @brief Maximum concurrent queries per tenant; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t maxConcurrentQueries() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Rate limiting
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff @p tenant_id is allowed to send one more request now.
     *
     * Uses token-bucket semantics internally.  May consume a token — callers
     * must not issue the request if this method returns false.
     *
     * @param tenant_id  Tenant whose rate limit is checked.
     */
    [[nodiscard]] virtual bool allowRequest(const std::string& tenant_id) = 0;

    /**
     * @brief Maximum requests per second per tenant; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t maxRequestsPerSecond() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Status
    // -------------------------------------------------------------------------

    /**
     * @brief Return true when per-tenant quota enforcement is active.
     *
     * Implementations should return false when every limit is 0 (unlimited)
     * and enforcement is a no-op, so callers can skip the check on hot paths.
     */
    [[nodiscard]] virtual bool isEnforced() const noexcept = 0;

protected:
    ITenantQuotaPolicy() = default;
    ITenantQuotaPolicy(ITenantQuotaPolicy&&) noexcept = default;
    ITenantQuotaPolicy& operator=(ITenantQuotaPolicy&&) noexcept = default;
};

} // namespace tenant
} // namespace themis
