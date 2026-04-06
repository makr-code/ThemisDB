/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_factory.cpp                                ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:15:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     520                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 04a46f63a9  2026-03-12  fix(chimera): address PR review comments on multi-databas... ║
    • 3bd2167e65  2026-03-12  feat(chimera): implement multi-database adapter registrat... ║
    • 66c21bb7c5  2026-03-12  fix(chimera): address review feedback on AdapterConfig va... ║
    • a3a9d7e09c  2026-03-12  feat(chimera): implement AdapterConfig validation (v1.2.0) ║
    • 0701ac8f4d  2026-03-12  feat(chimera): implement async/promise-based API (IAsyncD... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file adapter_factory.cpp
 * @brief Implementation of the AdapterFactory for CHIMERA Suite
 * 
 * @copyright MIT License
 */

#include "chimera/database_adapter.hpp"
#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace chimera {

// Thread-safe singleton registry
std::map<std::string, AdapterFactory::AdapterCreator>& AdapterFactory::get_registry() {
    static std::map<std::string, AdapterCreator> registry;
    return registry;
}

std::unique_ptr<IDatabaseAdapter> AdapterFactory::create(const std::string& system_name) {
    auto& registry = get_registry();
    auto it = registry.find(system_name);
    if (it != registry.end()) {
        return it->second();
    }
    return nullptr;
}

bool AdapterFactory::register_adapter(const std::string& system_name, AdapterCreator creator) {
    static std::mutex registry_mutex;
    std::lock_guard<std::mutex> lock(registry_mutex);
    
    auto& registry = get_registry();
    auto result = registry.insert({system_name, creator});
    return result.second; // true if inserted, false if already exists
}

std::vector<std::string> AdapterFactory::get_supported_systems() {
    auto& registry = get_registry();
    std::vector<std::string> systems;
    systems.reserve(registry.size());
    for (const auto& pair : registry) {
        systems.push_back(pair.first);
    }
    // Sort alphabetically for vendor-neutrality
    std::sort(systems.begin(), systems.end());
    return systems;
}

bool AdapterFactory::is_supported(const std::string& system_name) {
    auto& registry = get_registry();
    return registry.find(system_name) != registry.end();
}

// Thread-safe singleton capability hints registry
std::map<std::string, std::vector<Capability>>& AdapterFactory::get_capability_hints() {
    static std::map<std::string, std::vector<Capability>> hints;
    return hints;
}

// Shared mutex protecting the capability hints map.
// Used with exclusive ownership during writes (register_adapter overload) and
// shared ownership during reads (create_with_capabilities).
static std::shared_mutex& hints_shared_mutex() {
    static std::shared_mutex mtx;
    return mtx;
}

bool AdapterFactory::register_adapter(const std::string& system_name,
                                       AdapterCreator creator,
                                       const std::vector<Capability>& static_capabilities) {
    // Store capability hints thread-safely under exclusive ownership before
    // delegating to the base overload.  Hints are stored unconditionally so
    // that re-registration can update capabilities even when the creator slot
    // is already taken.
    {
        std::unique_lock<std::shared_mutex> lock(hints_shared_mutex());
        get_capability_hints()[system_name] = static_capabilities;
    }
    return register_adapter(system_name, std::move(creator));
}

std::unique_ptr<IDatabaseAdapter> AdapterFactory::create_with_fallback(
    const std::vector<std::string>& candidates
) {
    for (const auto& name : candidates) {
        if (!is_supported(name)) {
            continue;
        }
        auto adapter = create(name);
        if (adapter) {
            return adapter;
        }
    }
    return nullptr;
}

std::unique_ptr<IDatabaseAdapter> AdapterFactory::create_with_capabilities(
    const std::vector<std::string>& candidates,
    const std::vector<Capability>& required_capabilities
) {
    // Acquire a shared (read) lock for the duration of the lookup so that
    // concurrent registrations cannot race with hint reads.
    std::shared_lock<std::shared_mutex> hints_lock(hints_shared_mutex());
    auto& hints = get_capability_hints();

    for (const auto& name : candidates) {
        if (!is_supported(name)) {
            continue;
        }

        // Fast path: if static capability hints were registered, use them to
        // negotiate without instantiating the adapter, avoiding potentially
        // expensive construction for non-qualifying candidates.
        auto hint_it = hints.find(name);
        if (hint_it != hints.end()) {
            bool meets_requirements = true;
            for (const auto& cap : required_capabilities) {
                if (std::find(hint_it->second.begin(), hint_it->second.end(), cap)
                        == hint_it->second.end()) {
                    meets_requirements = false;
                    break;
                }
            }
            if (!meets_requirements) {
                continue;
            }
        }

        // Create the adapter — either hints confirmed it qualifies, or no
        // hints were registered and we must probe via the live instance.
        auto adapter = create(name);
        if (!adapter) {
            continue;
        }

        // If no static hints, fall back to runtime capability probing.
        if (hint_it == hints.end()) {
            bool meets_requirements = true;
            for (const auto& cap : required_capabilities) {
                if (!adapter->has_capability(cap)) {
                    meets_requirements = false;
                    break;
                }
            }
            if (!meets_requirements) {
                continue;
            }
        }

        return adapter;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// AdapterCapabilityMatrix
// ---------------------------------------------------------------------------

void AdapterCapabilityMatrix::add_entry(
    const std::string& system_name,
    const std::vector<Capability>& capabilities
) {
    CapabilityRow& row = matrix_[system_name];
    // Initialise every known capability to false, then mark supported ones.
    for (const auto& cap : all_capabilities()) {
        row[cap] = false;
    }
    for (const auto& cap : capabilities) {
        row[cap] = true;
    }
}

void AdapterCapabilityMatrix::add_adapter(
    const std::string& system_name,
    const ISystemInfoAdapter& adapter
) {
    add_entry(system_name, adapter.get_capabilities());
}

AdapterCapabilityMatrix AdapterCapabilityMatrix::build_from_factory() {
    AdapterCapabilityMatrix matrix;
    for (const auto& name : AdapterFactory::get_supported_systems()) {
        auto adapter = AdapterFactory::create(name);
        if (adapter) {
            matrix.add_adapter(name, *adapter);
        }
    }
    return matrix;
}

bool AdapterCapabilityMatrix::supports(
    const std::string& system_name,
    Capability cap
) const {
    auto row_it = matrix_.find(system_name);
    if (row_it == matrix_.end()) {
        return false;
    }
    auto cap_it = row_it->second.find(cap);
    return cap_it != row_it->second.end() && cap_it->second;
}

std::vector<std::string> AdapterCapabilityMatrix::adapters_supporting(
    Capability cap
) const {
    std::vector<std::string> result;
    for (const auto& kv : matrix_) {
        auto cap_it = kv.second.find(cap);
        if (cap_it != kv.second.end() && cap_it->second) {
            result.push_back(kv.first);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<Capability> AdapterCapabilityMatrix::capabilities_of(
    const std::string& system_name
) const {
    std::vector<Capability> result;
    auto row_it = matrix_.find(system_name);
    if (row_it == matrix_.end()) {
        return result;
    }
    for (const auto& kv : row_it->second) {
        if (kv.second) {
            result.push_back(kv.first);
        }
    }
    return result;
}

std::vector<std::string> AdapterCapabilityMatrix::system_names() const {
    std::vector<std::string> names;
    names.reserve(matrix_.size());
    for (const auto& kv : matrix_) {
        names.push_back(kv.first);
    }
    // matrix_ is a std::map so keys are already sorted; preserve order.
    return names;
}

std::vector<Capability> AdapterCapabilityMatrix::all_capabilities() {
    return {
        Capability::RELATIONAL_QUERIES,
        Capability::VECTOR_SEARCH,
        Capability::GRAPH_TRAVERSAL,
        Capability::DOCUMENT_STORE,
        Capability::FULL_TEXT_SEARCH,
        Capability::TRANSACTIONS,
        Capability::DISTRIBUTED_QUERIES,
        Capability::GEOSPATIAL_QUERIES,
        Capability::TIME_SERIES,
        Capability::STREAM_PROCESSING,
        Capability::BATCH_OPERATIONS,
        Capability::SECONDARY_INDEXES,
        Capability::MATERIALIZED_VIEWS,
        Capability::REPLICATION,
        Capability::SHARDING,
        Capability::ASYNC_OPERATIONS
    };
}

std::string AdapterCapabilityMatrix::capability_to_string(Capability cap) {
    switch (cap) {
        case Capability::RELATIONAL_QUERIES:   return "RELATIONAL_QUERIES";
        case Capability::VECTOR_SEARCH:        return "VECTOR_SEARCH";
        case Capability::GRAPH_TRAVERSAL:      return "GRAPH_TRAVERSAL";
        case Capability::DOCUMENT_STORE:       return "DOCUMENT_STORE";
        case Capability::FULL_TEXT_SEARCH:     return "FULL_TEXT_SEARCH";
        case Capability::TRANSACTIONS:         return "TRANSACTIONS";
        case Capability::DISTRIBUTED_QUERIES:  return "DISTRIBUTED_QUERIES";
        case Capability::GEOSPATIAL_QUERIES:   return "GEOSPATIAL_QUERIES";
        case Capability::TIME_SERIES:          return "TIME_SERIES";
        case Capability::STREAM_PROCESSING:    return "STREAM_PROCESSING";
        case Capability::BATCH_OPERATIONS:     return "BATCH_OPERATIONS";
        case Capability::SECONDARY_INDEXES:    return "SECONDARY_INDEXES";
        case Capability::MATERIALIZED_VIEWS:   return "MATERIALIZED_VIEWS";
        case Capability::REPLICATION:          return "REPLICATION";
        case Capability::SHARDING:             return "SHARDING";
        case Capability::ASYNC_OPERATIONS:     return "ASYNC_OPERATIONS";
        default:                               return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// AdapterConfig — connection-string parsing
// ---------------------------------------------------------------------------

namespace {

/// Known URI schemes accepted by the chimera adapter suite.
const std::unordered_set<std::string> kKnownSchemes = {
    "themisdb",
    "postgresql", "postgres",
    "mongodb", "mongodb+srv",
    "bolt", "neo4j", "neo4j+s",
    "http", "https"
};

/// Printable list of known schemes for error messages (sorted for stability).
const std::vector<std::string> kKnownSchemesList = {
    "bolt", "http", "https", "mongodb", "mongodb+srv",
    "neo4j", "neo4j+s", "postgres", "postgresql", "themisdb"
};

/// Well-known integer options and their valid [min, max] inclusive ranges.
struct IntOptionRange {
    int64_t min_val;
    int64_t max_val;
};

const std::unordered_map<std::string, IntOptionRange> kIntOptionRanges = {
    { "pool_size",        { 1,      10000   } },
    { "timeout_ms",       { 1,      3600000 } },
    { "connect_timeout",  { 1,      3600000 } },
    { "max_retries",      { 0,      100     } }
};

/// Well-known boolean option names.
const std::unordered_set<std::string> kBoolOptions = {
    "use_tls", "tls_enabled", "ssl", "verify_cert", "read_only"
};

} // anonymous namespace

ParsedConnectionString AdapterConfig::parse_connection_string() const {
    ParsedConnectionString parsed;
    const std::string& cs = connection_string;

    // Extract scheme (everything before "://")
    const std::string sep = "://";
    auto scheme_end = cs.find(sep);
    if (scheme_end == std::string::npos) {
        return parsed; // malformed — caller checks this
    }
    parsed.scheme = cs.substr(0, scheme_end);

    // Remainder after "://"
    std::string rest = cs.substr(scheme_end + sep.size());

    // Strip query/fragment
    auto qmark = rest.find('?');
    if (qmark != std::string::npos) {
        rest = rest.substr(0, qmark);
    }
    auto hash = rest.find('#');
    if (hash != std::string::npos) {
        rest = rest.substr(0, hash);
    }

    // Separate userinfo from host/path ("user:pass@host:port/db")
    auto at_pos = rest.find('@');
    if (at_pos != std::string::npos) {
        const std::string userinfo = rest.substr(0, at_pos);
        rest = rest.substr(at_pos + 1);

        auto colon_pos = userinfo.find(':');
        if (colon_pos != std::string::npos) {
            parsed.username = userinfo.substr(0, colon_pos);
            // password intentionally not stored
        } else {
            parsed.username = userinfo;
        }
    }

    // Split host:port from /database
    auto slash_pos = rest.find('/');
    std::string host_port;
    if (slash_pos != std::string::npos) {
        host_port         = rest.substr(0, slash_pos);
        parsed.database   = rest.substr(slash_pos + 1);
    } else {
        host_port = rest;
    }

    // Split host from port
    auto colon_pos = host_port.rfind(':');
    if (colon_pos != std::string::npos) {
        parsed.host = host_port.substr(0, colon_pos);
        parsed.port = host_port.substr(colon_pos + 1);
    } else {
        parsed.host = host_port;
    }

    return parsed;
}

std::vector<std::string> AdapterConfig::get_validation_errors() const {
    std::vector<std::string> errors;

    // --- connection_string: must not be empty ---
    if (connection_string.empty()) {
        errors.emplace_back("connection_string must not be empty");
        // Cannot parse further without a connection string.
        return errors;
    }

    // --- connection_string: must contain "://" ---
    const bool has_scheme_separator =
        connection_string.find("://") != std::string::npos;
    if (!has_scheme_separator) {
        errors.emplace_back(
            "connection_string is missing a URI scheme (expected '<scheme>://<host>')");
        // Component validation (scheme/host/port) is meaningless without a separator.
        // Still proceed to option validation below.
    } else {
        // --- Parse and validate components ---
        const ParsedConnectionString parsed = parse_connection_string();

        // Scheme must be non-empty and recognised
        if (parsed.scheme.empty()) {
            errors.emplace_back(
                "connection_string must specify a non-empty URI scheme before '://'");
        } else if (kKnownSchemes.find(parsed.scheme) == kKnownSchemes.end()) {
            std::ostringstream oss;
            oss << "unknown URI scheme '" << parsed.scheme
                << "'; recognised schemes: ";
            for (size_t i = 0; i < kKnownSchemesList.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << kKnownSchemesList[i];
            }
            errors.push_back(oss.str());
        }

        // Host must be present
        if (parsed.host.empty()) {
            errors.emplace_back("connection_string must specify a non-empty host");
        }

        // If a port string is present it must be a valid port number
        if (!parsed.port.empty()) {
            try {
                size_t idx = 0;
                long port_val = std::stol(parsed.port, &idx);
                if (idx != parsed.port.size() || port_val < 1 || port_val > 65535) {
                    errors.emplace_back(
                        "connection_string port '" + parsed.port +
                        "' is out of valid range [1, 65535]");
                }
            } catch (...) {
                errors.emplace_back(
                    "connection_string port '" + parsed.port + "' is not a valid integer");
            }
        }
    }

    // --- Options: type and range validation ---
    for (const auto& kv : options) {
        const std::string& key   = kv.first;
        const Scalar&      value = kv.second;

        // Check integer options (O(1) lookup)
        auto int_it = kIntOptionRanges.find(key);
        if (int_it != kIntOptionRanges.end()) {
            if (!std::holds_alternative<int64_t>(value)) {
                errors.push_back("option '" + key + "' must be of type int64_t");
            } else {
                int64_t v = std::get<int64_t>(value);
                if (v < int_it->second.min_val || v > int_it->second.max_val) {
                    std::ostringstream oss;
                    oss << "option '" << key << "' value " << v
                        << " is out of valid range ["
                        << int_it->second.min_val << ", "
                        << int_it->second.max_val << "]";
                    errors.push_back(oss.str());
                }
            }
        }

        // Check boolean options (O(1) lookup)
        if (kBoolOptions.count(key)) {
            if (!std::holds_alternative<bool>(value)) {
                errors.push_back("option '" + key + "' must be of type bool");
            }
        }
    }

    return errors;
}

Result<bool> AdapterConfig::validate() const {
    const auto errors = get_validation_errors();
    if (errors.empty()) {
        return Result<bool>::ok(true);
    }
    return Result<bool>::err(ErrorCode::INVALID_ARGUMENT, errors.front());
}
} // namespace chimera
