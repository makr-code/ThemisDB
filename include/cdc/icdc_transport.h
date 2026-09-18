/**
 * @file icdc_transport.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ICDCTransport {
public:
    /**
     * @brief ICDCTransport.
     * @return Return value.
     */
    virtual ~ICDCTransport() = default;

    [[nodiscard]] virtual bool start() = 0;

    /**
     * @brief Stop.
     */
    virtual void stop() = 0;

    [[nodiscard]] virtual bool publish(const Changefeed::ChangeEvent& event) = 0;
};

} // namespace cdc
} // namespace themis
