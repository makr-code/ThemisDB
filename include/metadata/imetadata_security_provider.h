/**
 * @file imetadata_security_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — Metadata Security Provider Interface
 *
 * Pluggable RBAC / access-control interface for metadata operations.
 * Allows production deployments to enforce per-principal, per-operation, and
 * per-resource authorisation checks on all metadata access paths (schema
 * introspection, statistics, lineage, audit log, external catalog export, …).
 *
 * Implementations ship in this header-only file:
 *   - IMetadataSecurityProvider  — abstract interface
 *   - NoOpMetadataSecurityProvider — permits everything (default)
 *   - InMemoryRbacMetadataSecurityProvider — configurable in-memory RBAC
 *
 * Design constraints (FUTURE_ENHANCEMENTS.md §Security):
 *  - hasPermission() must be non-blocking; ≤ 1 µs on the hot path.
 *  - The wildcard resource "*" grants the operation on every resource.
 *  - The ADMIN operation implies all other operations on all resources.
 *  - Implementations must be thread-safe.
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

namespace themis {
namespace metadata {

// ── MetadataOperation ─────────────────────────────────────────────────────────

/**
 * @brief Operations that can be performed on metadata resources.
 *
 * Used by IMetadataSecurityProvider to authorise access.  Ordered from
 * most-specific (READ_SCHEMA) to broadest (ADMIN).
 */
enum class MetadataOperation {
    READ_SCHEMA,        ///< Read schema: getTables(), getColumns(), INFORMATION_SCHEMA
    WRITE_SCHEMA,       ///< Modify schema: createSchemaVersion(), rollback(), constraints
    READ_STATISTICS,    ///< Read statistics: getStats(), histogram queries
    WRITE_STATISTICS,   ///< Update statistics: refreshStats(), manual overrides
    READ_LINEAGE,       ///< Query lineage: getLineage(), upstream/downstream traversal
    WRITE_LINEAGE,      ///< Record lineage: recordTransformation()
    READ_AUDIT_LOG,     ///< Access audit log: queryAuditLog()
    ADMIN,              ///< Full administrative control (implies all other operations)
};

// ── MetadataAccessDeniedException ─────────────────────────────────────────────

/**
 * @brief Exception thrown by IMetadataSecurityProvider::assertPermission()
 *        when the requested operation is not permitted.
 */
class MetadataAccessDeniedException : public std::runtime_error {
public:
    MetadataAccessDeniedException(std::string_view  principal,
                                  MetadataOperation op,
                                  std::string_view  resource)
        : std::runtime_error(
              "Metadata access denied: principal='" + std::string(principal) +
              "' operation=" + std::to_string(static_cast<int>(op)) +
              " resource='" + std::string(resource) + "'")
        , principal_(principal)
        , operation_(op)
        , resource_(resource) {}

    const std::string& principal() const noexcept { return principal_; }
    MetadataOperation  operation() const noexcept { return operation_; }
    const std::string& resource()  const noexcept { return resource_; }

private:
    std::string       principal_;
    MetadataOperation operation_;
    std::string       resource_;
};

// ── IMetadataSecurityProvider ─────────────────────────────────────────────────

/**
 * @brief Abstract interface for metadata access-control decisions.
 *
 * Implementations MUST be thread-safe.
 */
class IMetadataSecurityProvider {
public:
    virtual ~IMetadataSecurityProvider() = default;

    /**
     * @brief Return true if @p principal may perform @p op on @p resource.
     *
     * @param principal  Identity string (user, service account, role, …).
     * @param op         The requested metadata operation.
     * @param resource   The affected resource name, or "*" for any resource.
     * @return true      if the operation is permitted; false otherwise.
     */
    virtual bool hasPermission(std::string_view   principal,
                               MetadataOperation  op,
                               std::string_view   resource) const = 0;

    /**
     * @brief Assert that @p principal may perform @p op on @p resource.
     *
     * @throws MetadataAccessDeniedException if permission is denied.
     */
    virtual void assertPermission(std::string_view   principal,
                                  MetadataOperation  op,
                                  std::string_view   resource) const = 0;
};

// ── NoOpMetadataSecurityProvider ──────────────────────────────────────────────

/**
 * @brief Default (no-op) implementation that permits every operation.
 *
 * Used when no RBAC configuration is required.  Production deployments
 * that need access control should replace this with
 * InMemoryRbacMetadataSecurityProvider or a custom implementation.
 */
class NoOpMetadataSecurityProvider : public IMetadataSecurityProvider {
public:
    bool hasPermission(std::string_view  /*principal*/,
                       MetadataOperation /*op*/,
                       std::string_view  /*resource*/) const override {
        return true;
    }

    void assertPermission(std::string_view  /*principal*/,
                          MetadataOperation /*op*/,
                          std::string_view  /*resource*/) const override {
        // always permitted — no-op
    }
};

// ── InMemoryRbacMetadataSecurityProvider ──────────────────────────────────────

/**
 * @brief Thread-safe in-memory RBAC implementation of IMetadataSecurityProvider.
 *
 * Permissions are granted per (principal, operation, resource) triple.
 * Two special rules apply:
 *  - Wildcard resource "*" grants the operation on every resource.
 *  - ADMIN operation implies all other operations on all resources.
 *
 * Example:
 * @code
 *   InMemoryRbacMetadataSecurityProvider sec;
 *   sec.grant("analyst",  MetadataOperation::READ_SCHEMA,    "*");
 *   sec.grant("dba",      MetadataOperation::WRITE_SCHEMA,   "*");
 *   sec.grant("ops",      MetadataOperation::ADMIN,          "*");
 * @endcode
 */
class InMemoryRbacMetadataSecurityProvider : public IMetadataSecurityProvider {
public:
    // ── Permission management ─────────────────────────────────────────────────

    /**
     * @brief Grant @p principal permission to execute @p op on @p resource.
     *
     * @param principal  Identity string.
     * @param op         The operation to permit.
     * @param resource   Resource name, or "*" for all resources.
     */
    void grant(std::string_view  principal,
               MetadataOperation op,
               std::string_view  resource) {
        std::unique_lock<std::mutex> lk(mutex_);
        rules_[std::string(principal)][op].insert(std::string(resource));
    }

    /**
     * @brief Revoke a previously granted permission.
     *
     * No-op if the (principal, op, resource) triple was never granted.
     */
    void revoke(std::string_view  principal,
                MetadataOperation op,
                std::string_view  resource) {
        std::unique_lock<std::mutex> lk(mutex_);
        auto p_it = rules_.find(std::string(principal));
        if (p_it == rules_.end()) {
          return;
        }
        auto o_it = p_it->second.find(op);
        if (o_it == p_it->second.end()) {
          return;
        }
        o_it->second.erase(std::string(resource));
    }

    /**
     * @brief Remove all permissions for @p principal.
     */
    void revokeAll(std::string_view principal) {
        std::unique_lock<std::mutex> lk(mutex_);
        rules_.erase(std::string(principal));
    }

    // ── IMetadataSecurityProvider ─────────────────────────────────────────────

    bool hasPermission(std::string_view  principal,
                       MetadataOperation op,
                       std::string_view  resource) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return hasPermission_(principal, op, resource);
    }

    void assertPermission(std::string_view  principal,
                          MetadataOperation op,
                          std::string_view  resource) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        if (!hasPermission_(principal, op, resource)) {
            throw MetadataAccessDeniedException(principal, op, resource);
        }
    }

private:
    // Invariant: called with mutex_ held.
    bool hasPermission_(std::string_view  principal,
                        MetadataOperation op,
                        std::string_view  resource) const {
        auto p_it = rules_.find(std::string(principal));
        if (p_it == rules_.end()) {
          return false;
        }

        // ADMIN implies all operations on all resources.
        auto admin_it = p_it->second.find(MetadataOperation::ADMIN);
        if (admin_it != p_it->second.end()) {
            if (admin_it->second.count("*") > 0 ||
                admin_it->second.count(std::string(resource)) > 0) {
                return true;
            }
        }

        // Check the specific requested operation.
        auto o_it = p_it->second.find(op);
        if (o_it == p_it->second.end()) {
          return false;
        }

        // Wildcard resource or exact resource match.
        return o_it->second.count("*") > 0 ||
               o_it->second.count(std::string(resource)) > 0;
    }

    mutable std::mutex mutex_;
    // principal → operation → set<resource>
    std::map<std::string,
        std::map<MetadataOperation,
            std::set<std::string>
        >
    > rules_;
};

} // namespace metadata
} // namespace themis

