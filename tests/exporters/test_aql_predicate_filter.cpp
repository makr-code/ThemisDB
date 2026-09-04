#include <gtest/gtest.h>
#include "exporters/aql_predicate_filter.h"
#include "exporters/exporter_interface.h"
#include "exporters/jsonl_llm_exporter.h"
#include "storage/base_entity.h"
#include <filesystem>
#include <fstream>
#include <ctime>

using namespace themis::exporters;
using namespace themis;

class AqlPredicateFilterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test entities with various fields
        BaseEntity e1;
        e1.setPrimaryKey("e1");
        e1.setField("category", std::string("active"));
        e1.setField("age", int64_t(25));
        e1.setField("score", 0.8);
        e1.setField("name", std::string("Alice"));
        entities_.push_back(e1);

        BaseEntity e2;
        e2.setPrimaryKey("e2");
        e2.setField("category", std::string("inactive"));
        e2.setField("age", int64_t(17));
        e2.setField("score", 0.4);
        e2.setField("name", std::string("Bob"));
        entities_.push_back(e2);

        BaseEntity e3;
        e3.setPrimaryKey("e3");
        e3.setField("category", std::string("active"));
        e3.setField("age", int64_t(30));
        e3.setField("score", 0.9);
        e3.setField("name", std::string("Carol"));
        entities_.push_back(e3);

        // Create test output directory
        auto temp_base = std::filesystem::temp_directory_path();
        test_dir_ = (temp_base / ("themis_aql_filter_test_" +
                     std::to_string(std::time(nullptr)))).string();
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::vector<std::string> readLines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream f(path);
        std::string line = {};
        while (std::getline(f, line)) {
            if (!line.empty()) {
              lines.push_back(line);
            }
        }
        return lines;
    }

    std::vector<BaseEntity> entities_;
    std::string test_dir_ = {};
};

// ─── Unit tests for AqlPredicateFilter ───────────────────────────────────────

TEST_F(AqlPredicateFilterTest, EmptyPredicateAcceptsAll) {
    AqlPredicateFilter filter("");
    for (const auto& e : entities_) {
        EXPECT_TRUE(filter.evaluate(e));
    }
}

TEST_F(AqlPredicateFilterTest, EqualityStringFilter) {
    AqlPredicateFilter filter(R"(doc.category == "active")");
    EXPECT_TRUE(filter.evaluate(entities_[0]));   // active
    EXPECT_FALSE(filter.evaluate(entities_[1]));  // inactive
    EXPECT_TRUE(filter.evaluate(entities_[2]));   // active
}

TEST_F(AqlPredicateFilterTest, InequalityStringFilter) {
    AqlPredicateFilter filter(R"(doc.category != "inactive")");
    EXPECT_TRUE(filter.evaluate(entities_[0]));
    EXPECT_FALSE(filter.evaluate(entities_[1]));
    EXPECT_TRUE(filter.evaluate(entities_[2]));
}

TEST_F(AqlPredicateFilterTest, NumericGreaterThanFilter) {
    AqlPredicateFilter filter("doc.age > 18");
    EXPECT_TRUE(filter.evaluate(entities_[0]));   // age=25
    EXPECT_FALSE(filter.evaluate(entities_[1]));  // age=17
    EXPECT_TRUE(filter.evaluate(entities_[2]));   // age=30
}

TEST_F(AqlPredicateFilterTest, NumericLessThanFilter) {
    AqlPredicateFilter filter("doc.age < 20");
    EXPECT_FALSE(filter.evaluate(entities_[0]));
    EXPECT_TRUE(filter.evaluate(entities_[1]));
    EXPECT_FALSE(filter.evaluate(entities_[2]));
}

TEST_F(AqlPredicateFilterTest, NumericGreaterEqualFilter) {
    AqlPredicateFilter filter("doc.age >= 25");
    EXPECT_TRUE(filter.evaluate(entities_[0]));   // age=25
    EXPECT_FALSE(filter.evaluate(entities_[1]));  // age=17
    EXPECT_TRUE(filter.evaluate(entities_[2]));   // age=30
}

TEST_F(AqlPredicateFilterTest, DoubleFieldFilter) {
    AqlPredicateFilter filter("doc.score >= 0.8");
    EXPECT_TRUE(filter.evaluate(entities_[0]));   // score=0.8
    EXPECT_FALSE(filter.evaluate(entities_[1]));  // score=0.4
    EXPECT_TRUE(filter.evaluate(entities_[2]));   // score=0.9
}

TEST_F(AqlPredicateFilterTest, LogicalAndFilter) {
    AqlPredicateFilter filter(R"(doc.category == "active" AND doc.age >= 30)");
    EXPECT_FALSE(filter.evaluate(entities_[0]));  // active, age=25 (age not >=30)
    EXPECT_FALSE(filter.evaluate(entities_[1]));  // inactive
    EXPECT_TRUE(filter.evaluate(entities_[2]));   // active, age=30
}

TEST_F(AqlPredicateFilterTest, LogicalOrFilter) {
    AqlPredicateFilter filter(R"(doc.age < 18 OR doc.score >= 0.9)");
    EXPECT_FALSE(filter.evaluate(entities_[0]));  // age=25, score=0.8
    EXPECT_TRUE(filter.evaluate(entities_[1]));   // age=17
    EXPECT_TRUE(filter.evaluate(entities_[2]));   // score=0.9
}

TEST_F(AqlPredicateFilterTest, InvalidPredicateThrows) {
    EXPECT_THROW(
        AqlPredicateFilter filter("!!! invalid aql @@@"),
        AqlPredicateFilterException
    );
}

TEST_F(AqlPredicateFilterTest, MissingFieldReturnsFalse) {
    AqlPredicateFilter filter(R"(doc.nonexistent == "value")");
    // nonexistent field → null; null != "value" → false
    EXPECT_FALSE(filter.evaluate(entities_[0]));
}

TEST_F(AqlPredicateFilterTest, GetPredicateReturnsOriginal) {
    const std::string pred = R"(doc.category == "active")";
    AqlPredicateFilter filter(pred);
    EXPECT_EQ(filter.getPredicate(), pred);
}

// ─── Integration tests: filter_expression wired into JSONL exporter ──────────

class AqlPredicateFilterIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto temp_base = std::filesystem::temp_directory_path();
        test_dir_ = (temp_base / ("themis_aql_int_test_" +
                     std::to_string(std::time(nullptr)))).string();
        std::filesystem::create_directories(test_dir_);

        for (int i = 0; i < 5; ++i) {
            BaseEntity e;
            e.setPrimaryKey("entity_" + std::to_string(i));
            e.setField("question", std::string("Question ") + std::to_string(i));
            e.setField("answer",   std::string("Answer ")   + std::to_string(i));
            e.setField("score",    static_cast<double>(i) * 0.2);  // 0.0 .. 0.8
            entities_.push_back(e);
        }
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::vector<std::string> readLines(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream f(path);
        std::string line = {};
        while (std::getline(f, line)) {
            if (!line.empty()) {
              lines.push_back(line);
            }
        }
        return lines;
    }

    std::vector<BaseEntity> entities_;
    std::string test_dir_;
};

TEST_F(AqlPredicateFilterIntegrationTest, JSONLExporterFiltersEntities) {
    JSONLLLMConfig config;
    config.style = JSONLFormat::Style::INSTRUCTION_TUNING;
    config.quality.skip_duplicates = false;
    config.quality.min_text_length = 0;

    JSONLLLMExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/filtered.jsonl";
    options.filter_expression = "doc.score >= 0.4";  // scores 0.4, 0.6, 0.8 pass

    auto stats = exporter.exportEntities(entities_, options);

    EXPECT_EQ(stats.total_entities, 5u);
    EXPECT_EQ(stats.exported_entities, 3u);  // entities 2,3,4 (scores 0.4,0.6,0.8)
    EXPECT_EQ(stats.failed_entities, 0u);

    auto lines = readLines(options.output_path);
    EXPECT_EQ(lines.size(), 3u);
}

TEST_F(AqlPredicateFilterIntegrationTest, JSONLExporterNoFilterExportsAll) {
    JSONLLLMConfig config;
    config.quality.skip_duplicates = false;
    config.quality.min_text_length = 0;
    JSONLLLMExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/all.jsonl";
    // filter_expression is empty → no filtering

    auto stats = exporter.exportEntities(entities_, options);

    EXPECT_EQ(stats.total_entities, 5u);
    EXPECT_EQ(stats.exported_entities, 5u);
}

TEST_F(AqlPredicateFilterIntegrationTest, JSONLExporterInvalidFilterThrows) {
    JSONLLLMConfig config;
    JSONLLLMExporter exporter(config);

    ExportOptions options;
    options.output_path = test_dir_ + "/invalid.jsonl";
    options.filter_expression = "@#$% not valid";

    EXPECT_THROW(
        exporter.exportEntities(entities_, options),
        AqlPredicateFilterException
    );
}
