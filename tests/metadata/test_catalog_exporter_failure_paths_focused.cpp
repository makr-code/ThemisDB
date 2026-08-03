// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_catalog_exporter_failure_paths_focused.cpp
 * @brief Phase B3: Failure-path tests for CatalogExporter.
 * @note Test IDs: MCH-EX01..MCH-EX04
 *
 *   MCH-EX01  CatalogExporter with HTTP 500 returns success=false with non-empty error
 *   MCH-EX02  CatalogExporter with HTTP 200 returns success=true
 *   MCH-EX03  CatalogExporter::publishSchema() with empty table list returns entity_count==0
 *   MCH-EX04  DataHub type with HTTP 200 returns success=true
 *
 * All tests inject a fake HTTP function via setHttpPostForTesting() — no real
 * network I/O, no libcurl calls.
 * Canonical PRNG seed: kExportSeed = 42.
 *
 * @see include/metadata/catalog_exporter.h
 * @see src/metadata/ROADMAP.md — Phase B export failure-path items
 */

#include <gtest/gtest.h>

#include "metadata/catalog_exporter.h"

#include <string>
#include <vector>

using namespace themis;

namespace {

[[maybe_unused]] static constexpr uint64_t kExportSeed = 42;

/// Build a minimal Atlas config pointing at a localhost endpoint that will
/// never be reached (real HTTP is replaced by the injected function).
CatalogExporter::Config makeAtlasConfig() {
    CatalogExporter::Config cfg;
    cfg.type     = CatalogExporter::CatalogType::APACHE_ATLAS;
    cfg.endpoint = "http://127.0.0.1:21000";
    cfg.username = "admin";
    cfg.password = "admin";
    return cfg;
}

/// Build a minimal DataHub config.
CatalogExporter::Config makeDataHubConfig() {
    CatalogExporter::Config cfg;
    cfg.type     = CatalogExporter::CatalogType::DATAHUB;
    cfg.endpoint = "http://127.0.0.1:8080";
    cfg.token    = "test-token";
    return cfg;
}

/// Build a trivial non-empty table list.
std::vector<SchemaManager::TableSchema> makeOneTableList() {
    SchemaManager::TableSchema schema;
    schema.name = "test_table";
    schema.type = "collection";
    SchemaManager::PropertyInfo prop;
    prop.name = "id";
    prop.type = "string";
    schema.properties.push_back(prop);
    return {schema};
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// MCH-EX01: HTTP 500 → success=false with non-empty error
// ---------------------------------------------------------------------------
TEST(CatalogExporterFailurePathsTest, MCHEX01_Http500ReturnsFailure) {
    CatalogExporter exporter(makeAtlasConfig());
    exporter.setHttpPostForTesting(
        [](const std::string& /*url*/,
           const std::string& /*body*/,
           const std::string& /*auth*/) -> int { return 500; }
    );

    const auto result = exporter.publishSchema(makeOneTableList());

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty())
        << "error field must be non-empty on HTTP 500";
}

// ---------------------------------------------------------------------------
// MCH-EX02: HTTP 200 → success=true
// ---------------------------------------------------------------------------
TEST(CatalogExporterFailurePathsTest, MCHEX02_Http200ReturnsSuccess) {
    CatalogExporter exporter(makeAtlasConfig());
    exporter.setHttpPostForTesting(
        [](const std::string& /*url*/,
           const std::string& /*body*/,
           const std::string& /*auth*/) -> int { return 200; }
    );

    const auto result = exporter.publishSchema(makeOneTableList());

    EXPECT_TRUE(result.success);
}

// ---------------------------------------------------------------------------
// MCH-EX03: Empty table list → entity_count == 0, success=true
// ---------------------------------------------------------------------------
TEST(CatalogExporterFailurePathsTest, MCHEX03_EmptyTableListReturnsZeroEntities) {
    CatalogExporter exporter(makeAtlasConfig());
    exporter.setHttpPostForTesting(
        [](const std::string& /*url*/,
           const std::string& /*body*/,
           const std::string& /*auth*/) -> int { return 200; }
    );

    const auto result = exporter.publishSchema({});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entity_count, 0);
}

// ---------------------------------------------------------------------------
// MCH-EX04: DataHub type with HTTP 200 → success=true
// ---------------------------------------------------------------------------
TEST(CatalogExporterFailurePathsTest, MCHEX04_DataHubHttp200ReturnsSuccess) {
    CatalogExporter exporter(makeDataHubConfig());
    exporter.setHttpPostForTesting(
        [](const std::string& /*url*/,
           const std::string& /*body*/,
           const std::string& /*auth*/) -> int { return 200; }
    );

    const auto result = exporter.publishSchema(makeOneTableList());

    EXPECT_TRUE(result.success);
}
