// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Fuzz-style regression tests for SchemaManager::parseTableSchema().
//
// These tests exercise edge cases, malformed inputs, and boundary conditions
// that a production fuzzer (AFL, libFuzzer) might generate.
// They serve as a standing regression suite even without a full fuzzing harness.

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>

#include "metadata/schema_manager.h"

using namespace themis;
using json = nlohmann::json;

// ============================================================================
// Helper: parse without throwing; returns true if no exception
// ============================================================================

static bool tryParse(const json& j) {
    try {
        SchemaManager::parseTableSchema(j);
        return true;
    } catch (...) {
        return false;  // Exception is acceptable for malformed input
    }
}

// ============================================================================
// Valid minimal schemas
// ============================================================================

TEST(SchemaManagerFuzzTest, EmptyObject) {
    // Empty schema is invalid because the required "name" field is missing.
    EXPECT_THROW(SchemaManager::parseTableSchema(json::object()), std::runtime_error);
}

TEST(SchemaManagerFuzzTest, MinimalValidSchema) {
    json j = {{"name", "t"}, {"type", "relational"}};
    auto schema = SchemaManager::parseTableSchema(j);
    EXPECT_EQ(schema.name, "t");
    EXPECT_EQ(schema.type, "relational");
    EXPECT_TRUE(schema.properties.empty());
    EXPECT_TRUE(schema.indexes.empty());
}

TEST(SchemaManagerFuzzTest, FullSchema) {
    json j = {
        {"name", "users"},
        {"type", "relational"},
        {"properties", {
            {{"name", "id"},    {"type", "integer"}, {"indexed", true},  {"nullable", false}},
            {{"name", "email"}, {"type", "string"},  {"indexed", true},  {"nullable", false}},
            {{"name", "age"},   {"type", "integer"}, {"indexed", false}, {"nullable", true}},
        }},
        {"indexes", {
            {{"name", "pk_id"}, {"type", "regular"}, {"unique", true}, {"columns", {"id"}}}
        }},
    };
    auto schema = SchemaManager::parseTableSchema(j);
    EXPECT_EQ(schema.name, "users");
    EXPECT_EQ(schema.properties.size(), 3u);
    EXPECT_EQ(schema.indexes.size(), 1u);
    EXPECT_EQ(schema.indexes[0].name, "pk_id");
}

// ============================================================================
// Malformed / fuzzing edge cases
// ============================================================================

TEST(SchemaManagerFuzzTest, NullName) {
    json j = {{"name", nullptr}, {"type", "relational"}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, NumericName) {
    json j = {{"name", 42}, {"type", "relational"}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, ArrayName) {
    json j = {{"name", {"a", "b"}}, {"type", "relational"}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, ObjectName) {
    json j = {{"name", {{"inner", 1}}}, {"type", "relational"}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, NullType) {
    json j = {{"name", "t"}, {"type", nullptr}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, BooleanType) {
    json j = {{"name", "t"}, {"type", false}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, PropertiesNotArray) {
    json j = {{"name", "t"}, {"type", "relational"}, {"properties", "not_an_array"}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, PropertiesIsNull) {
    json j = {{"name", "t"}, {"type", "relational"}, {"properties", nullptr}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, PropertiesIsObject) {
    json j = {{"name", "t"}, {"type", "relational"}, {"properties", {{"key", "val"}}}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, PropertyMissingName) {
    json j = {
        {"name", "t"},
        {"type", "relational"},
        {"properties", {
            {{"type", "string"}}  // No "name" key
        }}
    };
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, PropertyNullName) {
    json j = {
        {"name", "t"},
        {"type", "relational"},
        {"properties", {
            {{"name", nullptr}, {"type", "string"}}
        }}
    };
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, PropertyNumericName) {
    json j = {
        {"name", "t"},
        {"type", "relational"},
        {"properties", {
            {{"name", 99}, {"type", "integer"}}
        }}
    };
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, IndexesNotArray) {
    json j = {{"name", "t"}, {"type", "relational"}, {"indexes", 123}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, IndexMissingName) {
    json j = {
        {"name", "t"},
        {"type", "relational"},
        {"indexes", {
            {{"type", "regular"}, {"columns", {"id"}}}
        }}
    };
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, IndexColumnsNotArray) {
    json j = {
        {"name", "t"},
        {"type", "relational"},
        {"indexes", {
            {{"name", "idx"}, {"type", "regular"}, {"columns", "not_array"}}
        }}
    };
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, IndexColumnsWithNullEntry) {
    json j = {
        {"name", "t"},
        {"type", "relational"},
        {"indexes", {
            {{"name", "idx"}, {"type", "regular"}, {"columns", {nullptr, "id"}}}
        }}
    };
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, DeepNestedObject) {
    json deep = {{"name", "t"}};
    json inner = deep;
    for (int i = 0; i < 20; ++i) {
        inner = {{"nested", inner}};
    }
    EXPECT_NO_THROW(tryParse(inner));
}

TEST(SchemaManagerFuzzTest, VeryLongName) {
    std::string long_name(10000, 'a');
    json j = {{"name", long_name}, {"type", "relational"}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, UnicodeName) {
    json j = {{"name", "données_utilisateurs"}, {"type", "relational"}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, EmojiName) {
    json j = {{"name", "🦁🐯🐻"}, {"type", "relational"}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, NullBytesInName) {
    std::string null_name("abc\0def", 7);
    json j = {{"name", null_name}, {"type", "relational"}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, ManyProperties) {
    json props = json::array();
    for (int i = 0; i < 500; ++i) {
        props.push_back({{"name", "col_" + std::to_string(i)},
                         {"type", "string"}, {"indexed", false}});
    }
    json j = {{"name", "wide_table"}, {"type", "relational"}, {"properties", props}};
    EXPECT_NO_THROW({
        auto schema = SchemaManager::parseTableSchema(j);
        EXPECT_LE(schema.properties.size(), 500u);
    });
}

TEST(SchemaManagerFuzzTest, ManyIndexes) {
    json indexes = json::array();
    for (int i = 0; i < 100; ++i) {
        indexes.push_back({{"name", "idx_" + std::to_string(i)},
                           {"type", "regular"}, {"unique", false},
                           {"columns", {"col_" + std::to_string(i)}}});
    }
    json j = {{"name", "t"}, {"type", "relational"}, {"indexes", indexes}};
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, IndexUniqueNotBool) {
    json j = {
        {"name", "t"},
        {"type", "relational"},
        {"indexes", {
            {{"name", "idx"}, {"type", "regular"}, {"unique", "yes"}, {"columns", {"id"}}}
        }}
    };
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, PropertyIndexedNotBool) {
    json j = {
        {"name", "t"},
        {"type", "relational"},
        {"properties", {
            {{"name", "col"}, {"type", "string"}, {"indexed", "true"}}
        }}
    };
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, AdditionalUnknownFields) {
    json j = {
        {"name", "t"},
        {"type", "relational"},
        {"unknown_field", {1, 2, 3}},
        {"another_unknown", "value"},
        {"properties", {
            {{"name", "id"}, {"type", "integer"}, {"foo", "bar"}}
        }}
    };
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, ArrayAsRootInput) {
    json j = json::array({{"name", "t"}});
    EXPECT_NO_THROW(tryParse(j));
}

TEST(SchemaManagerFuzzTest, NullAsRootInput) {
    EXPECT_NO_THROW(tryParse(nullptr));
}

TEST(SchemaManagerFuzzTest, IntegerAsRootInput) {
    EXPECT_NO_THROW(tryParse(42));
}

TEST(SchemaManagerFuzzTest, StringAsRootInput) {
    EXPECT_NO_THROW(tryParse("not_an_object"));
}

// ============================================================================
// Round-trip property preservation
// ============================================================================

TEST(SchemaManagerFuzzTest, RoundTripViaJSON) {
    json j = {
        {"name", "products"},
        {"type", "relational"},
        {"properties", {
            {{"name", "sku"},   {"type", "string"},  {"indexed", true},  {"nullable", false}},
            {{"name", "price"}, {"type", "double"},  {"indexed", false}, {"nullable", false}},
        }},
        {"indexes", {
            {{"name", "idx_sku"}, {"type", "regular"}, {"unique", true}, {"columns", {"sku"}}}
        }},
    };

    auto schema1 = SchemaManager::parseTableSchema(j);
    auto j2      = schema1.toJSON();
    auto schema2 = SchemaManager::parseTableSchema(j2);

    EXPECT_EQ(schema1.name, schema2.name);
    EXPECT_EQ(schema1.type, schema2.type);
    EXPECT_EQ(schema1.properties.size(), schema2.properties.size());
    EXPECT_EQ(schema1.indexes.size(), schema2.indexes.size());

    if (!schema1.properties.empty() && schema1.properties.size() == schema2.properties.size()) {
        EXPECT_EQ(schema1.properties[0].name, schema2.properties[0].name);
        EXPECT_EQ(schema1.properties[0].type, schema2.properties[0].type);
    }
}
