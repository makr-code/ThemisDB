// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_schema_version_manager_lock_contract_focused.cpp
 * @brief Phase A2: External-lock contract tests for SchemaVersionManager.
 * @note Test IDs: MCH-L01..MCH-L04
 *
 * SchemaVersionManager is explicitly NOT thread-safe (documented contract).
 * These tests verify the sequential API contract and documented error paths:
 *
 *   MCH-L01  VersionResult<T>::failure() carries correct error code and message
 *   MCH-L02  VersionResult<T>::ok flag is false when constructed via failure()
 *   MCH-L03  VersionErrorCode values are distinct (TABLE_NOT_FOUND != VERSION_NOT_FOUND)
 *   MCH-L04  VersionResult<uint64_t>::success() sets ok=true and preserves value
 *
 * All tests are self-contained header-only; no RocksDB, no network I/O.
 * Canonical PRNG seed: kLockContractSeed = 42 (declared; not required here).
 *
 * @see include/metadata/schema_version_manager.h
 * @see src/metadata/ROADMAP.md — Phase A external-lock contract
 */

#include <gtest/gtest.h>

#include "metadata/schema_version_manager.h"

#include <string>

using namespace themis;

namespace {

[[maybe_unused]] static constexpr uint64_t kLockContractSeed = 42;

} // anonymous namespace

// ---------------------------------------------------------------------------
// MCH-L01: VersionResult::failure() carries correct error code and message
// ---------------------------------------------------------------------------
TEST(SchemaVersionManagerLockContractTest, MCHL01_FailureResultCarriesErrorCode) {
    const auto result = VersionResult<uint64_t>::failure(
        VersionErrorCode::TABLE_NOT_FOUND,
        "table 'users' not found"
    );

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, VersionErrorCode::TABLE_NOT_FOUND);
    EXPECT_EQ(result.error_message, "table 'users' not found");
}

// ---------------------------------------------------------------------------
// MCH-L02: VersionResult::ok is false for failure() result
// ---------------------------------------------------------------------------
TEST(SchemaVersionManagerLockContractTest, MCHL02_FailureResultOkIsFalse) {
    const auto result = VersionResult<bool>::failure(
        VersionErrorCode::VERSION_NOT_FOUND,
        "version 99 not found"
    );

    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error, VersionErrorCode::OK);
}

// ---------------------------------------------------------------------------
// MCH-L03: VersionErrorCode values are distinct
// ---------------------------------------------------------------------------
TEST(SchemaVersionManagerLockContractTest, MCHL03_ErrorCodesAreDistinct) {
    EXPECT_NE(static_cast<int>(VersionErrorCode::OK),
              static_cast<int>(VersionErrorCode::TABLE_NOT_FOUND));
    EXPECT_NE(static_cast<int>(VersionErrorCode::TABLE_NOT_FOUND),
              static_cast<int>(VersionErrorCode::VERSION_NOT_FOUND));
    EXPECT_NE(static_cast<int>(VersionErrorCode::OK),
              static_cast<int>(VersionErrorCode::VERSION_NOT_FOUND));
}

// ---------------------------------------------------------------------------
// MCH-L04: VersionResult::success() has ok=true and correct value
// ---------------------------------------------------------------------------
TEST(SchemaVersionManagerLockContractTest, MCHL04_SuccessResultOkIsTrue) {
    const auto result = VersionResult<uint64_t>::success(42u);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.value, 42u);
    EXPECT_EQ(result.error, VersionErrorCode::OK);
    EXPECT_TRUE(result.error_message.empty());
}
