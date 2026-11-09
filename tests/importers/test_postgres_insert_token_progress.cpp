// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "importers/postgres_importer.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

using themis::importers::ImportOptions;
using themis::importers::PostgreSQLImporter;
using themis::importers::json;

class PostgresInsertTokenProgress : public testing::Test {
protected:
    void SetUp() override {
        directory_ = std::filesystem::temp_directory_path() /
            ("themis_insert_progress_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        created_directory_ = std::filesystem::create_directory(directory_);
        ASSERT_TRUE(created_directory_);
    }

    void TearDown() override {
        if (created_directory_) {
            std::filesystem::remove_all(directory_);
        }
    }

    void expectValues(const std::string& clause, const std::vector<std::string>& expected) {
        const auto source = directory_ / "dump.sql";
        {
            std::ofstream output(source);
            ASSERT_TRUE(output);
            output << "-- PostgreSQL database dump\n"
                   << "CREATE TABLE tokens (a text, b text, c text);\n"
                   << "INSERT INTO tokens (a, b, c) VALUES (" << clause << ");\n";
            output.close();
            ASSERT_TRUE(output);
        }

        PostgreSQLImporter importer;
        ASSERT_TRUE(importer.initialize("{}"));
        std::vector<json> rows;
        ImportOptions options;
        options.continue_on_error = false;
        options.streaming_row_callback = [&rows](const std::string&, const json& row) {
            rows.push_back(row);
            return true;
        };
        const auto stats = importer.importData(source.string(), options);
        ASSERT_EQ(stats.failed_records, 0u);
        ASSERT_EQ(stats.imported_records, 1u);
        ASSERT_EQ(rows.size(), 1u);
        const std::vector<std::string> columns{"a", "b", "c"};
        ASSERT_EQ(expected.size(), columns.size());
        for (std::size_t i = 0; i < columns.size(); ++i) {
            EXPECT_EQ(rows.front().at(columns[i]), expected[i]);
        }
    }

    std::filesystem::path directory_;
    bool created_directory_ = false;
};

TEST_F(PostgresInsertTokenProgress, ConsumesParenthesesInsideUnquotedToken) {
    expectValues("NOW(), NULL, 'last'", {"NOW()", "NULL", "last"});
}

TEST_F(PostgresInsertTokenProgress, ConsumesClosingParenthesisAtTokenStart) {
    expectValues("), 'second', 'last'", {")", "second", "last"});
}

TEST_F(PostgresInsertTokenProgress, PreservesQuotedDelimitersAndEscapedQuotes) {
    expectValues("'comma, closing)', 'it''s fine', 42",
                 {"comma, closing)", "it's fine", "42"});
}

} // namespace
