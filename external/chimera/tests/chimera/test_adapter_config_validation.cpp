/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_adapter_config_validation.cpp                 ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:22:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     402                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 66c21bb7c5  2026-03-12  fix(chimera): address review feedback on AdapterConfig va... ║
    • a3a9d7e09c  2026-03-12  feat(chimera): implement AdapterConfig validation (v1.2.0) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * @file test_adapter_config_validation.cpp
 * @brief Unit tests for AdapterConfig validation (chimera suite)
 *
 * Covers:
 *  - Configuration schema validation (valid / invalid)
 *  - Required parameter checking (empty connection string)
 *  - Type validation (pool_size, timeout_ms, use_tls)
 *  - Range checking (pool_size, timeout_ms, port)
 *  - Connection string parsing (scheme, host, port, database, userinfo)
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"

using namespace chimera;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static AdapterConfig make_valid_config() {
    AdapterConfig cfg;
    cfg.connection_string = "themisdb://localhost:8529/mydb";
    cfg.options["pool_size"]  = int64_t{10};
    cfg.options["timeout_ms"] = int64_t{5000};
    cfg.options["use_tls"]    = false;
    return cfg;
}

// ---------------------------------------------------------------------------
// validate() — basic happy path
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, ValidConfigReturnsOk) {
    auto cfg = make_valid_config();
    auto result = cfg.validate();
    EXPECT_TRUE(result.is_ok()) << result.error_message;
}

TEST(AdapterConfigValidationTest, ValidConfigHasNoErrors) {
    auto cfg = make_valid_config();
    EXPECT_TRUE(cfg.get_validation_errors().empty());
}

// ---------------------------------------------------------------------------
// Required parameter: connection_string must not be empty
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, EmptyConnectionStringReturnsError) {
    AdapterConfig cfg;
    cfg.connection_string = "";
    auto result = cfg.validate();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST(AdapterConfigValidationTest, EmptyConnectionStringReportsError) {
    AdapterConfig cfg;
    cfg.connection_string = "";
    auto errors = cfg.get_validation_errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_NE(errors[0].find("connection_string"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Connection string: scheme validation
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, MissingSchemeSeparatorIsInvalid) {
    AdapterConfig cfg;
    cfg.connection_string = "localhostmydb";
    auto errors = cfg.get_validation_errors();
    EXPECT_FALSE(errors.empty());
    // Only the "missing separator" error — no spurious "non-empty host" error
    for (const auto& e : errors) {
        EXPECT_EQ(e.find("must specify a non-empty host"), std::string::npos)
            << "Spurious host-required error when separator is missing: " << e;
    }
}

TEST(AdapterConfigValidationTest, EmptySchemeWithSeparatorIsInvalid) {
    AdapterConfig cfg;
    cfg.connection_string = "://localhost:8529/mydb";
    auto errors = cfg.get_validation_errors();
    EXPECT_FALSE(errors.empty());
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("scheme") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Expected a scheme-related error for '://localhost:8529/mydb'";
}

TEST(AdapterConfigValidationTest, UnknownSchemeIsInvalid) {
    AdapterConfig cfg;
    cfg.connection_string = "ftp://localhost:21/mydb";
    auto errors = cfg.get_validation_errors();
    EXPECT_FALSE(errors.empty());
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("unknown URI scheme") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(AdapterConfigValidationTest, AllKnownSchemesAreAccepted) {
    const std::vector<std::string> valid_uris = {
        "themisdb://localhost:8529/db",
        "postgresql://localhost:5432/db",
        "postgres://localhost:5432/db",
        "mongodb://localhost:27017/db",
        "mongodb+srv://cluster.example.net/db",
        "bolt://localhost:7687",
        "neo4j://localhost:7687",
        "neo4j+s://cluster.example.net",
        "http://localhost:9200",
        "https://localhost:9200"
    };
    for (const auto& uri : valid_uris) {
        AdapterConfig cfg;
        cfg.connection_string = uri;
        auto errors = cfg.get_validation_errors();
        bool has_scheme_error = false;
        for (const auto& e : errors) {
            if (e.find("unknown URI scheme") != std::string::npos) {
                has_scheme_error = true;
                break;
            }
        }
        EXPECT_FALSE(has_scheme_error) << "Scheme rejected for URI: " << uri;
    }
}

// ---------------------------------------------------------------------------
// Connection string: host validation
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, MissingHostIsInvalid) {
    AdapterConfig cfg;
    cfg.connection_string = "themisdb:///mydb"; // no host
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("host") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// Connection string: port validation
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, InvalidPortStringIsInvalid) {
    AdapterConfig cfg;
    cfg.connection_string = "themisdb://localhost:notaport/db";
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("port") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(AdapterConfigValidationTest, OutOfRangePortIsInvalid) {
    AdapterConfig cfg;
    cfg.connection_string = "themisdb://localhost:99999/db";
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("port") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(AdapterConfigValidationTest, ValidPortIsAccepted) {
    AdapterConfig cfg;
    cfg.connection_string = "themisdb://localhost:1234/db";
    auto errors = cfg.get_validation_errors();
    for (const auto& e : errors) {
        EXPECT_EQ(e.find("port"), std::string::npos) << "Unexpected port error: " << e;
    }
}

// ---------------------------------------------------------------------------
// Type validation: pool_size must be int64_t
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, PoolSizeAsStringIsInvalid) {
    auto cfg = make_valid_config();
    cfg.options["pool_size"] = std::string{"10"};
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("pool_size") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(AdapterConfigValidationTest, PoolSizeAsDoubleIsInvalid) {
    auto cfg = make_valid_config();
    cfg.options["pool_size"] = double{10.0};
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("pool_size") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(AdapterConfigValidationTest, PoolSizeAsInt64IsValid) {
    auto cfg = make_valid_config();
    cfg.options["pool_size"] = int64_t{20};
    EXPECT_TRUE(cfg.get_validation_errors().empty());
}

// ---------------------------------------------------------------------------
// Range checking: pool_size
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, PoolSizeZeroIsInvalid) {
    auto cfg = make_valid_config();
    cfg.options["pool_size"] = int64_t{0};
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("pool_size") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(AdapterConfigValidationTest, PoolSizeNegativeIsInvalid) {
    auto cfg = make_valid_config();
    cfg.options["pool_size"] = int64_t{-5};
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("pool_size") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(AdapterConfigValidationTest, PoolSizeExceedsMaxIsInvalid) {
    auto cfg = make_valid_config();
    cfg.options["pool_size"] = int64_t{99999};
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("pool_size") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// Range checking: timeout_ms
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, TimeoutMsZeroIsInvalid) {
    auto cfg = make_valid_config();
    cfg.options["timeout_ms"] = int64_t{0};
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("timeout_ms") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(AdapterConfigValidationTest, TimeoutMsValidIsAccepted) {
    auto cfg = make_valid_config();
    cfg.options["timeout_ms"] = int64_t{30000};
    EXPECT_TRUE(cfg.get_validation_errors().empty());
}

// ---------------------------------------------------------------------------
// Type validation: use_tls must be bool
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, UseTlsAsIntIsInvalid) {
    auto cfg = make_valid_config();
    cfg.options["use_tls"] = int64_t{1};
    auto errors = cfg.get_validation_errors();
    bool found = false;
    for (const auto& e : errors) {
        if (e.find("use_tls") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(AdapterConfigValidationTest, UseTlsAsBoolIsValid) {
    auto cfg = make_valid_config();
    cfg.options["use_tls"] = true;
    EXPECT_TRUE(cfg.get_validation_errors().empty());
}

// ---------------------------------------------------------------------------
// get_validation_errors() returns all errors (not just the first)
// ---------------------------------------------------------------------------

TEST(AdapterConfigValidationTest, MultipleErrorsReported) {
    AdapterConfig cfg;
    cfg.connection_string = "themisdb://localhost:8529/db";
    cfg.options["pool_size"]  = int64_t{0};    // range error
    cfg.options["timeout_ms"] = int64_t{-1};   // range error
    cfg.options["use_tls"]    = int64_t{1};    // type error
    auto errors = cfg.get_validation_errors();
    EXPECT_GE(errors.size(), 3u);
}

// ---------------------------------------------------------------------------
// Connection string parsing
// ---------------------------------------------------------------------------

TEST(AdapterConfigParseTest, ParseFullUri) {
    AdapterConfig cfg;
    cfg.connection_string = "themisdb://alice:secret@myhost:8529/mydb";
    auto parsed = cfg.parse_connection_string();

    EXPECT_EQ(parsed.scheme,   "themisdb");
    EXPECT_EQ(parsed.host,     "myhost");
    EXPECT_EQ(parsed.port,     "8529");
    EXPECT_EQ(parsed.database, "mydb");
    EXPECT_EQ(parsed.username, "alice");
}

TEST(AdapterConfigParseTest, ParseUriWithoutPort) {
    AdapterConfig cfg;
    cfg.connection_string = "themisdb://myhost/mydb";
    auto parsed = cfg.parse_connection_string();

    EXPECT_EQ(parsed.scheme,   "themisdb");
    EXPECT_EQ(parsed.host,     "myhost");
    EXPECT_TRUE(parsed.port.empty());
    EXPECT_EQ(parsed.database, "mydb");
}

TEST(AdapterConfigParseTest, ParseUriWithoutDatabase) {
    AdapterConfig cfg;
    cfg.connection_string = "bolt://localhost:7687";
    auto parsed = cfg.parse_connection_string();

    EXPECT_EQ(parsed.scheme, "bolt");
    EXPECT_EQ(parsed.host,   "localhost");
    EXPECT_EQ(parsed.port,   "7687");
    EXPECT_TRUE(parsed.database.empty());
}

TEST(AdapterConfigParseTest, ParseUriWithoutUserInfo) {
    AdapterConfig cfg;
    cfg.connection_string = "postgresql://dbhost:5432/testdb";
    auto parsed = cfg.parse_connection_string();

    EXPECT_EQ(parsed.scheme,   "postgresql");
    EXPECT_EQ(parsed.host,     "dbhost");
    EXPECT_EQ(parsed.port,     "5432");
    EXPECT_EQ(parsed.database, "testdb");
    EXPECT_TRUE(parsed.username.empty());
}

TEST(AdapterConfigParseTest, ParseUriQueryStringIgnored) {
    AdapterConfig cfg;
    cfg.connection_string = "postgresql://dbhost:5432/testdb?sslmode=require&connect_timeout=10";
    auto parsed = cfg.parse_connection_string();

    EXPECT_EQ(parsed.scheme,   "postgresql");
    EXPECT_EQ(parsed.host,     "dbhost");
    EXPECT_EQ(parsed.port,     "5432");
    EXPECT_EQ(parsed.database, "testdb");
}

TEST(AdapterConfigParseTest, ParseMalformedUriReturnsEmptyComponents) {
    AdapterConfig cfg;
    cfg.connection_string = "not-a-uri";
    auto parsed = cfg.parse_connection_string();

    EXPECT_TRUE(parsed.scheme.empty());
    EXPECT_TRUE(parsed.host.empty());
}
