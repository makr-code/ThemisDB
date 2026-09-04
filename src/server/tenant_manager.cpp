/**
 * @file tenant_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/tenant_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace themis {

/** @brief Return process-global TenantManager singleton. */
TenantManager& TenantManager::instance() {
    static TenantManager instance;
    return instance;
}

/** @brief Construct manager with secure defaults and optional default tenant bootstrap. */
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

/**
 * @brief Apply manager configuration and rebuild derived indexes.
 * @param config New configuration snapshot.
 */
void TenantManager::configure(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    ensureDefaultTenant();
    rebuildDomainIndex();
    THEMIS_INFO("TenantManager configured: tenant_header='{}', default_tenant='{}'",
                config_.tenant_header, config_.default_tenant_id);
}

/** @brief Ensure default tenant exists when default-tenant mode is enabled. */
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

// static
/**
 * @brief Normalize host/domain for case-insensitive lookup.
 * @param host Host header value (may include port suffix).
 * @return Lower-case hostname without optional numeric port.
 */
std::string TenantManager::normaliseDomain(std::string_view host) {
    // Strip optional port suffix (":NNN")
    // A valid port is at most 5 digits (1-65535), so only strip when the
    // suffix after the last ':' is 1-5 all-digit characters.
    std::string result(host);
    const auto colon = result.rfind(':');
    if (colon != std::string::npos) {
        const std::size_t suffix_len = result.size() - colon - 1;
        if (suffix_len >= 1 && suffix_len <= 5) {
            const bool is_port = std::all_of(result.begin() + static_cast<std::ptrdiff_t>(colon) + 1,
                                             result.end(),
                                             [](unsigned char c){ return std::isdigit(c) != 0; });
            if (is_port) {
                result.erase(colon);
            }
        }
    }
    // Lower-case for case-insensitive comparison
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return result;
}

/** @brief Rebuild domain-to-tenant reverse index from current tenant configs. */
void TenantManager::rebuildDomainIndex() {
    domain_to_tenant_.clear();
    for (const auto& [tid, cfg] : tenants_) {
        for (const auto& domain : cfg.custom_domains) {
            const std::string key = normaliseDomain(domain);
            if (!key.empty()) {
                domain_to_tenant_[key] = tid;
            }
        }
    }
}

/**
 * @brief Create tenant and initialize usage tracking state.
 * @param config Tenant configuration.
 * @return Create result code with validation/quota outcome.
 */
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
    if (static_cast<int>(tenants_.size()) > = config_.global_max_tenants) {
        THEMIS_WARN("TenantManager: Global tenant limit ({}) reached", config_.global_max_tenants);
        return CreateResult::QuotaExceeded;
    }

    // Validate custom domain uniqueness
    if (!config.custom_domain.empty()) {
        if (domain_to_tenant_.count(config.custom_domain)) {
            THEMIS_WARN("TenantManager: Custom domain '{}' is already registered to tenant '{}'",
                        config.custom_domain, domain_to_tenant_.at(config.custom_domain));
            return CreateResult::InvalidConfig;
        }
    }
    
    // Create tenant
    TenantConfig newConfig = config;
    newConfig.created_at = std::chrono::system_clock::now();
    newConfig.updated_at = newConfig.created_at;
    
    tenants_[config.tenant_id] = newConfig;
    usage_[config.tenant_id] = std::make_unique<TenantUsage>();
    usage_[config.tenant_id]->tenant_id = config.tenant_id;

    // Index custom domains
    for (const auto& domain : newConfig.custom_domains) {
        const std::string key = normaliseDomain(domain);
        if (!key.empty()) {
            domain_to_tenant_[key] = config.tenant_id;
            THEMIS_INFO("TenantManager: Registered custom domain '{}' for tenant '{}'",
                        key, config.tenant_id);
        }
    }
    
    THEMIS_INFO("TenantManager: Created tenant '{}' ({})", config.tenant_id, config.display_name);
    return CreateResult::Success;
}

/**
 * @brief Update existing tenant config and refresh domain mappings.
 * @param config Updated tenant configuration.
 * @return true on success.
 */
bool TenantManager::updateTenant(const TenantConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tenants_.find(config.tenant_id);
    if (it == tenants_.end()) {
        THEMIS_WARN("TenantManager: Cannot update non-existent tenant '{}'", config.tenant_id);
        return false;
    }

    // Validate custom domain uniqueness (allow same tenant to keep its own domain)
    if (!config.custom_domain.empty()) {
        auto domainIt = domain_to_tenant_.find(config.custom_domain);
        if (domainIt != domain_to_tenant_.end() && domainIt->second != config.tenant_id) {
            THEMIS_WARN("TenantManager: Custom domain '{}' is already registered to tenant '{}'",
                        config.custom_domain, domainIt->second);
            return false;
        }
    }

    // Remove old domain mapping if the domain changed
    const std::string& old_domain = it->second.custom_domain;
    if (!old_domain.empty() && old_domain != config.custom_domain) {
        domain_to_tenant_.erase(old_domain);
        THEMIS_INFO("TenantManager: Unregistered custom domain '{}' for tenant '{}'",
                    old_domain, config.tenant_id);
    }
    
    // Preserve created_at, update updated_at
    TenantConfig updated = config;
    updated.created_at = it->second.created_at;
    updated.updated_at = std::chrono::system_clock::now();
    
    it->second = updated;
    // Rebuild the full domain index to reflect any changes to custom_domains
    rebuildDomainIndex();

    // Register new domain mapping
    if (!updated.custom_domain.empty()) {
        domain_to_tenant_[updated.custom_domain] = updated.tenant_id;
        THEMIS_INFO("TenantManager: Registered custom domain '{}' for tenant '{}'",
                    updated.custom_domain, updated.tenant_id);
    }

    THEMIS_INFO("TenantManager: Updated tenant '{}'", config.tenant_id);
    return true;
}

/**
 * @brief Delete tenant and associated usage/domain state.
 * @param tenant_id Tenant identifier.
 * @return true on success.
 */
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

    // Remove any custom domain mappings that belong to this tenant
    for (const auto& domain : it->second.custom_domains) {
        domain_to_tenant_.erase(normaliseDomain(domain));
    }

    // Unregister custom domain mapping
    if (!it->second.custom_domain.empty()) {
        domain_to_tenant_.erase(it->second.custom_domain);
        THEMIS_INFO("TenantManager: Unregistered custom domain '{}' for tenant '{}'",
                    it->second.custom_domain, tenant_id);
    }
    
    tenants_.erase(it);
    usage_.erase(tid);
    
    THEMIS_INFO("TenantManager: Deleted tenant '{}'", tenant_id);
    return true;
}

/**
 * @brief Toggle tenant enabled state.
 * @param tenant_id Tenant identifier.
 * @param enabled Desired enabled state.
 * @return true when tenant exists and state was updated.
 */
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

/** @brief Get tenant configuration snapshot by id. */
std::optional<TenantConfig> TenantManager::getTenant(std::string_view tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tenants_.find(std::string(tenant_id));
    if (it != tenants_.end()) {
        return it->second;
    }
    return std::nullopt;
}

/** @brief Return snapshot list of all tenants. */
std::vector<TenantConfig> TenantManager::listTenants() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<TenantConfig> result = {};

    result.reserve(tenants_.size());
    for (const auto& [id, config] : tenants_) {
        result.push_back(config);
    }
    return result;
}

/** @brief Check whether tenant exists. */
bool TenantManager::tenantExists(std::string_view tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tenants_.find(std::string(tenant_id)) != tenants_.end();
}

/** @brief Return number of configured tenants. */
size_t TenantManager::getTenantCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tenants_.size();
}

/**
 * @brief Extract tenant id from headers, domain mapping, path prefix, or default tenant.
 * @param headers Request headers.
 * @param path Request path.
 * @return Tenant id when found, otherwise std::nullopt.
 */
std::optional<std::string> TenantManager::extractTenantId(
    const std::unordered_map<std::string, std::string>& headers,
    std::string_view path
) const {
    // 1. Explicit tenant header takes highest priority
    // Try explicit tenant header first (highest priority)
    auto it = headers.find(config_.tenant_header);
    if (it != headers.end() && !it->second.empty()) {
        return it->second;
    }

    // 2. Custom domain routing via Host (or configured host header)
    // Try configured custom domain host header first, fallback to standard Host
    std::string hostHeaderName = !config_.custom_domain_host_header.empty() 
        ? config_.custom_domain_host_header 
        : "Host";
    auto hostIt = headers.find("Host");
    if (hostIt == headers.end()) {
        // Case-insensitive fallback: check lowercase "host"
        hostIt = headers.find("host");
    }
    if (hostIt != headers.end() && !hostIt->second.empty()) {
        const std::string key = normaliseDomain(hostIt->second);
        auto domIt = domain_to_tenant_.find(key);
        if (domIt != domain_to_tenant_.end()) {
            return domIt->second;
        }
    }
    
    // 3. Path-based tenant routing
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
    
    // 4. Return default tenant if allowed
    if (config_.allow_default_tenant) {
        return config_.default_tenant_id;
    }
    
    return std::nullopt;
}

/**
 * @brief Strip tenant prefix from path when present.
 * @param path Input request path.
 * @return Path without /tenants/{id} prefix, or original path.
 */
std::string TenantManager::stripTenantPath(std::string_view path) const {
    const std::string pathStr(path);
    if (pathStr.rfind(config_.tenant_path_prefix, 0) != 0) {
        return pathStr;  // Not a tenant-prefixed path
    }
    const size_t id_start = config_.tenant_path_prefix.size();
    const size_t slash_pos = pathStr.find('/', id_start);
    if (slash_pos != std::string::npos) {
        return pathStr.substr(slash_pos);
    }
    // Path has tenant ID but no trailing slash (e.g., "/tenants/acme-corp")
    return "/";
}

/**
 * @brief Rewrite tenant-prefixed path and expose extracted tenant id.
 * @param path Input request path.
 * @return Rewrite result containing effective path and optional tenant id.
 */
TenantManager::PathRewriteResult TenantManager::rewriteTenantPath(
    std::string_view path) const {
    const std::string pathStr(path);
    if (pathStr.rfind(config_.tenant_path_prefix, 0) != 0) {
        return {pathStr, "", false};  // Not a tenant-prefixed path
    }
    const size_t id_start = config_.tenant_path_prefix.size();
    const size_t slash_pos = pathStr.find('/', id_start);
    const size_t id_end = (slash_pos != std::string::npos)
                          ? slash_pos : pathStr.size();
    if (id_end <= id_start) {
        return {pathStr, "", false};  // No tenant ID segment found
    }
    const std::string tenant_id = pathStr.substr(id_start, id_end - id_start);
    const std::string effective_path = (slash_pos != std::string::npos)
                                       ? pathStr.substr(slash_pos) : "/";
    return {effective_path, tenant_id, true};
}

/**
 * @brief Resolve tenant via host/domain mapping.
 * @param host Host header value (may include port).
 * @return Tenant id when mapping exists.
 */
std::optional<std::string> TenantManager::resolveTenantByDomain(std::string_view host) const {
    // Strip port suffix if present
    std::string h(host);
    const auto colon = h.find(':');
    if (colon != std::string::npos) {
        h.resize(colon);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = domain_to_tenant_.find(h);
    if (it != domain_to_tenant_.end()) {
        return it->second;
    }
    return std::nullopt;
}

/**
 * @brief Resolve request context for enabled tenant.
 * @param headers Request headers.
 * @param path Request path.
 * @param user_id Optional user id.
 * @param roles Optional role list.
 * @return Tenant context when resolution succeeds and tenant is enabled.
 */
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

/**
 * @brief Validate tenant quota request.
 * @param tenant_id Tenant identifier.
 * @param resource_type Resource kind name.
 * @param requested_amount Requested increment.
 * @return Quota decision with optional reason.
 */
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

/** @brief Return mutable usage counters for tenant. */
TenantUsage* TenantManager::getUsage(std::string_view tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = usage_.find(std::string(tenant_id));
    return it != usage_.end() ? it->second.get() : nullptr;
}

/** @brief Return immutable usage counters for tenant. */
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

/** @brief Register custom domain mapping to tenant. */
bool TenantManager::registerCustomDomain(std::string_view tenant_id, std::string_view domain) {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string tid(tenant_id);
    if (tenants_.find(tid) == tenants_.end()) {
        THEMIS_WARN("TenantManager: Cannot register domain '{}' for non-existent tenant '{}'",
                    domain, tenant_id);
        return false;
    }

    const std::string key = normaliseDomain(domain);
    if (key.empty()) {
        return false;
    }

    // Reject if already mapped to a different tenant
    const auto it = domain_to_tenant_.find(key);
    if (it != domain_to_tenant_.end() && it->second != tid) {
        THEMIS_WARN("TenantManager: Domain '{}' is already registered to tenant '{}'",
                    domain, it->second);
        return false;
    }

    domain_to_tenant_[key] = tid;

    // Keep TenantConfig in sync
    auto& cfg = tenants_[tid];
    const std::string rawDomain(domain);
    if (std::find(cfg.custom_domains.begin(), cfg.custom_domains.end(), rawDomain)
            == cfg.custom_domains.end()) {
        cfg.custom_domains.push_back(rawDomain);
        cfg.updated_at = std::chrono::system_clock::now();
    }

    THEMIS_INFO("TenantManager: Registered custom domain '{}' for tenant '{}'", domain, tenant_id);
    return true;
}

/** @brief Remove custom domain mapping. */
bool TenantManager::unregisterCustomDomain(std::string_view domain) {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string key = normaliseDomain(domain);
    const auto it = domain_to_tenant_.find(key);
    if (it == domain_to_tenant_.end()) {
        return false;
    }

    const std::string tid = it->second;
    domain_to_tenant_.erase(it);

    // Remove from TenantConfig.custom_domains as well
    auto tenantIt = tenants_.find(tid);
    if (tenantIt != tenants_.end()) {
        const std::string rawDomain(domain);
        auto& domains = tenantIt->second.custom_domains;
        domains.erase(std::remove(domains.begin(), domains.end(), rawDomain), domains.end());
        tenantIt->second.updated_at = std::chrono::system_clock::now();
    }

    THEMIS_INFO("TenantManager: Unregistered custom domain '{}' (was tenant '{}')", domain, tid);
    return true;
}

/** @brief Lookup tenant id by host/domain value. */
std::optional<std::string> TenantManager::lookupTenantByDomain(std::string_view host) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string key = normaliseDomain(host);
    const auto it = domain_to_tenant_.find(key);
    if (it != domain_to_tenant_.end()) {
        return it->second;
    }
    return std::nullopt;
}

/** @brief Export current tenant metrics in Prometheus exposition format. */
std::string TenantManager::getMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss = {};
    
    oss << "# HELP themis_tenant_count Total number of tenants\n";
    oss << "# TYPE themis_tenant_count gauge\n";
    oss << "themis_tenant_count " << tenants_.size() << "\n\n";
    
    for (const auto& [id, usage] : usage_) {
        // Escape label value safely using a new string
        std::string tid = {};
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
