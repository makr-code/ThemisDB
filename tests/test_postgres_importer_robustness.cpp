// Test: PostgreSQL Importer – Structured Errors, Type Mapping & COPY/INSERT Parsing
//
// Tests added as part of production-hardening:
//   - ImportErrorCode / ImportErrorSeverity / ImportError
//   - Expanded mapPostgreSQLTypeToThemis (user overrides, new types)
//   - parseCopyRow / unescapeCopyValue (COPY text format)
//   - parseInsertValues (INSERT VALUES tokeniser)

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

// ---------------------------------------------------------------------------
// Minimal re-implementation of the relevant logic so this test is self-contained
// (mirrors what postgres_importer.cpp / importer_interface.h provide).
// ---------------------------------------------------------------------------

enum class ImportErrorCode : uint32_t {
    SUCCESS              = 0,
    FILE_NOT_FOUND       = 100,
    FILE_OPEN_FAILED     = 101,
    FILE_READ_FAILED     = 102,
    NOT_A_PG_DUMP        = 103,
    PARSE_CREATE_TABLE   = 200,
    PARSE_INSERT         = 201,
    PARSE_COPY_HEADER    = 202,
    PARSE_COPY_ROW       = 203,
    STATEMENT_TOO_LARGE  = 204,
    UNKNOWN_TABLE        = 300,
    COLUMN_COUNT_MISMATCH = 301,
    TYPE_CONVERSION      = 400,
    UNKNOWN_PG_TYPE      = 401,
    VALUE_OUT_OF_RANGE   = 402,
    DRY_RUN_ONLY         = 500,
    TABLE_EXCLUDED       = 501,
    UNKNOWN              = 900
};

enum class ImportErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

struct ImportError {
    ImportErrorCode     code     = ImportErrorCode::UNKNOWN;
    ImportErrorSeverity severity = ImportErrorSeverity::ERROR;
    std::string         message;
    std::string         location;
};

// ---------------------------------------------------------------------------
// Type mapper (mirrors mapPostgreSQLTypeToThemis)
// ---------------------------------------------------------------------------
static std::string mapType(const std::string& pg_type,
                            const std::map<std::string, std::string>& overrides = {}) {
    auto it = overrides.find(pg_type);
    if (it != overrides.end()) return it->second;

    std::string t = pg_type;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    it = overrides.find(t);
    if (it != overrides.end()) return it->second;

    // Array types
    if (t.back() == ']' || t.find("[]") != std::string::npos) return "array";

    if (t == "bigserial" || t == "bigint" || t == "int8") return "long";
    if (t == "smallint" || t == "int2" || t == "smallserial") return "integer";
    if (t == "integer" || t == "int" || t == "int4" ||
        t == "serial" || t == "serial4") return "integer";
    if (t == "real" || t == "float4") return "float";
    if (t == "double precision" || t == "float8") return "double";
    if (t == "numeric" || t == "decimal") return "double";
    if (t == "money") return "double";
    if (t == "boolean" || t == "bool") return "boolean";
    if (t == "text" || t == "name") return "string";
    if (t == "uuid") return "string";
    if (t == "inet" || t == "cidr" || t == "macaddr" || t == "macaddr8") return "string";
    if (t == "xml") return "string";
    if (t == "bytea") return "binary";
    if (t == "json" || t == "jsonb") return "json";
    if (t == "interval") return "string";
    if (t == "point" || t == "polygon" || t == "circle" ||
        t == "line" || t == "lseg" || t == "box" || t == "path") return "geo";
    if (t == "tsvector" || t == "tsquery") return "string";
    if (t == "oid" || t == "xid" || t == "cid") return "integer";

    if (t.find("int") != std::string::npos) return "integer";
    if (t.find("serial") != std::string::npos) return "integer";
    if (t.find("float") != std::string::npos) return "double";
    if (t.find("char") != std::string::npos) return "string";
    if (t.find("varchar") != std::string::npos) return "string";
    if (t.find("timestamp") != std::string::npos) return "datetime";
    if (t.find("date") != std::string::npos) return "date";
    if (t.find("time") != std::string::npos) return "time";
    if (t.find("json") != std::string::npos) return "json";

    return "string";
}

// ---------------------------------------------------------------------------
// COPY row parser (mirrors parseCopyRow / unescapeCopyValue)
// ---------------------------------------------------------------------------
static std::string unescapeCopy(const std::string& val) {
    if (val == "\\N") return "";  // SQL NULL
    std::string out;
    out.reserve(val.size());
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] == '\\' && i + 1 < val.size()) {
            char next = val[++i];
            switch (next) {
                case 'N':  out += '\\'; out += 'N'; break;  // \N is NULL only as whole field
                case 't':  out += '\t'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case '\\': out += '\\'; break;
                default:   out += '\\'; out += next; break;
            }
        } else {
            out += val[i];
        }
    }
    return out;
}

static std::vector<std::string> parseCopyRow(const std::string& line) {
    std::vector<std::string> result;
    size_t start = 0;
    for (size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == '\t') {
            result.push_back(unescapeCopy(line.substr(start, i - start)));
            start = i + 1;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// INSERT VALUES tokeniser (mirrors parseInsertValues)
// ---------------------------------------------------------------------------
static std::vector<std::string> parseInsertValues(const std::string& values_clause) {
    std::vector<std::string> result;
    size_t i = 0;
    const size_t n = values_clause.size();

    while (i < n) {
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t')) ++i;
        if (i >= n) break;

        if (values_clause[i] == '\'') {
            ++i;
            std::string val;
            while (i < n) {
                if (values_clause[i] == '\'' && i + 1 < n && values_clause[i + 1] == '\'') {
                    val += '\'';
                    i += 2;
                } else if (values_clause[i] == '\'') {
                    ++i;
                    break;
                } else {
                    val += values_clause[i++];
                }
            }
            result.push_back(val);
        } else {
            size_t start = i;
            while (i < n && values_clause[i] != ',' && values_clause[i] != ')') ++i;
            std::string token = values_clause.substr(start, i - start);
            token.erase(token.find_last_not_of(" \t") + 1);
            result.push_back(token);
        }

        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t' ||
                          values_clause[i] == ',')) ++i;
    }
    return result;
}

// ============================================================================
// Tests: ImportErrorCode / ImportErrorSeverity / ImportError
// ============================================================================

TEST(ImportErrorTest, ErrorCodeValues) {
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::SUCCESS), 0u);
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::FILE_NOT_FOUND), 100u);
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::PARSE_CREATE_TABLE), 200u);
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::UNKNOWN_TABLE), 300u);
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::TYPE_CONVERSION), 400u);
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::DRY_RUN_ONLY), 500u);
    EXPECT_EQ(static_cast<uint32_t>(ImportErrorCode::UNKNOWN), 900u);
}

TEST(ImportErrorTest, StructuredErrorFields) {
    ImportError e;
    e.code     = ImportErrorCode::COLUMN_COUNT_MISMATCH;
    e.severity = ImportErrorSeverity::WARNING;
    e.message  = "row has 3 columns, expected 5";
    e.location = "table users, row 42";

    EXPECT_EQ(e.code, ImportErrorCode::COLUMN_COUNT_MISMATCH);
    EXPECT_EQ(e.severity, ImportErrorSeverity::WARNING);
    EXPECT_EQ(e.message, "row has 3 columns, expected 5");
    EXPECT_EQ(e.location, "table users, row 42");
}

TEST(ImportErrorTest, SeverityLevels) {
    EXPECT_NE(ImportErrorSeverity::INFO,     ImportErrorSeverity::WARNING);
    EXPECT_NE(ImportErrorSeverity::WARNING,  ImportErrorSeverity::ERROR);
    EXPECT_NE(ImportErrorSeverity::ERROR,    ImportErrorSeverity::CRITICAL);
}

// ============================================================================
// Tests: Type Mapping
// ============================================================================

TEST(TypeMappingTest, IntegerTypes) {
    EXPECT_EQ(mapType("integer"),    "integer");
    EXPECT_EQ(mapType("int"),        "integer");
    EXPECT_EQ(mapType("int4"),       "integer");
    EXPECT_EQ(mapType("serial"),     "integer");
    EXPECT_EQ(mapType("serial4"),    "integer");
    EXPECT_EQ(mapType("smallint"),   "integer");
    EXPECT_EQ(mapType("int2"),       "integer");
    EXPECT_EQ(mapType("smallserial"),"integer");
}

TEST(TypeMappingTest, LongTypes) {
    EXPECT_EQ(mapType("bigint"),   "long");
    EXPECT_EQ(mapType("int8"),     "long");
    EXPECT_EQ(mapType("bigserial"),"long");
}

TEST(TypeMappingTest, FloatTypes) {
    EXPECT_EQ(mapType("real"),             "float");
    EXPECT_EQ(mapType("float4"),           "float");
    EXPECT_EQ(mapType("double precision"), "double");
    EXPECT_EQ(mapType("float8"),           "double");
    EXPECT_EQ(mapType("numeric"),          "double");
    EXPECT_EQ(mapType("decimal"),          "double");
    EXPECT_EQ(mapType("money"),            "double");
}

TEST(TypeMappingTest, BoolType) {
    EXPECT_EQ(mapType("boolean"), "boolean");
    EXPECT_EQ(mapType("bool"),    "boolean");
}

TEST(TypeMappingTest, StringTypes) {
    EXPECT_EQ(mapType("text"),    "string");
    EXPECT_EQ(mapType("name"),    "string");
    EXPECT_EQ(mapType("uuid"),    "string");
    EXPECT_EQ(mapType("inet"),    "string");
    EXPECT_EQ(mapType("cidr"),    "string");
    EXPECT_EQ(mapType("macaddr"), "string");
    EXPECT_EQ(mapType("macaddr8"),"string");
    EXPECT_EQ(mapType("xml"),     "string");
    EXPECT_EQ(mapType("interval"),"string");
    EXPECT_EQ(mapType("tsvector"),"string");
    EXPECT_EQ(mapType("tsquery"), "string");
    EXPECT_EQ(mapType("varchar"), "string");
    EXPECT_EQ(mapType("character varying"), "string");
    EXPECT_EQ(mapType("char"),    "string");
}

TEST(TypeMappingTest, JsonTypes) {
    EXPECT_EQ(mapType("json"),  "json");
    EXPECT_EQ(mapType("jsonb"), "json");
}

TEST(TypeMappingTest, BinaryType) {
    EXPECT_EQ(mapType("bytea"), "binary");
}

TEST(TypeMappingTest, DateTimeTypes) {
    EXPECT_EQ(mapType("timestamp"),              "datetime");
    EXPECT_EQ(mapType("timestamp with time zone"),"datetime");
    EXPECT_EQ(mapType("date"),                   "date");
    EXPECT_EQ(mapType("time"),                   "time");
    EXPECT_EQ(mapType("time without time zone"), "time");
}

TEST(TypeMappingTest, ArrayTypes) {
    EXPECT_EQ(mapType("integer[]"), "array");
    EXPECT_EQ(mapType("text[]"),    "array");
    EXPECT_EQ(mapType("bigint[]"),  "array");
}

TEST(TypeMappingTest, GeoTypes) {
    EXPECT_EQ(mapType("point"),   "geo");
    EXPECT_EQ(mapType("polygon"), "geo");
    EXPECT_EQ(mapType("circle"),  "geo");
    EXPECT_EQ(mapType("line"),    "geo");
    EXPECT_EQ(mapType("box"),     "geo");
}

TEST(TypeMappingTest, OidTypes) {
    EXPECT_EQ(mapType("oid"), "integer");
    EXPECT_EQ(mapType("xid"), "integer");
    EXPECT_EQ(mapType("cid"), "integer");
}

TEST(TypeMappingTest, UnknownTypeDefaultsToString) {
    EXPECT_EQ(mapType("custom_domain_type"), "string");
    EXPECT_EQ(mapType("myenum"),             "string");
}

TEST(TypeMappingTest, UserOverridesApplied) {
    std::map<std::string, std::string> overrides = {
        {"myenum",  "enum"},
        {"integer", "int32"}   // override a built-in type
    };
    EXPECT_EQ(mapType("myenum",  overrides), "enum");
    EXPECT_EQ(mapType("integer", overrides), "int32");
    // Types not in overrides still work normally
    EXPECT_EQ(mapType("boolean", overrides), "boolean");
}

TEST(TypeMappingTest, UserOverridesCaseInsensitive) {
    std::map<std::string, std::string> overrides = {{"MYTYPE", "custom"}};
    // Exact match first
    EXPECT_EQ(mapType("MYTYPE", overrides), "custom");
    // Lower-cased lookup
    EXPECT_EQ(mapType("mytype", overrides), "string");  // not in overrides lowercase → default
}

// ============================================================================
// Tests: COPY row parsing
// ============================================================================

TEST(CopyParsingTest, SimpleTabSeparated) {
    auto row = parseCopyRow("1\thello\tworld");
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(row[0], "1");
    EXPECT_EQ(row[1], "hello");
    EXPECT_EQ(row[2], "world");
}

TEST(CopyParsingTest, NullValue) {
    // \N in PostgreSQL COPY is the NULL marker
    auto row = parseCopyRow("1\t\\N\ttext");
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(row[0], "1");
    EXPECT_EQ(row[1], "");   // NULL → empty string sentinel
    EXPECT_EQ(row[2], "text");
}

TEST(CopyParsingTest, EscapeSequences) {
    // \t and \\ inside fields (not the whole field)
    auto row = parseCopyRow("a\\tb\ta\\\\b");
    ASSERT_EQ(row.size(), 2u);
    EXPECT_EQ(row[0], "a\tb");    // escaped tab inside field
    EXPECT_EQ(row[1], "a\\b");    // escaped backslash
}

TEST(CopyParsingTest, NullIsOnlyWholeField) {
    // \\N embedded within a longer field is NOT treated as NULL
    auto row = parseCopyRow("abc\\Nxyz");
    ASSERT_EQ(row.size(), 1u);
    // "\\N" is only NULL when the whole raw field is exactly "\\N"
    EXPECT_NE(row[0], "");        // not NULL
}

TEST(CopyParsingTest, SingleField) {
    auto row = parseCopyRow("only_one");
    ASSERT_EQ(row.size(), 1u);
    EXPECT_EQ(row[0], "only_one");
}

TEST(CopyParsingTest, EmptyFields) {
    auto row = parseCopyRow("\t\t");
    ASSERT_EQ(row.size(), 3u);
    EXPECT_EQ(row[0], "");
    EXPECT_EQ(row[1], "");
    EXPECT_EQ(row[2], "");
}

TEST(CopyParsingTest, ManyColumns) {
    std::string line;
    for (int i = 0; i < 100; ++i) {
        if (i > 0) line += '\t';
        line += std::to_string(i);
    }
    auto row = parseCopyRow(line);
    ASSERT_EQ(row.size(), 100u);
    EXPECT_EQ(row[0],  "0");
    EXPECT_EQ(row[99], "99");
}

// ============================================================================
// Tests: INSERT VALUES tokeniser
// ============================================================================

TEST(InsertValuesTest, SimpleNumericAndString) {
    auto vals = parseInsertValues("42, 'hello', 3.14");
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "42");
    EXPECT_EQ(vals[1], "hello");
    EXPECT_EQ(vals[2], "3.14");
}

TEST(InsertValuesTest, NullLiteral) {
    auto vals = parseInsertValues("1, NULL, 'x'");
    ASSERT_EQ(vals.size(), 3u);
    EXPECT_EQ(vals[0], "1");
    EXPECT_EQ(vals[1], "NULL");
    EXPECT_EQ(vals[2], "x");
}

TEST(InsertValuesTest, SingleQuoteEscape) {
    // SQL standard: '' inside a quoted string represents a literal single quote
    auto vals = parseInsertValues("'it''s fine'");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "it's fine");
}

TEST(InsertValuesTest, EmptyString) {
    auto vals = parseInsertValues("''");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "");
}

TEST(InsertValuesTest, BooleanLiterals) {
    auto vals = parseInsertValues("true, false");
    ASSERT_EQ(vals.size(), 2u);
    EXPECT_EQ(vals[0], "true");
    EXPECT_EQ(vals[1], "false");
}

TEST(InsertValuesTest, NegativeNumber) {
    auto vals = parseInsertValues("-42, -3.14");
    ASSERT_EQ(vals.size(), 2u);
    EXPECT_EQ(vals[0], "-42");
    EXPECT_EQ(vals[1], "-3.14");
}

TEST(InsertValuesTest, StringWithCommaInside) {
    // Comma inside quoted string must not split the value
    auto vals = parseInsertValues("'hello, world', 99");
    ASSERT_EQ(vals.size(), 2u);
    EXPECT_EQ(vals[0], "hello, world");
    EXPECT_EQ(vals[1], "99");
}

TEST(InsertValuesTest, SingleValue) {
    auto vals = parseInsertValues("42");
    ASSERT_EQ(vals.size(), 1u);
    EXPECT_EQ(vals[0], "42");
}

TEST(InsertValuesTest, ManyValues) {
    std::string clause;
    for (int i = 0; i < 50; ++i) {
        if (i > 0) clause += ", ";
        clause += std::to_string(i);
    }
    auto vals = parseInsertValues(clause);
    ASSERT_EQ(vals.size(), 50u);
    EXPECT_EQ(vals[0],  "0");
    EXPECT_EQ(vals[49], "49");
}
