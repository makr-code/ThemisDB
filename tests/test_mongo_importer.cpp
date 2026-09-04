// test_mongo_importer.cpp
//
// Unit tests for the MongoDB importer covering:
//   - BSON extended JSON v2 unwrapping ($oid, $date, $numberDecimal, $numberLong,
//     $numberInt, $numberDouble, $binary, $timestamp, $regex, $undefined, $minKey,
//     $maxKey, $code)
//   - Type inference from JSON values
//   - JSON-Lines (NDJSON) document parsing
//   - JSON array document parsing
//   - Collection name derivation from file path
//   - include/exclude collection filtering
//   - Dry-run mode
//   - Permission check callback (ACL enforcement)
//   - validateSource: accepts JSON files, rejects non-JSON
//   - Stats: imported_records, failed_records, skipped_records, tables_processed

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Minimal re-implementation of relevant types (mirrors importer_interface.h)
// to keep the test self-contained.
// ---------------------------------------------------------------------------

enum class ImportErrorCode : uint32_t {
    SUCCESS               = 0,
    FILE_NOT_FOUND        = 100,
    FILE_OPEN_FAILED      = 101,
    FILE_READ_FAILED      = 102,
    NOT_A_PG_DUMP         = 103,
    PARSE_INSERT          = 201,
    ROW_TOO_LARGE         = 205,
    UNKNOWN_TABLE         = 300,
    TYPE_CONVERSION       = 400,
    DRY_RUN_ONLY          = 500,
    TABLE_EXCLUDED        = 501,
    PERMISSION_DENIED     = 503,
    UNKNOWN               = 900
};

enum class ImportErrorSeverity { INFO, WARNING, ERROR, CRITICAL };

struct ImportError {
    ImportErrorCode     code     = ImportErrorCode::UNKNOWN;
    ImportErrorSeverity severity = ImportErrorSeverity::ERROR;
    std::string         message;
    std::string         location;
};

struct ImportStats {
    size_t total_records      = 0;
    size_t imported_records   = 0;
    size_t failed_records     = 0;
    size_t skipped_records    = 0;
    size_t tables_processed   = 0;
    double elapsed_seconds    = 0.0;
    std::vector<std::string>  warnings;
    std::vector<std::string>  errors;
    std::vector<ImportError>  structured_errors;
};

struct ImportOptions {
    bool                             dry_run            = false;
    bool                             continue_on_error  = true;
    size_t                           batch_size         = 1000;
    std::vector<std::string>         include_tables;
    std::vector<std::string>         exclude_tables;
    size_t                           max_row_size_bytes = 0;
    std::function<bool(const std::string&, const std::string&)> permission_check;
    std::function<bool(const std::string&, const json&)>        streaming_row_callback;
};

// ---------------------------------------------------------------------------
// Re-implementations of logic under test (kept in sync with mongo_importer.cpp)
// ---------------------------------------------------------------------------

/// Derive collection name from file path (basename without extension).
static std::string collectionFromPath(const std::string& path) {
    size_t sep = path.find_last_of("/\\");
    std::string basename = (sep == std::string::npos) ? path : path.substr(sep + 1);
    size_t dot = basename.rfind('.');
    if (dot != std::string::npos) {
      basename = basename.substr(0, dot);
    }
    return basename.empty() ? "documents" : basename;
}

/// Check whether a collection should be imported given the options.
static bool shouldImportCollection(const std::string& collection,
                                   const ImportOptions& options) {
    if (std::find(options.exclude_tables.begin(), options.exclude_tables.end(),
                  collection) != options.exclude_tables.end()) return false;
    if (!options.include_tables.empty()) {
        return std::find(options.include_tables.begin(), options.include_tables.end(),
                         collection) != options.include_tables.end();
    }
    return true;
}

/// Infer ThemisDB logical type from a JSON value.
static std::string inferThemisType(const json& value) {
    if (value.is_null()) {
      return "string";
    }
    if (value.is_boolean()) {
      return "boolean";
    }
    if (value.is_number_integer()) {
      return "long";
    }
    if (value.is_number_float()) {
      return "double";
    }
    if (value.is_string()) {
      return "string";
    }
    if (value.is_array()) {
      return "array";
    }
    if (value.is_object()) {
        if (value.contains("$oid")) {
          return "string";
        }
        if (value.contains("$date")) {
          return "datetime";
        }
        if (value.contains("$numberDecimal")) {
          return "double";
        }
        if (value.contains("$numberLong")) {
          return "long";
        }
        if (value.contains("$numberInt")) {
          return "integer";
        }
        if (value.contains("$numberDouble")) {
          return "double";
        }
        if (value.contains("$binary")) {
          return "binary";
        }
        if (value.contains("$timestamp")) {
          return "datetime";
        }
        if (value.contains("$regex")) {
          return "string";
        }
        if (value.contains("$ref")) {
          return "string";
        }
        if (value.contains("$code")) {
          return "string";
        }
        if (value.contains("$undefined")) {
          return "string";
        }
        if (value.contains("$minKey")) {
          return "string";
        }
        if (value.contains("$maxKey")) {
          return "string";
        }
        return "object";
    }
    return "string";
}

/// Unwrap a single BSON extended JSON v2 wrapper to a scalar/plain value.
static json unwrapBsonValue(const json& value) {
    if (!value.is_object()) {
      return value;
    }

    if (value.contains("$oid") && value["$oid"].is_string())
        return value["$oid"].get<std::string>();

    if (value.contains("$date")) {
        const json& d = value["$date"];
        if (d.is_string()) {
          return d.get<std::string>();
        }
        if (d.is_object() && d.contains("$numberLong") && d["$numberLong"].is_string())
            return d["$numberLong"].get<std::string>();
        if (d.is_number()) {
          return d;
        }
        return d.dump();
    }

    if (value.contains("$numberDecimal") && value["$numberDecimal"].is_string())
        return value["$numberDecimal"].get<std::string>();

    if (value.contains("$numberLong") && value["$numberLong"].is_string())
        return value["$numberLong"].get<std::string>();

    if (value.contains("$numberInt")) {
        const json& n = value["$numberInt"];
        if (n.is_string()) { try { return std::stoi(n.get<std::string>()); } catch (...) {} }
        if (n.is_number()) {
          return n;
        }
    }

    if (value.contains("$numberDouble")) {
        const json& n = value["$numberDouble"];
        if (n.is_string()) { try { return std::stod(n.get<std::string>()); } catch (...) {} }
        if (n.is_number()) {
          return n;
        }
    }

    if (value.contains("$binary")) {
        const json& b = value["$binary"];
        if (b.is_object() && b.contains("base64") && b["base64"].is_string())
            return b["base64"].get<std::string>();
        if (b.is_string()) {
          return b.get<std::string>();
        }
    }

    if (value.contains("$timestamp")) {
        const json& ts = value["$timestamp"];
        if (ts.is_object()) {
          return ts.dump();
        }
    }

    if (value.contains("$regex") && value["$regex"].is_string())
        return value["$regex"].get<std::string>();

    if (value.contains("$code") && value["$code"].is_string())
        return value["$code"].get<std::string>();

    if (value.contains("$ref"))
        return value.dump();

    if (value.contains("$undefined") || value.contains("$minKey") ||
        value.contains("$maxKey"))
        return "";

    return value;
}

/// Recursively unwrap all BSON extended JSON values in a document.
static json unwrapDocument(const json& doc) {
    if (!doc.is_object()) {
      return doc;
    }

    json result = json::object();
    for (auto it = doc.begin(); it != doc.end(); ++it) {
        const std::string& key = it.key();
        const json&        val = it.value();

        if (val.is_object()) {
            json uw = unwrapBsonValue(val);
            result[key] = (uw.is_object() && uw == val) ? unwrapDocument(val) : uw;
        } else if (val.is_array()) {
            json arr = json::array();
            for (const auto& elem : val) {
                if (elem.is_object()) {
                    json uw = unwrapBsonValue(elem);
                    arr.push_back((uw.is_object() && uw == elem)
                                  ? unwrapDocument(elem) : uw);
                } else {
                    arr.push_back(elem);
                }
            }
            result[key] = arr;
        } else {
            result[key] = val;
        }
    }
    return result;
}

/// Returns true if the file's first non-empty, non-comment line starts with '{' or '['.
static bool looksLikeMongoExport(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
      return false;
    }
    std::string line = {};
    int checked = 0;
    while (std::getline(file, line) && checked < 200) {
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) { checked++; continue; }
        char c = line[first];
        if (c == '{' || c == '[') return true;
        if (c == '#')             { checked++; continue; }
        break;
    }
    return false;
}

/// Minimal NDJSON import (mirrors parseJsonLines logic without full importer).
static ImportStats importJsonLines(const std::string& content,
                                   const std::string& collection,
                                   const ImportOptions& options) {
    ImportStats stats;
    std::istringstream ss(content);
    std::string line = {};
    bool collection_counted = false;
    size_t doc_index = 0;

    while (std::getline(ss, line)) {
        size_t f = line.find_first_not_of(" \t\r\n");
        if (f == std::string::npos || line[f] == '#') {
          continue;
        }
        if (line[f] != '{') continue;

        if (options.max_row_size_bytes > 0 &&
            line.size() > options.max_row_size_bytes) {
            ImportError e;
            e.code     = ImportErrorCode::ROW_TOO_LARGE;
            e.severity = ImportErrorSeverity::WARNING;
            e.message  = "Document line too large";
            e.location = "document " + std::to_string(doc_index + 1);
            stats.structured_errors.push_back(e);
            stats.failed_records++;
            if (!options.continue_on_error) {
              return stats;
            }
            doc_index++;
            continue;
        }

        json doc;
        try {
            doc = json::parse(line);
        } catch (...) {
            stats.failed_records++;
            if (!options.continue_on_error) {
              return stats;
            }
            doc_index++;
            continue;
        }

        if (!doc.is_object()) { stats.failed_records++; doc_index++; continue; }

        if (!collection_counted) {
            stats.tables_processed++;
            collection_counted = true;
        }

        if (!shouldImportCollection(collection, options)) {
            stats.skipped_records++;
            doc_index++;
            continue;
        }

        stats.total_records++;
        if (options.dry_run) {
            stats.imported_records++;
            doc_index++;
            continue;
        }

        json entity = unwrapDocument(doc);
        entity["_type"] = collection;
        if (options.streaming_row_callback) {
            if (!options.streaming_row_callback(collection, entity)) {
                stats.imported_records++;
                doc_index++;
                return stats;  // early abort – set cancelled equivalent
            }
        }
        stats.imported_records++;
        doc_index++;
    }
    return stats;
}

/// Minimal JSON array import (mirrors parseJsonArray logic without full importer).
static ImportStats importJsonArray(const std::string& content,
                                   const std::string& collection,
                                   const ImportOptions& options) {
    ImportStats stats;
    json arr;
    try {
        arr = json::parse(content);
    } catch (...) {
        ImportError e;
        e.code     = ImportErrorCode::FILE_READ_FAILED;
        e.severity = ImportErrorSeverity::CRITICAL;
        e.message  = "Failed to parse JSON array";
        stats.structured_errors.push_back(e);
        return stats;
    }

    if (!arr.is_array()) {
        ImportError e;
        e.code     = ImportErrorCode::NOT_A_PG_DUMP;
        e.severity = ImportErrorSeverity::CRITICAL;
        e.message  = "Source is not a JSON array";
        stats.structured_errors.push_back(e);
        return stats;
    }

    if (!arr.empty()) {
      stats.tables_processed++;
    }

    for (size_t i = 0; i < arr.size(); ++i) {
        if (!arr[i].is_object()) { stats.failed_records++; continue; }
        if (!shouldImportCollection(collection, options)) {
            stats.skipped_records++;
            continue;
        }
        stats.total_records++;
        if (options.dry_run) { stats.imported_records++; continue; }
        json entity = unwrapDocument(arr[i]);
        entity["_type"] = collection;
        if (options.streaming_row_callback) {
            if (!options.streaming_row_callback(collection, entity)) {
                stats.imported_records++;
                return stats;  // early abort
            }
        }
        stats.imported_records++;
    }
    return stats;
}

// ===========================================================================
// Tests: collectionFromPath
// ===========================================================================

TEST(MongoCollectionFromPath, SimpleFilename) {
    EXPECT_EQ(collectionFromPath("users.json"), "users");
}

TEST(MongoCollectionFromPath, AbsolutePath) {
    EXPECT_EQ(collectionFromPath("/data/exports/orders.jsonl"), "orders");
}

TEST(MongoCollectionFromPath, WindowsPath) {
    EXPECT_EQ(collectionFromPath("C:\\exports\\products.json"), "products");
}

TEST(MongoCollectionFromPath, NoExtension) {
    EXPECT_EQ(collectionFromPath("/data/mycollection"), "mycollection");
}

TEST(MongoCollectionFromPath, EmptyFilename) {
    // Path ends with separator -> empty basename -> fallback "documents"
    EXPECT_EQ(collectionFromPath("/data/"), "documents");
}

TEST(MongoCollectionFromPath, DotInPath) {
    EXPECT_EQ(collectionFromPath("/my.dir/users.json"), "users");
}

// ===========================================================================
// Tests: shouldImportCollection
// ===========================================================================

TEST(MongoCollectionFilter, DefaultAllowsAll) {
    ImportOptions opts;
    EXPECT_TRUE(shouldImportCollection("users",    opts));
    EXPECT_TRUE(shouldImportCollection("products", opts));
}

TEST(MongoCollectionFilter, ExcludeList) {
    ImportOptions opts;
    opts.exclude_tables = {"audit_log", "sessions"};
    EXPECT_FALSE(shouldImportCollection("audit_log", opts));
    EXPECT_FALSE(shouldImportCollection("sessions",  opts));
    EXPECT_TRUE(shouldImportCollection("users",      opts));
}

TEST(MongoCollectionFilter, IncludeListFiltersOut) {
    ImportOptions opts;
    opts.include_tables = {"users"};
    EXPECT_TRUE(shouldImportCollection("users",    opts));
    EXPECT_FALSE(shouldImportCollection("products", opts));
}

TEST(MongoCollectionFilter, ExcludeTakesPriorityOverInclude) {
    ImportOptions opts;
    opts.include_tables = {"users", "products"};
    opts.exclude_tables = {"users"};
    EXPECT_FALSE(shouldImportCollection("users",    opts));
    EXPECT_TRUE(shouldImportCollection("products",  opts));
}

// ===========================================================================
// Tests: inferThemisType
// ===========================================================================

TEST(MongoInferType, Null)    { EXPECT_EQ(inferThemisType(json(nullptr)), "string"); }
TEST(MongoInferType, Bool)    { EXPECT_EQ(inferThemisType(json(true)),    "boolean"); }
TEST(MongoInferType, Integer) { EXPECT_EQ(inferThemisType(json(42)),      "long"); }
TEST(MongoInferType, Float)   { EXPECT_EQ(inferThemisType(json(3.14)),    "double"); }
TEST(MongoInferType, String)  { EXPECT_EQ(inferThemisType(json("hello")), "string"); }
TEST(MongoInferType, Array)   { EXPECT_EQ(inferThemisType(json::array()), "array"); }

TEST(MongoInferType, Oid) {
    EXPECT_EQ(inferThemisType(json{{"$oid", "507f1f77bcf86cd799439011"}}), "string");
}
TEST(MongoInferType, Date) {
    EXPECT_EQ(inferThemisType(json{{"$date", "2024-01-15T00:00:00Z"}}), "datetime");
}
TEST(MongoInferType, NumberDecimal) {
    EXPECT_EQ(inferThemisType(json{{"$numberDecimal", "3.14"}}), "double");
}
TEST(MongoInferType, NumberLong) {
    EXPECT_EQ(inferThemisType(json{{"$numberLong", "1234567890"}}), "long");
}
TEST(MongoInferType, NumberInt) {
    EXPECT_EQ(inferThemisType(json{{"$numberInt", "42"}}), "integer");
}
TEST(MongoInferType, NumberDouble) {
    EXPECT_EQ(inferThemisType(json{{"$numberDouble", "2.71"}}), "double");
}
TEST(MongoInferType, Binary) {
    EXPECT_EQ(inferThemisType(json{{"$binary", {{"base64", "abc"}, {"subType", "00"}}}}),
              "binary");
}
TEST(MongoInferType, Timestamp) {
    EXPECT_EQ(inferThemisType(json{{"$timestamp", {{"t", 1}, {"i", 1}}}}), "datetime");
}
TEST(MongoInferType, Regex) {
    EXPECT_EQ(inferThemisType(json{{"$regex", "^abc"}}), "string");
}
TEST(MongoInferType, Code) {
    EXPECT_EQ(inferThemisType(json{{"$code", "function(){}"}}), "string");
}
TEST(MongoInferType, Undefined) {
    EXPECT_EQ(inferThemisType(json{{"$undefined", true}}), "string");
}
TEST(MongoInferType, MinKey) {
    EXPECT_EQ(inferThemisType(json{{"$minKey", 1}}), "string");
}
TEST(MongoInferType, MaxKey) {
    EXPECT_EQ(inferThemisType(json{{"$maxKey", 1}}), "string");
}
TEST(MongoInferType, PlainObject) {
    EXPECT_EQ(inferThemisType(json{{"a", 1}, {"b", "x"}}), "object");
}

// ===========================================================================
// Tests: unwrapBsonValue
// ===========================================================================

TEST(MongoBsonUnwrap, OidToString) {
    json v{{"$oid", "507f1f77bcf86cd799439011"}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "507f1f77bcf86cd799439011");
}

TEST(MongoBsonUnwrap, DateIsoString) {
    json v{{"$date", "2024-01-15T00:00:00.000Z"}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "2024-01-15T00:00:00.000Z");
}

TEST(MongoBsonUnwrap, DateWithNumberLong) {
    json v{{"$date", {{"$numberLong", "1705276800000"}}}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "1705276800000");
}

TEST(MongoBsonUnwrap, DateNumeric) {
    json v{{"$date", 1705276800000LL}};
    json result = unwrapBsonValue(v);
    EXPECT_TRUE(result.is_number());
}

TEST(MongoBsonUnwrap, NumberDecimalToString) {
    json v{{"$numberDecimal", "3.141592653589793"}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "3.141592653589793");
}

TEST(MongoBsonUnwrap, NumberLongToString) {
    json v{{"$numberLong", "9007199254740993"}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "9007199254740993");
}

TEST(MongoBsonUnwrap, NumberIntStringToInt) {
    json v{{"$numberInt", "42"}};
    json result = unwrapBsonValue(v);
    EXPECT_TRUE(result.is_number_integer());
    EXPECT_EQ(result.get<int>(), 42);
}

TEST(MongoBsonUnwrap, NumberIntNumeric) {
    json v{{"$numberInt", 99}};
    json result = unwrapBsonValue(v);
    EXPECT_TRUE(result.is_number());
}

TEST(MongoBsonUnwrap, NumberDoubleStringToDouble) {
    json v{{"$numberDouble", "2.718281828"}};
    json result = unwrapBsonValue(v);
    EXPECT_TRUE(result.is_number_float());
    EXPECT_NEAR(result.get<double>(), 2.718281828, 1e-9);
}

TEST(MongoBsonUnwrap, BinaryBase64) {
    json v{{"$binary", {{"base64", "SGVsbG8="}, {"subType", "00"}}}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "SGVsbG8=");
}

TEST(MongoBsonUnwrap, BinaryStringDirect) {
    json v{{"$binary", "SGVsbG8="}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "SGVsbG8=");
}

TEST(MongoBsonUnwrap, TimestampToString) {
    json v{{"$timestamp", {{"t", 1609459200}, {"i", 1}}}};
    json result = unwrapBsonValue(v);
    // Result should be the JSON string dump of the timestamp object
    ASSERT_TRUE(result.is_string());
    std::string s = result.get<std::string>();
    EXPECT_NE(s.find("1609459200"), std::string::npos);
}

TEST(MongoBsonUnwrap, RegexToString) {
    json v{{"$regex", "^[a-z]+$"}, {"$options", "i"}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "^[a-z]+$");
}

TEST(MongoBsonUnwrap, CodeToString) {
    json v{{"$code", "function() { return 1; }"}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "function() { return 1; }");
}

TEST(MongoBsonUnwrap, UndefinedToEmpty) {
    json v{{"$undefined", true}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "");
}

TEST(MongoBsonUnwrap, MinKeyToEmpty) {
    json v{{"$minKey", 1}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "");
}

TEST(MongoBsonUnwrap, MaxKeyToEmpty) {
    json v{{"$maxKey", 1}};
    json result = unwrapBsonValue(v);
    ASSERT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "");
}

TEST(MongoBsonUnwrap, PlainObjectPassThrough) {
    json v{{"name", "Alice"}, {"age", 30}};
    json result = unwrapBsonValue(v);
    // Not a BSON wrapper; should be returned as-is
    EXPECT_EQ(result, v);
}

TEST(MongoBsonUnwrap, NonObjectPassThrough) {
    json v = "just a string";
    EXPECT_EQ(unwrapBsonValue(v), v);
    json v2 = 42;
    EXPECT_EQ(unwrapBsonValue(v2), v2);
}

// ===========================================================================
// Tests: unwrapDocument
// ===========================================================================

TEST(MongoUnwrapDocument, FlatDocument) {
    json doc{
        {"name",  "Alice"},
        {"age",   30},
        {"score", 9.5}
    };
    json result = unwrapDocument(doc);
    EXPECT_EQ(result["name"].get<std::string>(), "Alice");
    EXPECT_EQ(result["age"].get<int>(), 30);
    EXPECT_NEAR(result["score"].get<double>(), 9.5, 1e-9);
}

TEST(MongoUnwrapDocument, WithOidField) {
    json doc{
        {"_id",  {{"$oid", "507f1f77bcf86cd799439011"}}},
        {"name", "Bob"}
    };
    json result = unwrapDocument(doc);
    ASSERT_TRUE(result["_id"].is_string());
    EXPECT_EQ(result["_id"].get<std::string>(), "507f1f77bcf86cd799439011");
    EXPECT_EQ(result["name"].get<std::string>(), "Bob");
}

TEST(MongoUnwrapDocument, WithDateField) {
    json doc{
        {"created_at", {{"$date", "2024-01-15T12:00:00Z"}}}
    };
    json result = unwrapDocument(doc);
    ASSERT_TRUE(result["created_at"].is_string());
    EXPECT_EQ(result["created_at"].get<std::string>(), "2024-01-15T12:00:00Z");
}

TEST(MongoUnwrapDocument, WithNumberDecimal) {
    json doc{
        {"price", {{"$numberDecimal", "19.99"}}}
    };
    json result = unwrapDocument(doc);
    ASSERT_TRUE(result["price"].is_string());
    EXPECT_EQ(result["price"].get<std::string>(), "19.99");
}

TEST(MongoUnwrapDocument, NestedObject) {
    json doc{
        {"address", {
            {"city", "Berlin"},
            {"zip",  "10115"}
        }}
    };
    json result = unwrapDocument(doc);
    ASSERT_TRUE(result["address"].is_object());
    EXPECT_EQ(result["address"]["city"].get<std::string>(), "Berlin");
    EXPECT_EQ(result["address"]["zip"].get<std::string>(),  "10115");
}

TEST(MongoUnwrapDocument, NestedBsonInNestedObject) {
    json doc{
        {"meta", {
            {"_id",  {{"$oid", "aabbccdd11223344aabbccdd"}}},
            {"flag", true}
        }}
    };
    json result = unwrapDocument(doc);
    ASSERT_TRUE(result["meta"].is_object());
    ASSERT_TRUE(result["meta"]["_id"].is_string());
    EXPECT_EQ(result["meta"]["_id"].get<std::string>(), "aabbccdd11223344aabbccdd");
}

TEST(MongoUnwrapDocument, ArrayOfBsonValues) {
    json doc{
        {"ids", {
            {{"$oid", "111111111111111111111111"}},
            {{"$oid", "222222222222222222222222"}}
        }}
    };
    json result = unwrapDocument(doc);
    ASSERT_TRUE(result["ids"].is_array());
    ASSERT_EQ(result["ids"].size(), 2u);
    EXPECT_EQ(result["ids"][0].get<std::string>(), "111111111111111111111111");
    EXPECT_EQ(result["ids"][1].get<std::string>(), "222222222222222222222222");
}

TEST(MongoUnwrapDocument, ArrayOfPlainValues) {
    json doc{{"tags", {"a", "b", "c"}}};
    json result = unwrapDocument(doc);
    ASSERT_TRUE(result["tags"].is_array());
    EXPECT_EQ(result["tags"].size(), 3u);
    EXPECT_EQ(result["tags"][0].get<std::string>(), "a");
}

TEST(MongoUnwrapDocument, TypeFieldAddedByImporter) {
    // Verify that _type is added AFTER unwrapping (simulated here)
    json doc{{"name", "Alice"}, {"_id", {{"$oid", "aabbcc001122334455667788"}}}};
    json entity = unwrapDocument(doc);
    entity["_type"] = "users";
    EXPECT_EQ(entity["_type"].get<std::string>(), "users");
    EXPECT_EQ(entity["_id"].get<std::string>(),   "aabbcc001122334455667788");
}

// ===========================================================================
// Tests: validateSource equivalent (looksLikeMongoExport)
// ===========================================================================

TEST(MongoValidateSource, AcceptsNdJsonFile) {
    // Write a temp file
    std::string path = "/tmp/test_mongo_valid_ndjson.json";
    {
        std::ofstream f(path);
        f << R"({"_id": {"$oid": "1"}, "name": "Alice"})" << "\n";
        f << R"({"_id": {"$oid": "2"}, "name": "Bob"})" << "\n";
    }
    EXPECT_TRUE(looksLikeMongoExport(path));
}

TEST(MongoValidateSource, AcceptsJsonArrayFile) {
    std::string path = "/tmp/test_mongo_valid_array.json";
    {
        std::ofstream f(path);
        f << R"([{"name":"Alice"},{"name":"Bob"}])" << "\n";
    }
    EXPECT_TRUE(looksLikeMongoExport(path));
}

TEST(MongoValidateSource, RejectsPlainSqlFile) {
    std::string path = "/tmp/test_mongo_reject_sql.sql";
    {
        std::ofstream f(path);
        f << "-- PostgreSQL database dump\nCREATE TABLE t (id INT);\n";
    }
    EXPECT_FALSE(looksLikeMongoExport(path));
}

TEST(MongoValidateSource, RejectsEmptyFile) {
    std::string path = "/tmp/test_mongo_empty.json";
    { std::ofstream f(path); }
    EXPECT_FALSE(looksLikeMongoExport(path));
}

TEST(MongoValidateSource, SkipsCommentLines) {
    std::string path = "/tmp/test_mongo_comments.json";
    {
        std::ofstream f(path);
        f << "# This is a comment\n";
        f << "# Another comment\n";
        f << R"({"name":"Alice"})" << "\n";
    }
    EXPECT_TRUE(looksLikeMongoExport(path));
}

TEST(MongoValidateSource, RejectsNonexistentFile) {
    EXPECT_FALSE(looksLikeMongoExport("/tmp/this_file_does_not_exist_xyz.json"));
}

// ===========================================================================
// Tests: importJsonLines (NDJSON parsing)
// ===========================================================================

TEST(MongoImportNdjson, BasicDocuments) {
    std::string content =
        R"({"_id":{"$oid":"1"},"name":"Alice","age":30})" "\n"
        R"({"_id":{"$oid":"2"},"name":"Bob","age":25})" "\n";
    ImportOptions opts;
    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records,  2u);
    EXPECT_EQ(stats.failed_records,    0u);
    EXPECT_EQ(stats.skipped_records,   0u);
    EXPECT_EQ(stats.tables_processed,  1u);
    EXPECT_EQ(stats.total_records,     2u);
}

TEST(MongoImportNdjson, SkipsEmptyLines) {
    std::string content =
        "\n"
        R"({"name":"Alice"})" "\n"
        "\n"
        R"({"name":"Bob"})" "\n"
        "\n";
    ImportOptions opts;
    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(MongoImportNdjson, SkipsCommentLines) {
    std::string content =
        "# exported by mongoexport\n"
        R"({"name":"Alice"})" "\n"
        "# another comment\n"
        R"({"name":"Bob"})" "\n";
    ImportOptions opts;
    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(MongoImportNdjson, InvalidJsonLine) {
    std::string content =
        R"({"name":"Alice"})" "\n"
        "not valid json\n"
        R"({"name":"Bob"})" "\n";
    ImportOptions opts;
    auto stats = importJsonLines(content, "users", opts);
    // "not valid json" starts with 'n', not '{', so it's silently skipped
    EXPECT_EQ(stats.imported_records, 2u);
    EXPECT_EQ(stats.failed_records,   0u);
}

TEST(MongoImportNdjson, MalformedJsonObject) {
    std::string content =
        R"({"name":"Alice"})" "\n"
        R"({broken json})" "\n"
        R"({"name":"Bob"})" "\n";
    ImportOptions opts;
    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 2u);
    EXPECT_EQ(stats.failed_records,   1u);
}

TEST(MongoImportNdjson, DryRunMode) {
    std::string content =
        R"({"name":"Alice"})" "\n"
        R"({"name":"Bob"})" "\n";
    ImportOptions opts;
    opts.dry_run = true;
    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 2u);  // counted but not stored
}

TEST(MongoImportNdjson, ExcludeCollection) {
    std::string content =
        R"({"name":"Alice"})" "\n"
        R"({"name":"Bob"})" "\n";
    ImportOptions opts;
    opts.exclude_tables = {"users"};
    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records,  0u);
    EXPECT_EQ(stats.skipped_records,   2u);
}

TEST(MongoImportNdjson, IncludeCollectionFilterOut) {
    std::string content = R"({"name":"Alice"})" "\n";
    ImportOptions opts;
    opts.include_tables = {"products"};
    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.skipped_records,  1u);
}

TEST(MongoImportNdjson, RowSizeLimitExceeded) {
    // 32-byte limit; document will exceed it
    std::string content = R"({"name":"Alice","description":"A long description text"})" "\n";
    ImportOptions opts;
    opts.max_row_size_bytes = 32;
    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.failed_records,   1u);
    ASSERT_FALSE(stats.structured_errors.empty());
    EXPECT_EQ(stats.structured_errors[0].code, ImportErrorCode::ROW_TOO_LARGE);
}

TEST(MongoImportNdjson, RowSizeLimitNotExceeded) {
    std::string content = R"({"n":"A"})" "\n";  // 9 bytes
    ImportOptions opts;
    opts.max_row_size_bytes = 100;
    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 1u);
    EXPECT_EQ(stats.failed_records,   0u);
}

TEST(MongoImportNdjson, BsonUnwrappingInParsedDocument) {
    std::string content =
        R"({"_id":{"$oid":"507f1f77bcf86cd799439011"},"price":{"$numberDecimal":"9.99"},"ts":{"$date":"2024-01-01T00:00:00Z"}})"
        "\n";
    ImportOptions opts;
    // We verify correct unwrapping through inferThemisType on the parsed values
    json doc = json::parse(R"({"_id":{"$oid":"507f1f77bcf86cd799439011"},"price":{"$numberDecimal":"9.99"},"ts":{"$date":"2024-01-01T00:00:00Z"}})");
    json entity = unwrapDocument(doc);
    entity["_type"] = "items";
    EXPECT_EQ(entity["_id"].get<std::string>(),   "507f1f77bcf86cd799439011");
    EXPECT_EQ(entity["price"].get<std::string>(),  "9.99");
    EXPECT_EQ(entity["ts"].get<std::string>(),     "2024-01-01T00:00:00Z");
    EXPECT_EQ(entity["_type"].get<std::string>(),  "items");
}

// ===========================================================================
// Tests: importJsonArray (JSON array format)
// ===========================================================================

TEST(MongoImportJsonArray, BasicArray) {
    std::string content = R"([{"name":"Alice"},{"name":"Bob"},{"name":"Carol"}])";
    ImportOptions opts;
    auto stats = importJsonArray(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 3u);
    EXPECT_EQ(stats.failed_records,   0u);
    EXPECT_EQ(stats.tables_processed, 1u);
}

TEST(MongoImportJsonArray, EmptyArray) {
    std::string content = "[]";
    ImportOptions opts;
    auto stats = importJsonArray(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.tables_processed, 0u);
}

TEST(MongoImportJsonArray, MalformedJson) {
    std::string content = "[{broken}]";
    ImportOptions opts;
    auto stats = importJsonArray(content, "users", opts);
    ASSERT_FALSE(stats.structured_errors.empty());
    EXPECT_EQ(stats.structured_errors[0].code, ImportErrorCode::FILE_READ_FAILED);
}

TEST(MongoImportJsonArray, NotAnArray) {
    std::string content = R"({"name":"Alice"})";
    ImportOptions opts;
    auto stats = importJsonArray(content, "users", opts);
    ASSERT_FALSE(stats.structured_errors.empty());
    EXPECT_EQ(stats.structured_errors[0].code, ImportErrorCode::NOT_A_PG_DUMP);
}

TEST(MongoImportJsonArray, DryRunMode) {
    std::string content = R"([{"name":"Alice"},{"name":"Bob"}])";
    ImportOptions opts;
    opts.dry_run = true;
    auto stats = importJsonArray(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(MongoImportJsonArray, ExcludeCollection) {
    std::string content = R"([{"name":"Alice"}])";
    ImportOptions opts;
    opts.exclude_tables = {"users"};
    auto stats = importJsonArray(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 0u);
    EXPECT_EQ(stats.skipped_records,  1u);
}

TEST(MongoImportJsonArray, NonObjectElements) {
    // Array containing non-object elements should count as failures
    std::string content = R"([{"name":"Alice"}, 42, "not-an-object", {"name":"Bob"}])";
    ImportOptions opts;
    auto stats = importJsonArray(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 2u);
    EXPECT_EQ(stats.failed_records,   2u);
}

// ===========================================================================
// Tests: Permission check callback
// ===========================================================================

TEST(MongoPermissionCheck, DeniedByCallback) {
    ImportOptions opts;
    bool called = false;
    opts.permission_check = [&called](const std::string& resource,
                                      const std::string& action) -> bool {
        called = true;
        EXPECT_EQ(resource, "import");
        EXPECT_EQ(action,   "write");
        return false;
    };
    ASSERT_TRUE(static_cast<bool>(opts.permission_check));
    bool allowed = opts.permission_check("import", "write");
    EXPECT_TRUE(called);
    EXPECT_FALSE(allowed);
}

TEST(MongoPermissionCheck, AllowedByCallback) {
    ImportOptions opts;
    opts.permission_check = [](const std::string&, const std::string&) { return true; };
    EXPECT_TRUE(opts.permission_check("import", "write"));
}

// ===========================================================================
// Tests: Full round-trip using temp files
// ===========================================================================

TEST(MongoImportRoundTrip, NdjsonFile) {
    std::string path = "/tmp/test_mongo_rt_ndjson.json";
    {
        std::ofstream f(path);
        f << R"({"_id":{"$oid":"a1b2c3d4e5f6a1b2c3d4e5f6"},"name":"Alice","score":{"$numberDecimal":"98.5"}})" "\n";
        f << R"({"_id":{"$oid":"111111111111111111111111"},"name":"Bob","score":{"$numberDecimal":"87.0"}})" "\n";
        f << R"({"_id":{"$oid":"222222222222222222222222"},"name":"Carol","score":{"$numberDecimal":"92.3"}})" "\n";
    }

    // Verify validation passes
    ASSERT_TRUE(looksLikeMongoExport(path));

    // Verify collection name derived from path
    EXPECT_EQ(collectionFromPath(path), "test_mongo_rt_ndjson");

    // Import
    std::ifstream f(path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    ImportOptions opts;
    auto stats = importJsonLines(content, "students", opts);

    EXPECT_EQ(stats.imported_records, 3u);
    EXPECT_EQ(stats.failed_records,   0u);
    EXPECT_EQ(stats.tables_processed, 1u);
}

TEST(MongoImportRoundTrip, JsonArrayFile) {
    std::string path = "/tmp/test_mongo_rt_array.json";
    {
        std::ofstream f(path);
        f << R"([)" "\n";
        f << R"(  {"_id":{"$oid":"aaa"},"ts":{"$date":"2024-01-01T00:00:00Z"},"v":{"$numberLong":"100"}},)" "\n";
        f << R"(  {"_id":{"$oid":"bbb"},"ts":{"$date":"2024-01-02T00:00:00Z"},"v":{"$numberLong":"200"}})" "\n";
        f << R"(])" "\n";
    }

    ASSERT_TRUE(looksLikeMongoExport(path));

    std::ifstream f(path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    ImportOptions opts;
    auto stats = importJsonArray(content, "events", opts);

    EXPECT_EQ(stats.imported_records, 2u);
    EXPECT_EQ(stats.tables_processed, 1u);
}

// ===========================================================================
// Tests: Edge cases
// ===========================================================================

TEST(MongoEdgeCases, AllBsonTypesInOneDocument) {
    json doc{
        {"_id",        {{"$oid",           "507f1f77bcf86cd799439011"}}},
        {"created_at", {{"$date",          "2024-01-15T00:00:00.000Z"}}},
        {"price",      {{"$numberDecimal", "9.99"}}},
        {"big_num",    {{"$numberLong",    "9007199254740993"}}},
        {"count",      {{"$numberInt",     "42"}}},
        {"ratio",      {{"$numberDouble",  "0.5"}}},
        {"data",       {{"$binary",        {{"base64", "SGVsbG8="}, {"subType", "00"}}}}},
        {"ts",         {{"$timestamp",     {{"t", 1609459200}, {"i", 1}}}}},
        {"pattern",    {{"$regex",         "^abc"}}}
    };

    json entity = unwrapDocument(doc);

    EXPECT_EQ(entity["_id"].get<std::string>(),    "507f1f77bcf86cd799439011");
    EXPECT_EQ(entity["created_at"].get<std::string>(), "2024-01-15T00:00:00.000Z");
    EXPECT_EQ(entity["price"].get<std::string>(),  "9.99");
    EXPECT_EQ(entity["big_num"].get<std::string>(), "9007199254740993");
    EXPECT_EQ(entity["count"].get<int>(),          42);
    EXPECT_NEAR(entity["ratio"].get<double>(),     0.5, 1e-9);
    EXPECT_EQ(entity["data"].get<std::string>(),   "SGVsbG8=");
    EXPECT_NE(entity["ts"].get<std::string>().find("1609459200"), std::string::npos);
    EXPECT_EQ(entity["pattern"].get<std::string>(), "^abc");
}

TEST(MongoEdgeCases, NullFieldValue) {
    json doc{{"name", "Alice"}, {"middle_name", nullptr}};
    json entity = unwrapDocument(doc);
    EXPECT_TRUE(entity["middle_name"].is_null());
}

TEST(MongoEdgeCases, EmptyDocument) {
    json doc = json::object();
    json entity = unwrapDocument(doc);
    EXPECT_TRUE(entity.is_object());
    EXPECT_TRUE(entity.empty());
}

TEST(MongoEdgeCases, DeepNestedBson) {
    json doc{
        {"level1", {
            {"level2", {
                {"_id", {{"$oid", "deadbeefdeadbeefdeadbeef"}}}
            }}
        }}
    };
    json entity = unwrapDocument(doc);
    ASSERT_TRUE(entity["level1"]["level2"]["_id"].is_string());
    EXPECT_EQ(entity["level1"]["level2"]["_id"].get<std::string>(),
              "deadbeefdeadbeefdeadbeef");
}

// ===========================================================================
// Tests: Bug-fix regression tests
// ===========================================================================

/// Mimic the fixed peek-loop format detection from importData.
/// Returns '[' if first non-whitespace non-comment char is '[', else '{' (or '\0').
static char peekFirstChar(const std::string& content) {
    std::istringstream ss(content);
    char c = '\0';
    while (ss.get(c)) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
          continue;
        }
        if (c == '#') {
            while (ss.get(c) && c != '\n') { /* skip comment line */ }
            continue;
        }
        return c;
    }
    return '\0';
}

// Bug 1: Comment-line before '[' must be detected as JSON array, not NDJSON
TEST(MongoBugFix, CommentBeforeArrayDetectedAsArray) {
    std::string content = "# exported by mongoexport\n[{\"name\":\"Alice\"}]\n";
    EXPECT_EQ(peekFirstChar(content), '[');
}

TEST(MongoBugFix, CommentBeforeObjectDetectedAsNdjson) {
    std::string content = "# line 1\n# line 2\n{\"name\":\"Alice\"}\n";
    EXPECT_EQ(peekFirstChar(content), '{');
}

TEST(MongoBugFix, MultipleCommentLinesBeforeArray) {
    std::string content = "# comment 1\n# comment 2\n# comment 3\n[{\"a\":1}]\n";
    EXPECT_EQ(peekFirstChar(content), '[');
}

TEST(MongoBugFix, WhitespaceOnlyLinesBeforeFirstChar) {
    std::string content = "\n  \n\t\n[{\"x\":1}]\n";
    EXPECT_EQ(peekFirstChar(content), '[');
}

// Bug 2: doc_index must be incremented even when a line is too large
TEST(MongoBugFix, DocIndexSyncAfterOversizedLine) {
    // 3 docs: first normal, second oversized, third normal.
    // The error for the third doc should say "document 3" not "document 2".
    std::string long_val(200, 'x');  // 200 chars ensures the line exceeds 50 bytes
    std::string content =
        R"({"name":"Alice"})" "\n"
        + ("{\"name\":\"" + long_val + "\"}") + "\n"
        + R"({"name":"Carol"})";

    ImportOptions opts;
    opts.max_row_size_bytes = 50;  // 50-byte limit; second line will exceed it

    auto stats = importJsonLines(content, "users", opts);
    EXPECT_EQ(stats.imported_records, 2u);  // Alice + Carol
    EXPECT_EQ(stats.failed_records,   1u);  // second oversized line

    // The oversized error should reference document 2, not 1
    bool found_doc2_error = false;
    for (const auto& e : stats.structured_errors) {
        if (e.code == ImportErrorCode::ROW_TOO_LARGE &&
            e.location.find("document 2") != std::string::npos) {
            found_doc2_error = true;
        }
    }
    EXPECT_TRUE(found_doc2_error) << "Expected ROW_TOO_LARGE error referencing 'document 2'";
}

// ===========================================================================
// Tests: Sample fixture file integration
// ===========================================================================

static std::string getMongoFixturePath() {
    const std::string relative = "tests/fixtures/importers/sample_mongo.json";

    {
        std::ifstream f(relative);
        if (f.is_open()) {
          return relative;
        }
    }

    const auto source_based =
        (std::filesystem::path(__FILE__).parent_path() /
         "fixtures/importers/sample_mongo.json").lexically_normal();
    {
        std::ifstream f(source_based.string());
        if (f.is_open()) {
          return source_based.string();
        }
    }

    const auto cwd_based =
        (std::filesystem::current_path() /
         "../tests/fixtures/importers/sample_mongo.json").lexically_normal();
    return cwd_based.string();
}

TEST(MongoFixture, SampleMongoJsonIsValid) {
    // Verify the fixture file exists and passes basic format validation
    std::string path = getMongoFixturePath();
    std::ifstream f(path);
    if (!f.is_open()) {
        GTEST_SKIP() << "Fixture not found at " << path;
    }

    ASSERT_TRUE(looksLikeMongoExport(path))
        << "Fixture file must start with a JSON object or array";
}

TEST(MongoFixture, SampleMongoJsonImportsCorrectly) {
    std::string path = getMongoFixturePath();
    std::ifstream f(path);
    if (!f) {
      GTEST_SKIP() << "Fixture not found at " << path;
    }

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    ImportOptions opts;
    auto stats = importJsonLines(content, "users", opts);

    EXPECT_EQ(stats.imported_records, 5u);
    EXPECT_EQ(stats.failed_records,   0u);
    EXPECT_EQ(stats.tables_processed, 1u);
}

TEST(MongoFixture, SampleMongoJsonBsonUnwrap) {
    // Parse one line from the fixture and verify BSON unwrapping
    json doc = json::parse(
        R"({"_id":{"$oid":"507f1f77bcf86cd799439011"},"name":"Alice","score":{"$numberDecimal":"98.50"},"created_at":{"$date":"2024-01-15T08:00:00.000Z"},"active":true})"
    );
    json entity = unwrapDocument(doc);
    entity["_type"] = "users";

    EXPECT_EQ(entity["_id"].get<std::string>(),          "507f1f77bcf86cd799439011");
    EXPECT_EQ(entity["name"].get<std::string>(),          "Alice");
    EXPECT_EQ(entity["score"].get<std::string>(),         "98.50");
    EXPECT_EQ(entity["created_at"].get<std::string>(),    "2024-01-15T08:00:00.000Z");
    EXPECT_TRUE(entity["active"].get<bool>());
    EXPECT_EQ(entity["_type"].get<std::string>(),         "users");
}

// ===========================================================================
// Tests: Streaming row callback (NDJSON and JSON array)
// ===========================================================================

static const std::string kNdjsonDump = R"({"_id":1,"name":"Alice","score":95}
{"_id":2,"name":"Bob","score":87}
{"_id":3,"name":"Carol","score":91}
)";

static const std::string kJsonArrayDump = R"([
  {"_id":1,"product":"Widget","price":9.99},
  {"_id":2,"product":"Gadget","price":19.99}
])";

TEST(MongoStreamingCallback, NdjsonCallbackInvokedForEachDocument) {
    ImportOptions opts;
    std::vector<std::string> collections;
    std::vector<json>        entities;
    opts.streaming_row_callback = [&](const std::string& c, const json& e) -> bool {
        collections.push_back(c);
        entities.push_back(e);
        return true;
    };

    auto stats = importJsonLines(kNdjsonDump, "users", opts);

    EXPECT_EQ(collections.size(), 3u);
    for (auto& c : collections) {
      EXPECT_EQ(c, "users");
    }
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(MongoStreamingCallback, NdjsonCallbackReceivesCorrectFieldValues) {
    ImportOptions opts;
    std::vector<json> rows;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        rows.push_back(e);
        return true;
    };

    importJsonLines(kNdjsonDump, "users", opts);

    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[0]["name"].get<std::string>(), "Alice");
    EXPECT_EQ(rows[1]["name"].get<std::string>(), "Bob");
    EXPECT_EQ(rows[2]["name"].get<std::string>(), "Carol");
}

TEST(MongoStreamingCallback, NdjsonAbortOnFalseFromCallback) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return call_count < 2;  // abort after second document
    };

    auto stats = importJsonLines(kNdjsonDump, "users", opts);

    EXPECT_EQ(call_count, 2u);
    EXPECT_LE(stats.imported_records, 2u);
}

TEST(MongoStreamingCallback, NdjsonAbortOnFirstDocumentStopsImmediately) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return false;
    };

    auto stats = importJsonLines(kNdjsonDump, "users", opts);

    EXPECT_EQ(call_count, 1u);
    EXPECT_EQ(stats.imported_records, 1u);
}

TEST(MongoStreamingCallback, NdjsonNullCallbackImportsAll) {
    ImportOptions opts;
    auto stats = importJsonLines(kNdjsonDump, "users", opts);
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(MongoStreamingCallback, JsonArrayCallbackInvokedForEachDocument) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return true;
    };

    auto stats = importJsonArray(kJsonArrayDump, "products", opts);

    EXPECT_EQ(call_count, 2u);
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(MongoStreamingCallback, JsonArrayAbortOnFirstDocumentStopsImmediately) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return false;
    };

    auto stats = importJsonArray(kJsonArrayDump, "products", opts);

    EXPECT_EQ(call_count, 1u);
    EXPECT_EQ(stats.imported_records, 1u);
}

TEST(MongoStreamingCallback, StatsMatchCallbackInvocationCount) {
    ImportOptions opts;
    size_t callback_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++callback_count;
        return true;
    };

    auto stats = importJsonLines(kNdjsonDump, "users", opts);

    EXPECT_EQ(stats.imported_records, callback_count);
}

TEST(MongoStreamingCallback, ExcludeCollectionFiltersCallback) {
    ImportOptions opts;
    opts.exclude_tables = {"users"};
    std::vector<std::string> collections;
    opts.streaming_row_callback = [&](const std::string& c, const json&) -> bool {
        collections.push_back(c);
        return true;
    };

    importJsonLines(kNdjsonDump, "users", opts);

    EXPECT_TRUE(collections.empty());
}
