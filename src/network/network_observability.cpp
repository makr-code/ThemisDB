/**
 * @file network_observability.cpp
 * @brief Network observability implementation stubs for ThemisDB.
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 *
 * The core NetworkObservabilityRegistry and NetworkSpan logic is fully
 * header-inline (network_observability.h). This translation unit provides
 * the explicit out-of-line destructor for INetworkObservabilitySink to ensure
 * the vtable is emitted exactly once (avoids -Wweak-vtables / linker bloat).
 */

#include "network/network_observability.h"

namespace themis {
namespace network {

// Out-of-line virtual destructor — ensures vtable is placed in this TU only.
INetworkObservabilitySink::~INetworkObservabilitySink() = default;

} // namespace network
} // namespace themis
