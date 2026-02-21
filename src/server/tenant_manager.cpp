/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tenant_manager.cpp                                 ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     483                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/tenant_manager.h"
#include "utils/logger.h"
#include <sstream>
#include <iomanip>

namespace themis {

TenantManager& TenantManager::instance() {
    static TenantManager instance;
    return instance;
}

TenantManager::TenantManager() {
    // Create default configuration - SECURE BY DEFAULT
    config_.default_tenant_id = "default";
    config_.allow_default_tenant = false;  // Changed to false for production-ready security
    config_.tenant_header = "X-Tenant-ID";
    config_.tenant_path_prefix = "/tenants/";
    config_.global_max_tenants = 1000;
    config_.enforce_quotas = true;
    
    // ensureDefaultTenant() checks allow_default_tenant flag internally
    // and only creates the default tenant if the flag is true
    ensureDefaultTenant();
}

void TenantManager::configure(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    ensureDefaultTenant();
    THEMIS_INFO("TenantManager configured: tenant_header='{}', default_tenant='{}'",
                config_.tenant_header, config_.default_tenant_id);
}

void TenantManager::ensureDefaultTenant() {
    if (config_.allow_default_tenant && tenants_.find(config_.default_tenant_id) == tenants_.end()) {
        TenantConfig defaultTenant;
        defaultTenant.tenant_id = config_.default_tenant_id;
        defaultTenant.display_name = "Default Tenant";
        defaultTenant.enabled = true;
        defaultTenant.created_at = std::chrono::system_clock::now();
        defaultTenant.updated_at = defaultTenant.created_at;
        tenants_[config_.default_tenant_id] = defaultTenant;
        usage_[config_.default_tenant_id] = std::make_unique<TenantUsage>();
        usage_[config_.default_tenant_id]->tenant_id = config_.default_tenant_id;
    }
}

TenantManager::CreateResult TenantManager::createTenant(const TenantConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate tenant ID
    if (config.tenant_id.empty()) {
        THEMIS_WARN("TenantManager: Cannot create tenant with empty ID");
        return CreateResult::InvalidConfig;
    }
    
    // Check if already exists
    if (tenants_.find(config.tenant_id) != tenants_.end()) {
        THEMIS_WARN("TenantManager: Tenant '{}' already exists", config.tenant_id);
        return CreateResult::AlreadyExists;
    }
    
    // Check global tenant limit
    if (tenants_.size() >= config_.global_max_tenants) {
        THEMIS_WARN("TenantManager: Global tenant limit ({}) reached", config_.global_max_tenants);
        return CreateResult::QuotaExceeded;
    }
    
    // Create tenant
    TenantConfig newConfig = config;
    newConfig.created_at = std::chrono::system_clock::now();
    newConfig.updated_at = newConfig.created_at;
    
    tenants_[config.tenant_id] = newConfig;
    usage_[config.tenant_id] = std::make_unique<TenantUsage>();
    usage_[config.tenant_id]->tenant_id = config.tenant_id;
    
    THEMIS_INFO("TenantManager: Created tenant '{}' ({})", config.tenant_id, config.display_name);
    return CreateResult::Success;
}

bool TenantManager::updateTenant(const TenantConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tenants_.find(config.tenant_id);
    if (it == tenants_.end()) {
        THEMIS_WARN("TenantManager: Cannot update non-existent tenant '{}'", config.tenant_id);
        return false;
    }
    
    // Preserve created_at, update updated_at
    TenantConfig updated = config;
    updated.created_at = it->second.created_at;
    updated.updated_at = std::chrono::system_clock::now();
    
    it->second = updated;
    THEMIS_INFO("TenantManager: Updated tenant '{}'", config.tenant_id);
    return true;
}

bool TenantManager::deleteTenant(std::string_view tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string tid(tenant_id);
    
    // Don't allow deleting default tenant
    if (tid == config_.default_tenant_id) {
        THEMIS_WARN("TenantManager: Cannot delete default tenant");
        return false;
    }
    
    auto it = tenants_.find(tid);
    if (it == tenants_.end()) {
        return false;
    }
    
    tenants_.erase(it);
    usage_.erase(tid);
    
    THEMIS_INFO("TenantManager: Deleted tenant '{}'", tenant_id);
    return true;
}

bool TenantManager::setTenantEnabled(std::string_view tenant_id, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tenants_.find(std::string(tenant_id));
    if (it == tenants_.end()) {
        return false;
    }
    
    it->second.enabled = enabled;
    it->second.updated_at = std::chrono::system_clock::now();
    
    THEMIS_INFO("TenantManager: Tenant '{}' {}", tenant_id, enabled ? "enabled" : "disabled");
    return true;
}

std::optional<TenantConfig> TenantManager::getTenant(std::string_view tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tenants_.find(std::string(tenant_id));
    if (it != tenants_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<TenantConfig> TenantManager::listTenants() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<TenantConfig> result;
    result.reserve(tenants_.size());
    for (const auto& [id, config] : tenants_) {
        result.push_back(config);
    }
    return result;
}

bool TenantManager::tenantExists(std::string_view tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tenants_.find(std::string(tenant_id)) != tenants_.end();
}

size_t TenantManager::getTenantCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tenants_.size();
}

std::optional<std::string> TenantManager::extractTenantId(
    const std::unordered_map<std::string, std::string>& headers,
    std::string_view path
) const {
    // Try header first
    auto it = headers.find(config_.tenant_header);
    if (it != headers.end() && !it->second.empty()) {
        return it->second;
    }
    
    // Try path prefix
    std::string pathStr(path);
    if (pathStr.find(config_.tenant_path_prefix) == 0) {
        size_t start = config_.tenant_path_prefix.length();
        size_t end = pathStr.find('/', start);
        if (end == std::string::npos) {
            end = pathStr.length();
        }
        if (end > start) {
            return pathStr.substr(start, end - start);
        }
    }
    
    // Return default tenant if allowed
    if (config_.allow_default_tenant) {
        return config_.default_tenant_id;
    }
    
    return std::nullopt;
}

std::optional<TenantContext> TenantManager::resolveContext(
    const std::unordered_map<std::string, std::string>& headers,
    std::string_view path,
    std::string_view user_id,
    const std::vector<std::string>& roles
) const {
    auto tenantId = extractTenantId(headers, path);
    if (!tenantId) {
        return std::nullopt;
    }
    
    auto config = getTenant(*tenantId);
    if (!config || !config->enabled) {
        return std::nullopt;
    }
    
    return TenantContext::fromConfig(*config, user_id, roles);
}

TenantManager::QuotaCheckResult TenantManager::checkQuota(
    std::string_view tenant_id,
    std::string_view resource_type,
    uint64_t requested_amount
) const {
    if (!config_.enforce_quotas) {
        return {true, ""};
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto configIt = tenants_.find(std::string(tenant_id));
    if (configIt == tenants_.end()) {
        return {false, "Tenant not found"};
    }
    
    const auto& config = configIt->second;
    auto usageIt = usage_.find(std::string(tenant_id));
    if (usageIt == usage_.end()) {
        return {false, "Usage tracking not initialized"};
    }
    
    const auto& usage = *usageIt->second;
    
    if (resource_type == "storage") {
        if (config.max_storage_bytes > 0) {
            uint64_t current = usage.storage_bytes_used.load();
            if (current + requested_amount > config.max_storage_bytes) {
                return {false, "Storage quota exceeded"};
            }
        }
    } else if (resource_type == "documents") {
        if (config.max_documents > 0) {
            uint64_t current = usage.document_count.load();
            if (current + requested_amount > config.max_documents) {
                return {false, "Document quota exceeded"};
            }
        }
    } else if (resource_type == "collections") {
        if (config.max_collections > 0) {
            uint64_t current = usage.collection_count.load();
            if (current + requested_amount > config.max_collections) {
                return {false, "Collection quota exceeded"};
            }
        }
    } else if (resource_type == "connections") {
        uint32_t current = usage.active_connections.load();
        if (current >= config.max_connections) {
            return {false, "Connection limit exceeded"};
        }
    } else if (resource_type == "queries") {
        uint32_t current = usage.active_queries.load();
        if (current >= config.max_concurrent_queries) {
            return {false, "Concurrent query limit exceeded"};
        }
    }
    
    return {true, ""};
}

TenantUsage* TenantManager::getUsage(std::string_view tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = usage_.find(std::string(tenant_id));
    return it != usage_.end() ? it->second.get() : nullptr;
}

const TenantUsage* TenantManager::getUsage(std::string_view tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = usage_.find(std::string(tenant_id));
    return it != usage_.end() ? it->second.get() : nullptr;
}

void TenantManager::incrementStorage(std::string_view tenant_id, int64_t bytes) {
    if (auto* u = getUsage(tenant_id)) {
        if (bytes >= 0) {
            u->storage_bytes_used.fetch_add(static_cast<uint64_t>(bytes));
        } else {
            u->storage_bytes_used.fetch_sub(static_cast<uint64_t>(-bytes));
        }
    }
}

void TenantManager::incrementDocuments(std::string_view tenant_id, int64_t count) {
    if (auto* u = getUsage(tenant_id)) {
        if (count >= 0) {
            u->document_count.fetch_add(static_cast<uint64_t>(count));
        } else {
            u->document_count.fetch_sub(static_cast<uint64_t>(-count));
        }
    }
}

void TenantManager::incrementCollections(std::string_view tenant_id, int64_t count) {
    if (auto* u = getUsage(tenant_id)) {
        if (count >= 0) {
            u->collection_count.fetch_add(static_cast<uint64_t>(count));
        } else {
            u->collection_count.fetch_sub(static_cast<uint64_t>(-count));
        }
    }
}

void TenantManager::recordRequest(std::string_view tenant_id) {
    if (auto* u = getUsage(tenant_id)) {
        u->total_requests.fetch_add(1);
        u->last_activity_epoch.store(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }
}

void TenantManager::recordQuery(std::string_view tenant_id) {
    if (auto* u = getUsage(tenant_id)) {
        u->total_queries.fetch_add(1);
    }
}

void TenantManager::recordBytesRead(std::string_view tenant_id, uint64_t bytes) {
    if (auto* u = getUsage(tenant_id)) {
        u->total_bytes_read.fetch_add(bytes);
    }
}

void TenantManager::recordBytesWritten(std::string_view tenant_id, uint64_t bytes) {
    if (auto* u = getUsage(tenant_id)) {
        u->total_bytes_written.fetch_add(bytes);
    }
}

void TenantManager::recordRateLimited(std::string_view tenant_id) {
    if (auto* u = getUsage(tenant_id)) {
        u->rate_limited_requests.fetch_add(1);
    }
}

bool TenantManager::acquireConnection(std::string_view tenant_id) {
    auto check = checkQuota(tenant_id, "connections", 1);
    if (!check.allowed) {
        return false;
    }
    
    if (auto* u = getUsage(tenant_id)) {
        u->active_connections.fetch_add(1);
        return true;
    }
    return false;
}

void TenantManager::releaseConnection(std::string_view tenant_id) {
    if (auto* u = getUsage(tenant_id)) {
        u->active_connections.fetch_sub(1);
    }
}

bool TenantManager::acquireQuerySlot(std::string_view tenant_id) {
    auto check = checkQuota(tenant_id, "queries", 1);
    if (!check.allowed) {
        return false;
    }
    
    if (auto* u = getUsage(tenant_id)) {
        u->active_queries.fetch_add(1);
        return true;
    }
    return false;
}

void TenantManager::releaseQuerySlot(std::string_view tenant_id) {
    if (auto* u = getUsage(tenant_id)) {
        u->active_queries.fetch_sub(1);
    }
}

std::string TenantManager::getTenantKeyId(std::string_view tenant_id) const {
    auto config = getTenant(tenant_id);
    if (config && !config->encryption_key_id.empty()) {
        return config->encryption_key_id;
    }
    return "tenant:" + std::string(tenant_id) + ":master";
}

std::string TenantManager::getMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    oss << "# HELP themis_tenant_count Total number of tenants\n";
    oss << "# TYPE themis_tenant_count gauge\n";
    oss << "themis_tenant_count " << tenants_.size() << "\n\n";
    
    for (const auto& [id, usage] : usage_) {
        // Escape label value safely using a new string
        std::string tid;
        tid.reserve(id.size() + 4);  // Reserve some extra space for escapes
        for (char c : id) {
            if (c == '"' || c == '\\') {
                tid += '\\';
            }
            tid += c;
        }
        
        oss << "# HELP themis_tenant_storage_bytes Storage used by tenant\n";
        oss << "# TYPE themis_tenant_storage_bytes gauge\n";
        oss << "themis_tenant_storage_bytes{tenant=\"" << tid << "\"} " 
            << usage->storage_bytes_used.load() << "\n";
        
        oss << "# HELP themis_tenant_documents Document count for tenant\n";
        oss << "# TYPE themis_tenant_documents gauge\n";
        oss << "themis_tenant_documents{tenant=\"" << tid << "\"} " 
            << usage->document_count.load() << "\n";
        
        oss << "# HELP themis_tenant_connections Active connections for tenant\n";
        oss << "# TYPE themis_tenant_connections gauge\n";
        oss << "themis_tenant_connections{tenant=\"" << tid << "\"} " 
            << usage->active_connections.load() << "\n";
        
        oss << "# HELP themis_tenant_queries_active Active queries for tenant\n";
        oss << "# TYPE themis_tenant_queries_active gauge\n";
        oss << "themis_tenant_queries_active{tenant=\"" << tid << "\"} " 
            << usage->active_queries.load() << "\n";
        
        oss << "# HELP themis_tenant_requests_total Total requests for tenant\n";
        oss << "# TYPE themis_tenant_requests_total counter\n";
        oss << "themis_tenant_requests_total{tenant=\"" << tid << "\"} " 
            << usage->total_requests.load() << "\n";
        
        oss << "# HELP themis_tenant_rate_limited_total Rate limited requests for tenant\n";
        oss << "# TYPE themis_tenant_rate_limited_total counter\n";
        oss << "themis_tenant_rate_limited_total{tenant=\"" << tid << "\"} " 
            << usage->rate_limited_requests.load() << "\n\n";
    }
    
    return oss.str();
}

} // namespace themis
