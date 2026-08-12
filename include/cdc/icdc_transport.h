/**
 * @file icdc_transport.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB - CDC Transport Interface
 *
 * Abstract interface for CDC (Change Data Capture) transport backends.
 * Concrete implementations include:
 *   - KafkaCDCProducer (Kafka, include/cdc/kafka_cdc_producer.h)
 *
 * Each transport publishes ChangeEvent records to a specific delivery channel.
 * The interface is intentionally minimal: lifecycle management (start/stop)
 * plus a single-event publish method.  Transport-specific statistics and
 * configuration are exposed by the concrete classes.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"

namespace themis {
namespace cdc {

/**
 * @brief Abstract transport interface for CDC change event delivery.
 *
 * Implemented by transport backends that forward ChangeEvent records to
 * external message systems (e.g. Kafka).
 *
 * Thread-safety: start()/stop() must not be called concurrently with each
 * other.  publish() may be called from a single producer thread while the
 * transport is running.
 */
class ICDCTransport {
public:
    virtual ~ICDCTransport() = default;

    /**
     * @brief Start the transport.
     *
     * Initialise any backend connections and begin background processing.
     * Calling start() on an already-running transport is a no-op that returns
     * true.
     *
     * @return true on success, false if initialisation failed.
     */
    [[nodiscard]] virtual bool start() = 0;

    /**
     * @brief Stop the transport.
     *
     * Flush pending events, terminate background threads, and release backend
     * resources.  Calling stop() on an already-stopped transport is a no-op.
     */
    virtual void stop() = 0;

    /**
     * @brief Publish a single change event to the transport backend.
     *
     * The call is non-blocking where possible; delivery confirmation is
     * handled asynchronously by the concrete implementation.
     *
     * @param event  The change event to publish.
     * @return true if the event was accepted for delivery, false on error.
     */
    [[nodiscard]] virtual bool publish(const Changefeed::ChangeEvent& event) = 0;
};

} // namespace cdc
} // namespace themis
