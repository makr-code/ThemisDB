// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

// Tests: CatalogExporter – Apache Atlas and DataHub integration
// All tests use an injected HTTP function so no real network access is required.

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "metadata/catalog_exporter.h"
#include "metadata/schema_manager.h"

using namespace themis;
using json = nlohmann::json;

// ============================================================================
// Helpers
// ============================================================================

static SchemaManager::TableSchema makeTable(const std::string& name,
                                             const std::string& type = "relational",
                                             size_t row_count = 100) {
    SchemaManager::TableSchema t;
    t.name               = name;
    t.type               = type;
    t.estimated_row_count = row_count;

    SchemaManager::PropertyInfo id_prop;
    id_prop.name     = "id";
    id_prop.type     = "integer";
    id_prop.nullable = false;
    t.properties.push_back(id_prop);

    SchemaManager::PropertyInfo name_prop;
    name_prop.name     = "name";
    name_prop.type     = "string";
    name_prop.nullable = true;
    t.properties.push_back(name_prop);

    return t;
}

/// Build a minimal CatalogExporter::Config for Apache Atlas.
static CatalogExporter::Config atlasConfig(const std::string& endpoint = "http://atlas:21000") {
    CatalogExporter::Config cfg;
    cfg.type          = CatalogExporter::CatalogType::APACHE_ATLAS;
    cfg.endpoint      = endpoint;
    cfg.username      = "admin";
    cfg.password      = "admin";
    cfg.database_name = "TestDB";
    cfg.timeout_ms    = 1000;
    return cfg;
}

/// Build a minimal CatalogExporter::Config for DataHub.
static CatalogExporter::Config datahubConfig(const std::string& endpoint = "http://datahub-gms:8080") {
    CatalogExporter::Config cfg;
    cfg.type          = CatalogExporter::CatalogType::DATAHUB;
    cfg.endpoint      = endpoint;
    cfg.token         = "test-token";
    cfg.database_name = "TestDB";
    cfg.timeout_ms    = 1000;
    return cfg;
}

// ============================================================================
// Apache Atlas tests
// ============================================================================

class CatalogExporterAtlasTest : public ::testing::Test {
protected:
    struct Capture {
        std::string url;
        std::string body;
        std::string auth_header;
    };

    /// Install a test-double that records calls and returns the configured response.
    void setHttpMock(int status_code, const std::string& response_body = "{}") {
        exporter_->setHttpPostForTesting(
            [this, status_code, response_body](
                const std::string& url,
                const std::string& body,
                const std::string& auth,
                std::string&       resp) -> int {
                captured_.push_back({url, body, auth});
                resp = response_body;
                return status_code;
            });
    }

    void SetUp() override {
        exporter_ = std::make_unique<CatalogExporter>(atlasConfig());
    }

    std::unique_ptr<CatalogExporter> exporter_;
    std::vector<Capture>             captured_;
};

TEST_F(CatalogExporterAtlasTest, EmptyTableListSucceedsWithoutHttp) {
    // No HTTP call should be made for an empty list.
    bool http_called = false;
    exporter_->setHttpPostForTesting(
        [&http_called](const std::string&, const std::string&,
                        const std::string&, std::string&) -> int {
            http_called = true;
            return 200;
        });

    auto result = exporter_->publishSchema({});
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entities_published, 0);
    EXPECT_FALSE(http_called) << "Should not call HTTP for empty schema";
}

TEST_F(CatalogExporterAtlasTest, PublishSingleTableCallsCorrectUrl) {
    setHttpMock(200, R"({"mutatedEntities":{"CREATE":[{"guid":"1"},{"guid":"2"},{"guid":"3"}]}})");

    auto result = exporter_->publishTable(makeTable("users"));

    ASSERT_FALSE(captured_.empty());
    EXPECT_EQ(captured_[0].url, "http://atlas:21000/api/atlas/v2/entity/bulk");
    EXPECT_TRUE(result.success);
}

TEST_F(CatalogExporterAtlasTest, PublishSetsBasicAuthHeader) {
    setHttpMock(200);

    exporter_->publishTable(makeTable("orders"));

    ASSERT_FALSE(captured_.empty());
    // Authorization header must start with "Authorization: Basic "
    EXPECT_EQ(captured_[0].auth_header.substr(0, 21), "Authorization: Basic ");
}

TEST_F(CatalogExporterAtlasTest, PayloadContainsDbAndTableEntities) {
    setHttpMock(200);

    exporter_->publishTable(makeTable("products"));

    ASSERT_FALSE(captured_.empty());
    auto payload = json::parse(captured_[0].body);
    ASSERT_TRUE(payload.contains("entities"));
    ASSERT_TRUE(payload["entities"].is_array());

    // Expect at least: rdbms_db + rdbms_table + 2 rdbms_column entities
    EXPECT_GE(payload["entities"].size(), 4u);

    // Check entity types
    bool found_db    = false;
    bool found_table = false;
    bool found_col   = false;
    for (const auto& ent : payload["entities"]) {
        const std::string type = ent["typeName"].get<std::string>();
        if (type == "rdbms_db") {
          found_db    = true;
        }
        if (type == "rdbms_table") {
          found_table = true;
        }
        if (type == "rdbms_column") {
          found_col   = true;
        }
    }
    EXPECT_TRUE(found_db)    << "Should include rdbms_db entity";
    EXPECT_TRUE(found_table) << "Should include rdbms_table entity";
    EXPECT_TRUE(found_col)   << "Should include rdbms_column entities";
}

TEST_F(CatalogExporterAtlasTest, TableQualifiedNameContainsDatabaseName) {
    setHttpMock(200);

    exporter_->publishTable(makeTable("invoices"));

    ASSERT_FALSE(captured_.empty());
    const std::string body = captured_[0].body;
    EXPECT_NE(body.find("TestDB.invoices"), std::string::npos)
        << "qualifiedName should be <db>.<table>";
}

TEST_F(CatalogExporterAtlasTest, HttpErrorReturnsFailure) {
    setHttpMock(500, "Internal Server Error");

    auto result = exporter_->publishTable(makeTable("logs"));

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error.find("500"), std::string::npos);
}

TEST_F(CatalogExporterAtlasTest, Http201AlsoCountsAsSuccess) {
    setHttpMock(201);

    auto result = exporter_->publishTable(makeTable("events"));

    EXPECT_TRUE(result.success);
}

TEST_F(CatalogExporterAtlasTest, PublishMultipleTablesInSingleBulkCall) {
    setHttpMock(200);

    std::vector<SchemaManager::TableSchema> tables = {
        makeTable("t1"), makeTable("t2"), makeTable("t3")
    };
    auto result = exporter_->publishSchema(tables);

    // Should use exactly one HTTP call for the bulk endpoint
    EXPECT_EQ(captured_.size(), 1u);
    EXPECT_TRUE(result.success);
}

TEST_F(CatalogExporterAtlasTest, MutatedEntitiesCountIsReturned) {
    const std::string resp = R"({"mutatedEntities":{"CREATE":[{"g":"1"},{"g":"2"}],"UPDATE":[{"g":"3"}]}})";
    setHttpMock(200, resp);

    auto result = exporter_->publishTable(makeTable("accounts"));

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entities_published, 3);
}

TEST_F(CatalogExporterAtlasTest, PublishSingleTableViaPublishTable) {
    setHttpMock(200);
    auto tbl = makeTable("contracts");
    auto result = exporter_->publishTable(tbl);

    EXPECT_TRUE(result.success);
    ASSERT_FALSE(captured_.empty());
    const auto& body = captured_[0].body;
    EXPECT_NE(body.find("contracts"), std::string::npos);
}

// ============================================================================
// DataHub tests
// ============================================================================

class CatalogExporterDataHubTest : public ::testing::Test {
protected:
    struct Capture {
        std::string url;
        std::string body;
        std::string auth_header;
    };

    void setHttpMock(int status_code, const std::string& response_body = "") {
        exporter_->setHttpPostForTesting(
            [this, status_code, response_body](
                const std::string& url,
                const std::string& body,
                const std::string& auth,
                std::string&       resp) -> int {
                captured_.push_back({url, body, auth});
                resp = response_body;
                return status_code;
            });
    }

    void SetUp() override {
        exporter_ = std::make_unique<CatalogExporter>(datahubConfig());
    }

    std::unique_ptr<CatalogExporter> exporter_;
    std::vector<Capture>             captured_;
};

TEST_F(CatalogExporterDataHubTest, EmptySchemaSucceedsWithoutHttp) {
    bool called = false;
    exporter_->setHttpPostForTesting(
        [&called](const std::string&, const std::string&,
                   const std::string&, std::string&) -> int {
            called = true;
            return 200;
        });

    auto result = exporter_->publishSchema({});
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(called);
}

TEST_F(CatalogExporterDataHubTest, PublishCallsIngestProposalEndpoint) {
    setHttpMock(200);

    exporter_->publishTable(makeTable("customers"));

    ASSERT_FALSE(captured_.empty());
    for (const auto& cap : captured_) {
        EXPECT_EQ(cap.url, "http://datahub-gms:8080/aspects?action=ingestProposal");
    }
}

TEST_F(CatalogExporterDataHubTest, BearerTokenInAuthHeader) {
    setHttpMock(200);

    exporter_->publishTable(makeTable("sessions"));

    ASSERT_FALSE(captured_.empty());
    EXPECT_EQ(captured_[0].auth_header, "Authorization: Bearer test-token");
}

TEST_F(CatalogExporterDataHubTest, TwoProposalsPerTableDatasetPropertiesAndSchema) {
    setHttpMock(200);

    exporter_->publishTable(makeTable("orders"));

    // Expect 2 HTTP calls: datasetProperties + schemaMetadata
    EXPECT_EQ(captured_.size(), 2u);
}

TEST_F(CatalogExporterDataHubTest, ProposalContainsDatasetUrn) {
    setHttpMock(200);

    exporter_->publishTable(makeTable("items"));

    ASSERT_FALSE(captured_.empty());
    const auto& body = captured_[0].body;
    EXPECT_NE(body.find("urn:li:dataset"), std::string::npos)
        << "Proposal body should contain DataHub dataset URN";
    EXPECT_NE(body.find("TestDB.items"), std::string::npos)
        << "URN should contain database.table name";
}

TEST_F(CatalogExporterDataHubTest, DatasetPropertiesAspectNamePresent) {
    setHttpMock(200);
    exporter_->publishTable(makeTable("widgets"));

    bool found_properties = false;
    bool found_schema     = false;
    for (const auto& cap : captured_) {
        if (cap.body.find("datasetProperties") != std::string::npos)
            found_properties = true;
        if (cap.body.find("schemaMetadata") != std::string::npos)
            found_schema = true;
    }
    EXPECT_TRUE(found_properties) << "Should emit datasetProperties proposal";
    EXPECT_TRUE(found_schema)     << "Should emit schemaMetadata proposal";
}

TEST_F(CatalogExporterDataHubTest, SchemaMetadataContainsFieldPaths) {
    setHttpMock(200);
    exporter_->publishTable(makeTable("shipments"));

    bool found_id   = false;
    bool found_name = false;
    for (const auto& cap : captured_) {
        if (cap.body.find("\"id\"") != std::string::npos) {
          found_id   = true;
        }
        if (cap.body.find("\"name\"") != std::string::npos) {
          found_name = true;
        }
    }
    EXPECT_TRUE(found_id)   << "schemaMetadata should include 'id' field";
    EXPECT_TRUE(found_name) << "schemaMetadata should include 'name' field";
}

TEST_F(CatalogExporterDataHubTest, HttpErrorStopsAndReturnsFailure) {
    setHttpMock(401, "Unauthorized");

    auto result = exporter_->publishTable(makeTable("payments"));

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error.find("401"), std::string::npos);
}

TEST_F(CatalogExporterDataHubTest, PublishMultipleTablesCallsIngestForEach) {
    setHttpMock(200);

    std::vector<SchemaManager::TableSchema> tables = {
        makeTable("a"), makeTable("b")
    };
    auto result = exporter_->publishSchema(tables);

    // 2 tables × 2 proposals each = 4 HTTP calls
    EXPECT_EQ(captured_.size(), 4u);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.entities_published, 4);
}

TEST_F(CatalogExporterDataHubTest, ProposalWrappedInProposalKey) {
    setHttpMock(200);
    exporter_->publishTable(makeTable("records"));

    ASSERT_FALSE(captured_.empty());
    auto wrapper = json::parse(captured_[0].body);
    EXPECT_TRUE(wrapper.contains("proposal"))
        << "DataHub GMS expects {\"proposal\": {...}}";
}

// ============================================================================
// Config validation tests
// ============================================================================

TEST(CatalogExporterConfigTest, AtlasTypeIsDefault) {
    CatalogExporter::Config cfg;
    EXPECT_EQ(cfg.type, CatalogExporter::CatalogType::APACHE_ATLAS);
}

TEST(CatalogExporterConfigTest, DefaultDatabaseNameIsThemisDB) {
    CatalogExporter::Config cfg;
    EXPECT_EQ(cfg.database_name, "ThemisDB");
}

TEST(CatalogExporterConfigTest, DefaultTimeoutIs10Seconds) {
    CatalogExporter::Config cfg;
    EXPECT_EQ(cfg.timeout_ms, 10000);
}
