/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_path_mapping_metadata.cpp                     ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     297                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "config/path_mapping_metadata.h"
#include <thread>
#include <chrono>

namespace themis {
namespace config {
namespace test {

// Test fixture for path mapping metadata
class PathMappingMetadataTest : public ::testing::Test {
protected:
    // Helper to create a date
    static std::chrono::system_clock::time_point makeDate(int year, int month, int day) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
};

// ═══════════════════════════════════════════════════════════
// Basic Metadata Tests
// ═══════════════════════════════════════════════════════════

TEST_F(PathMappingMetadataTest, BasicMetadata) {
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    
    EXPECT_EQ(meta.legacy_path, "config/old.yaml");
    EXPECT_EQ(meta.new_path, "config/new/old.yaml");
    EXPECT_EQ(meta.category, "category");
}

// ═══════════════════════════════════════════════════════════
// Deprecation Status Tests
// ═══════════════════════════════════════════════════════════

TEST_F(PathMappingMetadataTest, NotDeprecated) {
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,  // No deprecation date
        std::nullopt,
        std::nullopt
    };
    
    EXPECT_FALSE(meta.isDeprecated());
}

TEST_F(PathMappingMetadataTest, DeprecatedInPast) {
    auto past_date = makeDate(2020, 1, 1);
    
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        past_date,
        std::nullopt,
        std::nullopt
    };
    
    EXPECT_TRUE(meta.isDeprecated());
}

TEST_F(PathMappingMetadataTest, DeprecatedInFuture) {
    auto future_date = makeDate(2030, 1, 1);
    
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        future_date,
        std::nullopt,
        std::nullopt
    };
    
    EXPECT_FALSE(meta.isDeprecated());
}

// ═══════════════════════════════════════════════════════════
// Removal Status Tests
// ═══════════════════════════════════════════════════════════

TEST_F(PathMappingMetadataTest, RemovalNotScheduled) {
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,
        std::nullopt,  // No removal date
        std::nullopt
    };
    
    EXPECT_FALSE(meta.isRemovalDue());
}

TEST_F(PathMappingMetadataTest, RemovalInPast) {
    auto past_date = makeDate(2020, 1, 1);
    
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,
        past_date,
        std::nullopt
    };
    
    EXPECT_TRUE(meta.isRemovalDue());
}

TEST_F(PathMappingMetadataTest, RemovalInFuture) {
    auto future_date = makeDate(2030, 1, 1);
    
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,
        future_date,
        std::nullopt
    };
    
    EXPECT_FALSE(meta.isRemovalDue());
}

// ═══════════════════════════════════════════════════════════
// Days Until Removal Tests
// ═══════════════════════════════════════════════════════════

TEST_F(PathMappingMetadataTest, DaysUntilRemovalNotScheduled) {
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    
    EXPECT_EQ(meta.daysUntilRemoval(), -1);
}

TEST_F(PathMappingMetadataTest, DaysUntilRemovalInFuture) {
    auto now = std::chrono::system_clock::now();
    auto future = now + std::chrono::hours(48); // 2 days in future
    
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,
        future,
        std::nullopt
    };
    
    int days = meta.daysUntilRemoval();
    EXPECT_GE(days, 1);
    EXPECT_LE(days, 3);
}

TEST_F(PathMappingMetadataTest, DaysUntilRemovalInPast) {
    auto past = makeDate(2020, 1, 1);
    
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,
        past,
        std::nullopt
    };
    
    EXPECT_LT(meta.daysUntilRemoval(), 0);
}

// ═══════════════════════════════════════════════════════════
// Deprecation Message Tests
// ═══════════════════════════════════════════════════════════

TEST_F(PathMappingMetadataTest, DeprecationMessageBasic) {
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    
    std::string msg = meta.getDeprecationMessage();
    EXPECT_NE(msg.find("config/old.yaml"), std::string::npos);
    EXPECT_NE(msg.find("config/new/old.yaml"), std::string::npos);
    EXPECT_NE(msg.find("deprecated"), std::string::npos);
}

TEST_F(PathMappingMetadataTest, DeprecationMessageWithRemovalDate) {
    auto future = std::chrono::system_clock::now() + std::chrono::hours(240); // ~10 days
    
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        makeDate(2024, 1, 1),
        future,
        std::nullopt
    };
    
    std::string msg = meta.getDeprecationMessage();
    EXPECT_NE(msg.find("days"), std::string::npos);
}

TEST_F(PathMappingMetadataTest, DeprecationMessageWithMigrationGuide) {
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        std::nullopt,
        std::nullopt,
        "https://example.com/migration"
    };
    
    std::string msg = meta.getDeprecationMessage();
    EXPECT_NE(msg.find("https://example.com/migration"), std::string::npos);
    EXPECT_NE(msg.find("Migration guide"), std::string::npos);
}

TEST_F(PathMappingMetadataTest, DeprecationMessageRemovalToday) {
    auto today = std::chrono::system_clock::now();
    
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        makeDate(2024, 1, 1),
        today,
        std::nullopt
    };
    
    std::string msg = meta.getDeprecationMessage();
    EXPECT_NE(msg.find("TODAY"), std::string::npos);
}

TEST_F(PathMappingMetadataTest, DeprecationMessageRemovalPast) {
    auto past = makeDate(2020, 1, 1);
    
    PathMappingMetadata meta{
        "config/old.yaml",
        "config/new/old.yaml",
        "category",
        makeDate(2019, 1, 1),
        past,
        std::nullopt
    };
    
    std::string msg = meta.getDeprecationMessage();
    EXPECT_NE(msg.find("days ago"), std::string::npos);
}

} // namespace test
} // namespace config
} // namespace themis
