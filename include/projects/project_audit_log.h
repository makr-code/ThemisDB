/**
 * @file project_audit_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
// Project activity audit log REST API interface
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <optional>

namespace themis { namespace projects {

enum class ProjectAuditAction {
    PROJECT_CREATED, PROJECT_DELETED, PROJECT_UPDATED,
    COLLECTION_CREATED, COLLECTION_DELETED,
    DOCUMENT_INSERTED, DOCUMENT_UPDATED, DOCUMENT_DELETED,
    INDEX_CREATED, INDEX_DROPPED,
    PERMISSION_GRANTED, PERMISSION_REVOKED,
    BUNDLE_EXPORTED, BUNDLE_IMPORTED,
    TEMPLATE_APPLIED,
};

struct ProjectAuditEntry {
    std::string entry_id;
    std::string project_id;
    ProjectAuditAction action;
    std::string actor_id;
    std::string actor_type;
    std::string resource_id;
    std::string resource_type;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> details;
    std::string ip_address;
    std::string request_id;
};

struct AuditQueryOptions {
    std::string project_id;
    std::optional<ProjectAuditAction> action_filter;
    std::string actor_id_filter;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    size_t limit = 100;
    size_t offset = 0;
    std::string sort_direction = "desc";
};

/** @brief I project audit log. */
class IProjectAuditLog {
public:
    virtual ~IProjectAuditLog() = default;
    virtual void record(const ProjectAuditEntry& entry) = 0;
    [[nodiscard]] virtual std::vector<ProjectAuditEntry> query(const AuditQueryOptions& opts) const = 0;
    [[nodiscard]] virtual size_t count(const AuditQueryOptions& opts) const = 0;
    [[nodiscard]] virtual bool purge(const std::string& project_id,
                       std::chrono::system_clock::time_point before) = 0;
};

}} // namespace themis::projects
