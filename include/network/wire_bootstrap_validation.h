/*
 * ThemisDB | File: wire_bootstrap_validation.h | Version: 0.0.47
 */

#pragma once

#include <initializer_list>
#include <ostream>

namespace themis::network::wire_bootstrap {

/**
 * @brief Named startup dependency used by Wire bootstrap validation.
 */
struct RequiredBackend {
    const char* name;
    bool available;
};

/**
 * @brief Validate fail-closed Wire bootstrap dependencies.
 *
 * If any required backend is unavailable, a consistent startup-refused log
 * line is emitted and false is returned.
 *
 * @param server_tag  Prefix used in log output (for example "WireProtocol").
 * @param required    Required backend list.
 * @param err         Output stream for refusal diagnostics.
 * @return true when all required backends are available, otherwise false.
 */
inline bool validateRequiredBackends(
    const char* server_tag,
    std::initializer_list<RequiredBackend> required,
    std::ostream& err) {
    bool all_available = true;
    for (const auto& backend : required) {
        all_available = all_available && backend.available;
    }

    if (all_available) {
        return true;
    }

    err << "[" << server_tag << "] Startup refused: missing required runtime backends (";
    bool first = true;
    for (const auto& backend : required) {
        if (!first) {
            err << ", ";
        }
        err << backend.name << "=" << backend.available;
        first = false;
    }
    err << ").\n";
    return false;
}

} // namespace themis::network::wire_bootstrap
