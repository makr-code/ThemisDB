/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_factory.cpp                                ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:57:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     207                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • be096cd97  2026-02-28  audit: update stale banner metadata in database_adapter.h... ║
    • 176df4359  2026-02-28  feat(chimera): implement AdapterCapabilityMatrix for cros... ║
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
#include <algorithm>
#include <sstream>

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
        Capability::SHARDING
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
        default:                               return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// AdapterConfig — connection-string parsing
// ---------------------------------------------------------------------------

namespace {

/// Known URI schemes accepted by the chimera adapter suite.
const std::vector<std::string> kKnownSchemes = {
    "themisdb",
    "postgresql", "postgres",
    "mongodb", "mongodb+srv",
    "bolt", "neo4j", "neo4j+s",
    "http", "https"
};

/// Well-known integer options and their valid [min, max] inclusive ranges.
struct IntOptionRange {
    const char* name;
    int64_t min_val;
    int64_t max_val;
};

const std::vector<IntOptionRange> kIntOptionRanges = {
    { "pool_size",        1,      10000  },
    { "timeout_ms",       1,      3600000 },
    { "connect_timeout",  1,      3600000 },
    { "max_retries",      0,      100    },
    { "port",             1,      65535  }
};

/// Well-known boolean option names.
const std::vector<std::string> kBoolOptions = {
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
    if (connection_string.find("://") == std::string::npos) {
        errors.emplace_back(
            "connection_string is missing a URI scheme (expected '<scheme>://<host>')");
    }

    // --- Parse and validate components ---
    const ParsedConnectionString parsed = parse_connection_string();

    // Scheme must be recognised
    if (!parsed.scheme.empty()) {
        bool known = false;
        for (const auto& s : kKnownSchemes) {
            if (parsed.scheme == s) { known = true; break; }
        }
        if (!known) {
            std::ostringstream oss;
            oss << "unknown URI scheme '" << parsed.scheme
                << "'; recognised schemes: ";
            for (size_t i = 0; i < kKnownSchemes.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << kKnownSchemes[i];
            }
            errors.push_back(oss.str());
        }
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

    // --- Options: type and range validation ---
    for (const auto& kv : options) {
        const std::string& key   = kv.first;
        const Scalar&      value = kv.second;

        // Check integer options
        for (const auto& spec : kIntOptionRanges) {
            if (key == spec.name) {
                if (!std::holds_alternative<int64_t>(value)) {
                    std::ostringstream oss;
                    oss << "option '" << key << "' must be of type int64_t";
                    errors.push_back(oss.str());
                } else {
                    int64_t v = std::get<int64_t>(value);
                    if (v < spec.min_val || v > spec.max_val) {
                        std::ostringstream oss;
                        oss << "option '" << key << "' value " << v
                            << " is out of valid range ["
                            << spec.min_val << ", " << spec.max_val << "]";
                        errors.push_back(oss.str());
                    }
                }
                break; // matched — no need to check further specs
            }
        }

        // Check boolean options
        for (const auto& bkey : kBoolOptions) {
            if (key == bkey) {
                if (!std::holds_alternative<bool>(value)) {
                    std::ostringstream oss;
                    oss << "option '" << key << "' must be of type bool";
                    errors.push_back(oss.str());
                }
                break;
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
