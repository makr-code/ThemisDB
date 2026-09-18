/**
 * @file in_doubt_recovery_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// CC-5: Shared recovery interface for 2PC coordinators.
// Enforces a common capability across independent coordinator implementations.

#pragma once

#include <cstddef>

namespace themis::transaction {

/** @brief I in doubt recovery coordinator component. */
class IInDoubtRecoveryCoordinator {
public:
    /**
     * @brief TBD: Describe ~IInDoubtRecoveryCoordinator.
     * @return Return value.
     */
    virtual ~IInDoubtRecoveryCoordinator() = default;

    [[nodiscard]] virtual size_t recoverInDoubtTransactions() = 0;
};

} // namespace themis::transaction
