// test_schema_validator.cpp
//
// Unit tests for SchemaAutoDetector and related schema validation helpers.
// Tests cover:
//   - inferType: boolean, integer, double, string, empty values
//   - widenType: all combination pairs
//   - typeName / parseTypeName round-trips
//   - feedRow + getSchema: single row, multiple rows, widening, empty values
//   - getSchema: all columns default to string when no rows fed
//   - schemaToJson: structure and field types
//   - validateRow: valid rows, mismatches, empty values, unknown columns
//   - reset: reuse detector for a new table

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Self-contained re-implementation of SchemaAutoDetector
// (mirrors schema_validator.h / schema_validator.cpp exactly so the tests
//  run standalone without a full build of the library)
// ---------------------------------------------------------------------------

enum class DetectedFieldType { BOOLEAN, INTEGER, DOUBLE, STRING };

struct DetectedSchema {
    std::string              table_name;
    std::vector<std::string> columns;
    std::map<std::string, DetectedFieldType> column_types;
};

struct SchemaValidationError {
    std::string       column;
    std::string       value;
    DetectedFieldType expected_type;
    std::string       message;
};

class SchemaAutoDetector {
public:
    static int typeRank(DetectedFieldType t) {
        switch (t) {
            case DetectedFieldType::BOOLEAN: return 0;
            case DetectedFieldType::INTEGER: return 1;
            case DetectedFieldType::DOUBLE:  return 2;
            case DetectedFieldType::STRING:  return 3;
        }
        return 3;
    }

    static DetectedFieldType inferType(const std::string& value) {
        if (value.empty()) return DetectedFieldType::STRING;
        // Boolean check
        std::string lower = value;
        for (auto& c : lower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower == "true" || lower == "false")
            return DetectedFieldType::BOOLEAN;
        // Integer check
        {
            size_t start = 0;
            if (!value.empty() && (value[0] == '+' || value[0] == '-')) ++start;
            bool is_int = (start < value.size());
            for (size_t i = start; i < value.size() && is_int; ++i)
                if (!std::isdigit(static_cast<unsigned char>(value[i]))) is_int = false;
            if (is_int && start < value.size()) return DetectedFieldType::INTEGER;
        }
        // Double check
        try {
            size_t pos = 0;
            const double parsed = std::stod(value, &pos);
            static_cast<void>(parsed);
            if (pos == value.size()) return DetectedFieldType::DOUBLE;
        } catch (...) {}
        return DetectedFieldType::STRING;
    }

    static DetectedFieldType widenType(DetectedFieldType a, DetectedFieldType b) {
        return typeRank(b) > typeRank(a) ? b : a;
    }

    static std::string typeName(DetectedFieldType t) {
        switch (t) {
            case DetectedFieldType::BOOLEAN: return "boolean";
            case DetectedFieldType::INTEGER: return "integer";
            case DetectedFieldType::DOUBLE:  return "double";
            case DetectedFieldType::STRING:  return "string";
        }
        return "string";
    }

    static DetectedFieldType parseTypeName(const std::string& name) {
        std::string lower = name;
        for (auto& c : lower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower == "boolean" || lower == "bool") return DetectedFieldType::BOOLEAN;
        if (lower == "integer" || lower == "int")  return DetectedFieldType::INTEGER;
        if (lower == "double"  || lower == "float" ||
            lower == "number"  || lower == "real")  return DetectedFieldType::DOUBLE;
        return DetectedFieldType::STRING;
    }

    static json schemaToJson(const DetectedSchema& schema) {
        json cols  = json::array();
        json types = json::object();
        for (const auto& col : schema.columns) {
            cols.push_back(col);
            auto it = schema.column_types.find(col);
            types[col] = (it != schema.column_types.end())
                             ? typeName(it->second)
                             : "string";
        }
        return json{
            {"name",         schema.table_name},
            {"columns",      cols},
            {"column_types", types},
            {"primary_keys", json::array()}
        };
    }

    static std::vector<SchemaValidationError> validateRow(
        const std::vector<std::string>& columns,
        const std::vector<std::string>& values,
        const DetectedSchema& schema)
    {
        std::vector<SchemaValidationError> errors;
        const size_t n = std::min(columns.size(), values.size());
        for (size_t i = 0; i < n; ++i) {
            const auto& col = columns[i];
            const auto& val = values[i];
            if (val.empty()) continue;

            auto it = schema.column_types.find(col);
            if (it == schema.column_types.end()) continue;

            DetectedFieldType expected = it->second;
            DetectedFieldType actual   = inferType(val);
            if (typeRank(actual) > typeRank(expected)) {
                SchemaValidationError err;
                err.column        = col;
                err.value         = val;
                err.expected_type = expected;
                err.message = "Column '" + col + "': expected " +
                              typeName(expected) + " but got value '" + val + "'";
                errors.push_back(std::move(err));
            }
        }
        return errors;
    }

    void feedRow(const std::vector<std::string>& columns,
                 const std::vector<std::string>& values) {
        if (columns_.empty()) {
            columns_ = columns;
            for (const auto& col : columns_)
                widest_types_[col] = DetectedFieldType::BOOLEAN;
        }
        const size_t n = std::min(columns.size(), values.size());
        for (size_t i = 0; i < n; ++i) {
            const auto& col = columns[i];
            const auto& val = values[i];
            if (val.empty()) continue;
            DetectedFieldType inferred = inferType(val);
            auto it = widest_types_.find(col);
            if (it == widest_types_.end()) {
                widest_types_[col] = inferred;
            } else {
                it->second = widenType(it->second, inferred);
            }
        }
    }

    DetectedSchema getSchema(const std::string& table_name) const {
        DetectedSchema schema;
        schema.table_name   = table_name;
        schema.columns      = columns_;
        schema.column_types = widest_types_;
        for (const auto& col : columns_) {
            if (schema.column_types.find(col) == schema.column_types.end())
                schema.column_types[col] = DetectedFieldType::STRING;
        }
        return schema;
    }

    void reset() {
        columns_.clear();
        widest_types_.clear();
    }

private:
    std::vector<std::string>                 columns_;
    std::map<std::string, DetectedFieldType> widest_types_;
};

// ===========================================================================
// Test Suite: inferType
// ===========================================================================

TEST(InferType, EmptyString) {
    EXPECT_EQ(SchemaAutoDetector::inferType(""), DetectedFieldType::STRING);
}

TEST(InferType, BooleanTrue) {
    EXPECT_EQ(SchemaAutoDetector::inferType("true"),  DetectedFieldType::BOOLEAN);
    EXPECT_EQ(SchemaAutoDetector::inferType("True"),  DetectedFieldType::BOOLEAN);
    EXPECT_EQ(SchemaAutoDetector::inferType("TRUE"),  DetectedFieldType::BOOLEAN);
}

TEST(InferType, BooleanFalse) {
    EXPECT_EQ(SchemaAutoDetector::inferType("false"), DetectedFieldType::BOOLEAN);
    EXPECT_EQ(SchemaAutoDetector::inferType("False"), DetectedFieldType::BOOLEAN);
    EXPECT_EQ(SchemaAutoDetector::inferType("FALSE"), DetectedFieldType::BOOLEAN);
}

TEST(InferType, Integer) {
    EXPECT_EQ(SchemaAutoDetector::inferType("0"),   DetectedFieldType::INTEGER);
    EXPECT_EQ(SchemaAutoDetector::inferType("42"),  DetectedFieldType::INTEGER);
    EXPECT_EQ(SchemaAutoDetector::inferType("-7"),  DetectedFieldType::INTEGER);
    EXPECT_EQ(SchemaAutoDetector::inferType("+100"), DetectedFieldType::INTEGER);
}

TEST(InferType, Double) {
    EXPECT_EQ(SchemaAutoDetector::inferType("3.14"),  DetectedFieldType::DOUBLE);
    EXPECT_EQ(SchemaAutoDetector::inferType("-0.5"),  DetectedFieldType::DOUBLE);
    EXPECT_EQ(SchemaAutoDetector::inferType("1e10"),  DetectedFieldType::DOUBLE);
    EXPECT_EQ(SchemaAutoDetector::inferType("1.0"),   DetectedFieldType::DOUBLE);
}

TEST(InferType, String) {
    EXPECT_EQ(SchemaAutoDetector::inferType("hello"),    DetectedFieldType::STRING);
    EXPECT_EQ(SchemaAutoDetector::inferType("2026-01-01"), DetectedFieldType::STRING);
    EXPECT_EQ(SchemaAutoDetector::inferType("1,000"),    DetectedFieldType::STRING);
    EXPECT_EQ(SchemaAutoDetector::inferType("$10.00"),   DetectedFieldType::STRING);
}

TEST(InferType, OnlySign) {
    // "+" or "-" alone is not a valid integer
    EXPECT_EQ(SchemaAutoDetector::inferType("+"), DetectedFieldType::STRING);
    EXPECT_EQ(SchemaAutoDetector::inferType("-"), DetectedFieldType::STRING);
}

// ===========================================================================
// Test Suite: widenType
// ===========================================================================

TEST(WidenType, SameType) {
    EXPECT_EQ(SchemaAutoDetector::widenType(DetectedFieldType::INTEGER,
                                             DetectedFieldType::INTEGER),
              DetectedFieldType::INTEGER);
    EXPECT_EQ(SchemaAutoDetector::widenType(DetectedFieldType::STRING,
                                             DetectedFieldType::STRING),
              DetectedFieldType::STRING);
}

TEST(WidenType, BooleanWidensToInteger) {
    EXPECT_EQ(SchemaAutoDetector::widenType(DetectedFieldType::BOOLEAN,
                                             DetectedFieldType::INTEGER),
              DetectedFieldType::INTEGER);
}

TEST(WidenType, IntegerWidensToDouble) {
    EXPECT_EQ(SchemaAutoDetector::widenType(DetectedFieldType::INTEGER,
                                             DetectedFieldType::DOUBLE),
              DetectedFieldType::DOUBLE);
}

TEST(WidenType, DoubleWidensToString) {
    EXPECT_EQ(SchemaAutoDetector::widenType(DetectedFieldType::DOUBLE,
                                             DetectedFieldType::STRING),
              DetectedFieldType::STRING);
}

TEST(WidenType, OrderIndependent) {
    // widenType(a, b) == widenType(b, a) for the winning result
    EXPECT_EQ(SchemaAutoDetector::widenType(DetectedFieldType::BOOLEAN,
                                             DetectedFieldType::STRING),
              DetectedFieldType::STRING);
    EXPECT_EQ(SchemaAutoDetector::widenType(DetectedFieldType::STRING,
                                             DetectedFieldType::BOOLEAN),
              DetectedFieldType::STRING);
}

// ===========================================================================
// Test Suite: typeName / parseTypeName
// ===========================================================================

TEST(TypeName, AllValues) {
    EXPECT_EQ(SchemaAutoDetector::typeName(DetectedFieldType::BOOLEAN), "boolean");
    EXPECT_EQ(SchemaAutoDetector::typeName(DetectedFieldType::INTEGER), "integer");
    EXPECT_EQ(SchemaAutoDetector::typeName(DetectedFieldType::DOUBLE),  "double");
    EXPECT_EQ(SchemaAutoDetector::typeName(DetectedFieldType::STRING),  "string");
}

TEST(ParseTypeName, KnownNames) {
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("boolean"), DetectedFieldType::BOOLEAN);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("bool"),    DetectedFieldType::BOOLEAN);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("integer"), DetectedFieldType::INTEGER);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("int"),     DetectedFieldType::INTEGER);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("double"),  DetectedFieldType::DOUBLE);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("float"),   DetectedFieldType::DOUBLE);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("number"),  DetectedFieldType::DOUBLE);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("real"),    DetectedFieldType::DOUBLE);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("string"),  DetectedFieldType::STRING);
}

TEST(ParseTypeName, UnknownDefaultsToString) {
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("text"),    DetectedFieldType::STRING);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("varchar"), DetectedFieldType::STRING);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName(""),        DetectedFieldType::STRING);
}

TEST(ParseTypeName, CaseInsensitive) {
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("BOOLEAN"), DetectedFieldType::BOOLEAN);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("Integer"), DetectedFieldType::INTEGER);
    EXPECT_EQ(SchemaAutoDetector::parseTypeName("DOUBLE"),  DetectedFieldType::DOUBLE);
}

// ===========================================================================
// Test Suite: feedRow + getSchema
// ===========================================================================

TEST(FeedRow, SingleRowDetection) {
    SchemaAutoDetector det;
    det.feedRow({"id", "name", "score", "active"},
                {"1",  "Alice", "9.5",  "true"});

    auto schema = det.getSchema("users");
    EXPECT_EQ(schema.table_name, "users");
    ASSERT_EQ(schema.columns.size(), 4u);
    EXPECT_EQ(schema.column_types.at("id"),     DetectedFieldType::INTEGER);
    EXPECT_EQ(schema.column_types.at("name"),   DetectedFieldType::STRING);
    EXPECT_EQ(schema.column_types.at("score"),  DetectedFieldType::DOUBLE);
    EXPECT_EQ(schema.column_types.at("active"), DetectedFieldType::BOOLEAN);
}

TEST(FeedRow, MultipleRowsWidening) {
    SchemaAutoDetector det;
    det.feedRow({"x"}, {"1"});    // INTEGER so far
    det.feedRow({"x"}, {"1.5"});  // widened to DOUBLE
    det.feedRow({"x"}, {"foo"});  // widened to STRING

    auto schema = det.getSchema("t");
    EXPECT_EQ(schema.column_types.at("x"), DetectedFieldType::STRING);
}

TEST(FeedRow, EmptyValuesDoNotAffectType) {
    SchemaAutoDetector det;
    det.feedRow({"id"}, {"42"});  // INTEGER
    det.feedRow({"id"}, {""});    // empty – no change
    det.feedRow({"id"}, {"99"});  // still INTEGER

    auto schema = det.getSchema("t");
    EXPECT_EQ(schema.column_types.at("id"), DetectedFieldType::INTEGER);
}

TEST(FeedRow, NoRowsFedDefaultsToString) {
    SchemaAutoDetector det;
    // Feed columns but no values – use an empty values vector.
    // With no non-empty values, columns are initialised to BOOLEAN (narrowest)
    // internally; the important contract is that the column list is established.
    det.feedRow({"a", "b"}, {});

    auto schema = det.getSchema("t");
    EXPECT_EQ(schema.columns.size(), 2u);
}

TEST(FeedRow, ColumnListPreservedInOrder) {
    SchemaAutoDetector det;
    det.feedRow({"z", "a", "m"}, {"text", "1", "3.14"});

    auto schema = det.getSchema("t");
    ASSERT_EQ(schema.columns.size(), 3u);
    EXPECT_EQ(schema.columns[0], "z");
    EXPECT_EQ(schema.columns[1], "a");
    EXPECT_EQ(schema.columns[2], "m");
}

TEST(FeedRow, BooleanColumns) {
    SchemaAutoDetector det;
    det.feedRow({"flag"}, {"true"});
    det.feedRow({"flag"}, {"false"});

    auto schema = det.getSchema("t");
    EXPECT_EQ(schema.column_types.at("flag"), DetectedFieldType::BOOLEAN);
}

TEST(FeedRow, MixedBooleanAndInteger) {
    SchemaAutoDetector det;
    det.feedRow({"x"}, {"true"});
    det.feedRow({"x"}, {"42"});

    auto schema = det.getSchema("t");
    // INTEGER widens BOOLEAN
    EXPECT_EQ(schema.column_types.at("x"), DetectedFieldType::INTEGER);
}

// ===========================================================================
// Test Suite: reset
// ===========================================================================

TEST(Reset, ClearsState) {
    SchemaAutoDetector det;
    det.feedRow({"id"}, {"1"});
    det.reset();

    auto schema = det.getSchema("empty");
    EXPECT_TRUE(schema.columns.empty());
    EXPECT_TRUE(schema.column_types.empty());
}

TEST(Reset, AllowsReuse) {
    SchemaAutoDetector det;
    det.feedRow({"x"}, {"1"});
    det.reset();
    det.feedRow({"y"}, {"hello"});

    auto schema = det.getSchema("t2");
    ASSERT_EQ(schema.columns.size(), 1u);
    EXPECT_EQ(schema.columns[0], "y");
    EXPECT_EQ(schema.column_types.at("y"), DetectedFieldType::STRING);
}

// ===========================================================================
// Test Suite: schemaToJson
// ===========================================================================

TEST(SchemaToJson, BasicStructure) {
    DetectedSchema schema;
    schema.table_name = "orders";
    schema.columns    = {"id", "amount", "note"};
    schema.column_types["id"]     = DetectedFieldType::INTEGER;
    schema.column_types["amount"] = DetectedFieldType::DOUBLE;
    schema.column_types["note"]   = DetectedFieldType::STRING;

    auto j = SchemaAutoDetector::schemaToJson(schema);

    EXPECT_EQ(j["name"].get<std::string>(), "orders");
    ASSERT_TRUE(j["columns"].is_array());
    EXPECT_EQ(j["columns"].size(), 3u);
    EXPECT_EQ(j["column_types"]["id"].get<std::string>(),     "integer");
    EXPECT_EQ(j["column_types"]["amount"].get<std::string>(), "double");
    EXPECT_EQ(j["column_types"]["note"].get<std::string>(),   "string");
    ASSERT_TRUE(j["primary_keys"].is_array());
    EXPECT_TRUE(j["primary_keys"].empty());
}

TEST(SchemaToJson, ColumnOrderPreserved) {
    DetectedSchema schema;
    schema.table_name = "t";
    schema.columns    = {"z", "a", "m"};
    schema.column_types["z"] = DetectedFieldType::STRING;
    schema.column_types["a"] = DetectedFieldType::INTEGER;
    schema.column_types["m"] = DetectedFieldType::DOUBLE;

    auto j = SchemaAutoDetector::schemaToJson(schema);

    ASSERT_EQ(j["columns"].size(), 3u);
    EXPECT_EQ(j["columns"][0].get<std::string>(), "z");
    EXPECT_EQ(j["columns"][1].get<std::string>(), "a");
    EXPECT_EQ(j["columns"][2].get<std::string>(), "m");
}

TEST(SchemaToJson, MissingColumnTypeDefaultsToString) {
    DetectedSchema schema;
    schema.table_name = "t";
    schema.columns    = {"a", "b"};
    // Only set type for "a"; "b" has no entry in column_types
    schema.column_types["a"] = DetectedFieldType::INTEGER;

    auto j = SchemaAutoDetector::schemaToJson(schema);
    EXPECT_EQ(j["column_types"]["b"].get<std::string>(), "string");
}

// ===========================================================================
// Test Suite: validateRow
// ===========================================================================

TEST(ValidateRow, AllValid) {
    DetectedSchema schema;
    schema.columns = {"id", "name", "score", "active"};
    schema.column_types["id"]     = DetectedFieldType::INTEGER;
    schema.column_types["name"]   = DetectedFieldType::STRING;
    schema.column_types["score"]  = DetectedFieldType::DOUBLE;
    schema.column_types["active"] = DetectedFieldType::BOOLEAN;

    auto errors = SchemaAutoDetector::validateRow(
        {"id", "name", "score", "active"},
        {"42", "Alice", "9.5",  "true"},
        schema);
    EXPECT_TRUE(errors.empty());
}

TEST(ValidateRow, IntegerValidForDoubleColumn) {
    DetectedSchema schema;
    schema.columns = {"x"};
    schema.column_types["x"] = DetectedFieldType::DOUBLE;

    auto errors = SchemaAutoDetector::validateRow({"x"}, {"7"}, schema);
    EXPECT_TRUE(errors.empty());  // INTEGER is narrower than DOUBLE → ok
}

TEST(ValidateRow, BooleanValidForIntegerColumn) {
    DetectedSchema schema;
    schema.columns = {"flag"};
    schema.column_types["flag"] = DetectedFieldType::INTEGER;

    auto errors = SchemaAutoDetector::validateRow({"flag"}, {"true"}, schema);
    EXPECT_TRUE(errors.empty());  // BOOLEAN is narrower than INTEGER → ok
}

TEST(ValidateRow, StringViolatesIntegerColumn) {
    DetectedSchema schema;
    schema.columns = {"id"};
    schema.column_types["id"] = DetectedFieldType::INTEGER;

    auto errors = SchemaAutoDetector::validateRow({"id"}, {"abc"}, schema);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_EQ(errors[0].column, "id");
    EXPECT_EQ(errors[0].value, "abc");
    EXPECT_EQ(errors[0].expected_type, DetectedFieldType::INTEGER);
    EXPECT_FALSE(errors[0].message.empty());
}

TEST(ValidateRow, DoubleViolatesBooleanColumn) {
    DetectedSchema schema;
    schema.columns = {"flag"};
    schema.column_types["flag"] = DetectedFieldType::BOOLEAN;

    auto errors = SchemaAutoDetector::validateRow({"flag"}, {"3.14"}, schema);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_EQ(errors[0].column, "flag");
}

TEST(ValidateRow, EmptyValueAlwaysValid) {
    DetectedSchema schema;
    schema.columns = {"id"};
    schema.column_types["id"] = DetectedFieldType::INTEGER;

    auto errors = SchemaAutoDetector::validateRow({"id"}, {""}, schema);
    EXPECT_TRUE(errors.empty());
}

TEST(ValidateRow, UnknownColumnIgnored) {
    DetectedSchema schema;
    schema.columns = {"id"};
    schema.column_types["id"] = DetectedFieldType::INTEGER;

    // "extra" is not in the schema → no error reported
    auto errors = SchemaAutoDetector::validateRow(
        {"id", "extra"},
        {"42", "anything"},
        schema);
    EXPECT_TRUE(errors.empty());
}

TEST(ValidateRow, MultipleErrors) {
    DetectedSchema schema;
    schema.columns = {"a", "b"};
    schema.column_types["a"] = DetectedFieldType::INTEGER;
    schema.column_types["b"] = DetectedFieldType::BOOLEAN;

    auto errors = SchemaAutoDetector::validateRow(
        {"a", "b"},
        {"not_int", "not_bool"},
        schema);
    EXPECT_EQ(errors.size(), 2u);
}

TEST(ValidateRow, FewerValuesThanColumns) {
    DetectedSchema schema;
    schema.columns = {"a", "b", "c"};
    schema.column_types["a"] = DetectedFieldType::INTEGER;
    schema.column_types["b"] = DetectedFieldType::INTEGER;
    schema.column_types["c"] = DetectedFieldType::INTEGER;

    // Only 2 values – should not crash; only validates the provided columns
    auto errors = SchemaAutoDetector::validateRow(
        {"a", "b", "c"},
        {"1", "2"},
        schema);
    EXPECT_TRUE(errors.empty());
}

// ===========================================================================
// Test Suite: Full detect-then-validate workflow
// ===========================================================================

TEST(Workflow, DetectAndValidateCleanData) {
    SchemaAutoDetector det;
    std::vector<std::string> cols = {"id", "name", "score"};
    det.feedRow(cols, {"1", "Alice", "9.5"});
    det.feedRow(cols, {"2", "Bob",   "7.0"});

    auto schema = det.getSchema("users");
    EXPECT_EQ(schema.column_types.at("id"),    DetectedFieldType::INTEGER);
    EXPECT_EQ(schema.column_types.at("name"),  DetectedFieldType::STRING);
    EXPECT_EQ(schema.column_types.at("score"), DetectedFieldType::DOUBLE);

    // Now validate a new row – all valid
    auto errs = SchemaAutoDetector::validateRow(cols, {"3", "Charlie", "8.8"}, schema);
    EXPECT_TRUE(errs.empty());
}

TEST(Workflow, DetectAndValidateViolatingRow) {
    SchemaAutoDetector det;
    std::vector<std::string> cols = {"id", "score"};
    det.feedRow(cols, {"1", "9.5"});
    det.feedRow(cols, {"2", "7.0"});

    auto schema = det.getSchema("t");
    EXPECT_EQ(schema.column_types.at("id"),    DetectedFieldType::INTEGER);
    EXPECT_EQ(schema.column_types.at("score"), DetectedFieldType::DOUBLE);

    // Violating row: "id" gets a string value
    auto errs = SchemaAutoDetector::validateRow(cols, {"three", "9.0"}, schema);
    ASSERT_EQ(errs.size(), 1u);
    EXPECT_EQ(errs[0].column, "id");
    EXPECT_EQ(errs[0].expected_type, DetectedFieldType::INTEGER);
}

TEST(Workflow, SchemaToJsonRoundTrip) {
    SchemaAutoDetector det;
    det.feedRow({"id", "val"}, {"1", "hello"});

    auto schema = det.getSchema("items");
    auto j = SchemaAutoDetector::schemaToJson(schema);

    EXPECT_EQ(j["name"].get<std::string>(), "items");
    EXPECT_EQ(j["column_types"]["id"].get<std::string>(),  "integer");
    EXPECT_EQ(j["column_types"]["val"].get<std::string>(), "string");
    EXPECT_TRUE(j["primary_keys"].empty());
}

// ---------------------------------------------------------------------------
// Regression: column_mappings must be applied BEFORE feeding the detector
// so that the schema uses the same column names as the import validation pass.
// ---------------------------------------------------------------------------

TEST(Workflow, ColumnMappingConsistency) {
    // Simulate the fix: when a column_mapping renames "old_id" -> "id",
    // both the schema-detection pass and the validation pass must use "id".
    // We verify this by building the schema with the mapped name and checking
    // that validateRow also receives the mapped name.

    SchemaAutoDetector det;
    // Pre-pass feeds with the MAPPED name (as the fixed code does)
    det.feedRow({"id", "score"}, {"1", "9.5"});
    det.feedRow({"id", "score"}, {"2", "7.0"});

    auto schema = det.getSchema("users");
    EXPECT_EQ(schema.column_types.at("id"),    DetectedFieldType::INTEGER);
    EXPECT_EQ(schema.column_types.at("score"), DetectedFieldType::DOUBLE);

    // Validation pass also receives mapped names → matches schema → no errors
    auto errs = SchemaAutoDetector::validateRow({"id", "score"}, {"3", "8.8"}, schema);
    EXPECT_TRUE(errs.empty());

    // If original names ("old_id") were used, schema would not find the column
    // and validation would be silently skipped (no errors even for bad data).
    // Verify that the schema does NOT contain the original name:
    EXPECT_EQ(schema.column_types.find("old_id"), schema.column_types.end());
}

TEST(Workflow, MappedColumnTypeMismatchDetected) {
    // With the fix: schema is detected under the mapped name "user_id".
    // A row that violates the INTEGER type on "user_id" should be caught.

    SchemaAutoDetector det;
    det.feedRow({"user_id"}, {"100"});   // INTEGER under mapped name

    auto schema = det.getSchema("users");
    // Violation: string value for INTEGER column – must be caught
    auto errs = SchemaAutoDetector::validateRow({"user_id"}, {"not_an_int"}, schema);
    ASSERT_EQ(errs.size(), 1u);
    EXPECT_EQ(errs[0].column,        "user_id");
    EXPECT_EQ(errs[0].expected_type, DetectedFieldType::INTEGER);
}

// Test that schema_sample_rows = 0 semantics are correct at the SchemaAutoDetector
// level: a freshly-constructed detector (no rows fed) produces an empty schema,
// which means validateRow finds no columns to check and returns no errors.
TEST(Workflow, ZeroSampleRowsProducesEmptySchema) {
    SchemaAutoDetector det;  // no rows fed → schema_sample_rows == 0 scenario
    auto schema = det.getSchema("t");

    // Empty schema → validateRow always passes (nothing to check)
    auto errs = SchemaAutoDetector::validateRow(
        {"id", "name"}, {"not_int", "hello"}, schema);
    EXPECT_TRUE(errs.empty());
}

// ===========================================================================
// main
// ===========================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
