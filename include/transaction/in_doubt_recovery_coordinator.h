// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// CC-5: Shared recovery interface for 2PC coordinators.
// Enforces a common capability across independent coordinator implementations.

#pragma once

#include <cstddef>

namespace themis::transaction {

class IInDoubtRecoveryCoordinator {
public:
    virtual ~IInDoubtRecoveryCoordinator() = default;

    [[nodiscard]] virtual size_t recoverInDoubtTransactions() = 0;
};

} // namespace themis::transaction
