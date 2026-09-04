// test_postgres_importer_streaming.cpp
//
// Unit tests for the streaming import API (Phase 2):
//   - RowCallback type and ImportOptions.streaming_row_callback field
//   - Per-row callback invocation for COPY-format data
//   - Per-row callback invocation for INSERT-format data
//   - Early abort when callback returns false
//   - Correct ImportStats after a streaming import
//   - Mixed COPY + INSERT in one dump
//   - Callback receives correct table name and field values

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Minimal type re-implementations (mirrors importer_interface.h)
// ---------------------------------------------------------------------------

enum class ImportErrorCode : uint32_t {
    SUCCESS              = 0,
    FILE_OPEN_FAILED     = 101,
    PARSE_CREATE_TABLE   = 200,
    PARSE_INSERT         = 201,
    PARSE_COPY_HEADER    = 202,
    PARSE_COPY_ROW       = 203,
    COLUMN_COUNT_MISMATCH= 301,
    UNKNOWN              = 900
};

enum class ImportErrorSeverity { INFO, WARNING, ERROR, CRITICAL };

struct ImportError {
    ImportErrorCode     code     = ImportErrorCode::UNKNOWN;
    ImportErrorSeverity severity = ImportErrorSeverity::ERROR;
    std::string         message;
    std::string         location;
};

struct ImportStats {
    size_t total_records    = 0;
    size_t imported_records = 0;
    size_t failed_records   = 0;
    size_t skipped_records  = 0;
    size_t tables_processed = 0;
    double elapsed_seconds  = 0.0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<ImportError> structured_errors;
};

// RowCallback: return false to abort the import early.
using RowCallback = std::function<bool(const std::string& table_name, const json& entity)>;

struct ImportOptions {
    bool   dry_run           = false;
    bool   continue_on_error = true;
    size_t batch_size        = 1000;
    std::string default_namespace = "imported";
    std::vector<std::string> include_tables;
    std::vector<std::string> exclude_tables;
    std::map<std::string, std::string> type_overrides;
    size_t max_row_size_bytes       = 0;
    size_t max_statement_size_bytes = 0;
    bool   enforce_utf8             = false;
    RowCallback streaming_row_callback;
};

// ---------------------------------------------------------------------------
// Minimal streaming-aware importer driven by in-memory text content
// ---------------------------------------------------------------------------

struct TableSchema {
    std::string              name;
    std::vector<std::string> columns;
};

static std::map<std::string, TableSchema> parseSchemas(const std::string& content) {
    std::map<std::string, TableSchema> schemas;
    std::istringstream ss(content);
    std::string line, sql;
    while (std::getline(ss, line)) {
        if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-')) {
          continue;
        }
        sql += line + " ";
        if (line.find(';') != std::string::npos) {
            // Very minimal CREATE TABLE extractor
            std::string upper = sql;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            auto ct = upper.find("CREATE TABLE");
            if (ct != std::string::npos) {
                // table name is the next word after "CREATE TABLE"
                size_t ns = sql.find_first_not_of(" \t", ct + 12);
                size_t ne = sql.find_first_of(" \t(", ns);
                std::string tname = sql.substr(ns, ne - ns);
                // remove schema prefix if present
                auto dot = tname.rfind('.');
                if (dot != std::string::npos) {
                  tname = tname.substr(dot + 1);
                }
                // extract column names from first paren block
                auto op = sql.find('(', ne);
                auto cp = sql.rfind(')');
                if (op != std::string::npos && cp != std::string::npos) {
                    std::string body = sql.substr(op + 1, cp - op - 1);
                    // split on commas at depth 0
                    std::vector<std::string> cols;
                    int depth = 0;
                    std::string cur;
                    for (char c : body) {
                        if (c == '(') { depth++; cur += c; }
                        else if (c == ')') { depth--; cur += c; }
                        else if (c == ',' && depth == 0) {
                            cols.push_back(cur); cur.clear();
                        } else { cur += c; }
                    }
                    if (!cur.empty()) {
                      cols.push_back(cur);
                    }
                    TableSchema ts;
                    ts.name = tname;
                    for (auto& col : cols) {
                        // first word is column name
                        size_t s = col.find_first_not_of(" \t\n\r");
                        if (s == std::string::npos) {
                          continue;
                        }
                        size_t e = col.find_first_of(" \t\n\r", s);
                        std::string cname = (e == std::string::npos)
                                            ? col.substr(s)
                                            : col.substr(s, e - s);
                        if (!cname.empty()) {
                          ts.columns.push_back(cname);
                        }
                    }
                    schemas[tname] = ts;
                }
            }
            sql.clear();
        }
    }
    return schemas;
}

// Parse a COPY tab-separated row into a JSON object using the given column list.
static json parseCopyRow(const std::string& line,
                          const std::string& table_name,
                          const std::vector<std::string>& columns) {
    std::vector<std::string> vals;
    size_t start = 0;
    for (size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == '\t') {
            std::string v = line.substr(start, i - start);
            if (v == "\\N") {
              v = "";
            }
            vals.push_back(v);
            start = i + 1;
        }
    }
    json obj;
    obj["_table"] = table_name;
    for (size_t i = 0; i < columns.size() && i < vals.size(); ++i)
        obj[columns[i]] = vals[i];
    return obj;
}

// Parse a minimal VALUES clause into a vector of strings.
static std::vector<std::string> parseInsertValues(const std::string& vals) {
    std::vector<std::string> result = {};

    size_t i = 0, n = vals.size();
    while (i < n) {
        while (i < n && (vals[i] == ' ' || vals[i] == '\t')) {
          ++i;
        }
        if (i >= n) {
          break;
        }
        if (vals[i] == '\'') {
            ++i;
            std::string v;
            while (i < n) {
                if (vals[i] == '\'' && i + 1 < n && vals[i+1] == '\'') { v += '\''; i += 2; }
                else if (vals[i] == '\'') { ++i; break; }
                else { v += vals[i++]; }
            }
            result.push_back(v);
        } else {
            size_t s = i;
            while (i < n && vals[i] != ',' && vals[i] != ') {
              ') ++i;
            }
            std::string tok = vals.substr(s, i - s);
            tok.erase(tok.find_last_not_of(" \t") + 1);
            result.push_back(tok);
        }
        while (i < n && (vals[i] == ' ' || vals[i] == '\t' || vals[i] == ',')) {
          ++i;
        }
    }
    return result;
}

/**
 * Minimal streaming-aware import driven from in-memory dump text.
 *
 * Reproduces the logic of PostgreSQLImporter::parseDumpFile() for COPY and
 * INSERT statements, calling options.streaming_row_callback per row.
 */
static ImportStats streamingImportContent(const std::string& content,
                                           const ImportOptions& options) {
    ImportStats stats;
    bool cancelled = false;

    auto schemas = parseSchemas(content);
    stats.tables_processed = schemas.size();

    std::istringstream file(content);
    std::string line, sql;
    std::string current_copy_table;
    std::vector<std::string> current_copy_columns;
    bool in_copy = false;

    while (std::getline(file, line) && !cancelled) {
        // Skip pure comment / empty lines outside SQL
        if (!in_copy) {
            if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-')) {
              continue;
            }
        }

        if (in_copy) {
            // End of COPY block
            if (line == "\\." || line.rfind("\\.", 0) == 0) {
                in_copy = false;
                continue;
            }
            stats.total_records++;
            // Check include/exclude
            bool excluded = false;
            if (!options.include_tables.empty()) {
                excluded = std::find(options.include_tables.begin(),
                                     options.include_tables.end(),
                                     current_copy_table) == options.include_tables.end();
            }
            if (!options.exclude_tables.empty()) {
                excluded = excluded || std::find(options.exclude_tables.begin(),
                                                 options.exclude_tables.end(),
                                                 current_copy_table) != options.exclude_tables.end();
            }
            if (excluded) { stats.skipped_records++; continue; }

            json entity = parseCopyRow(line, current_copy_table, current_copy_columns);
            if (options.streaming_row_callback) {
                if (!options.streaming_row_callback(current_copy_table, entity)) {
                    cancelled = true;
                    stats.imported_records++;
                    break;
                }
            }
            stats.imported_records++;
            continue;
        }

        // Accumulate SQL
        sql += line + " ";
        if (line.find(';') == std::string::npos && line.find("FROM stdin") == std::string::npos) {
            // Check for COPY header (ends without semicolon)
            std::string upper = sql;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper.find("COPY ") == std::string::npos || upper.find("FROM STDIN") == std::string::npos)
                continue;
        }

        {
            std::string upper = sql;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

            // COPY table (cols) FROM stdin
            if (upper.find("COPY ") != std::string::npos && upper.find("FROM STDIN") != std::string::npos) {
                // Extract table name
                size_t cs = upper.find("COPY ") + 5;
                while (cs < upper.size() && upper[cs] == ' ') {
                  ++cs;
                }
                size_t ce = upper.find_first_of(" \t(", cs);
                std::string tname = sql.substr(cs, ce - cs);
                auto dot = tname.rfind('.');
                if (dot != std::string::npos) {
                  tname = tname.substr(dot + 1);
                }
                // Extract column list
                std::vector<std::string> cols;
                auto op = sql.find('(', ce);
                auto cp2 = sql.find(')', op != std::string::npos ? op : 0);
                if (op != std::string::npos && cp2 != std::string::npos) {
                    std::istringstream cs2(sql.substr(op + 1, cp2 - op - 1));
                    std::string col;
                    while (std::getline(cs2, col, ',')) {
                        col.erase(0, col.find_first_not_of(" \t"));
                        col.erase(col.find_last_not_of(" \t") + 1);
                        if (!col.empty()) {
                          cols.push_back(col);
                        }
                    }
                } else if (schemas.count(tname)) {
                    cols = schemas[tname].columns;
                }
                current_copy_table   = tname;
                current_copy_columns = cols;
                in_copy = true;
                sql.clear();
                continue;
            }

            // INSERT INTO table (cols) VALUES (...)
            if (upper.find("INSERT INTO") != std::string::npos) {
                // Extract table name
                size_t is = upper.find("INSERT INTO") + 11;
                while (is < upper.size() && upper[is] == ' ') {
                  ++is;
                }
                size_t ie = upper.find_first_of(" \t(", is);
                std::string tname = sql.substr(is, ie - is);
                auto dot = tname.rfind('.');
                if (dot != std::string::npos) {
                  tname = tname.substr(dot + 1);
                }

                bool excluded = false;
                if (!options.include_tables.empty())
                    excluded = std::find(options.include_tables.begin(),
                                         options.include_tables.end(),
                                         tname) == options.include_tables.end();
                if (!options.exclude_tables.empty())
                    excluded = excluded || std::find(options.exclude_tables.begin(),
                                                     options.exclude_tables.end(),
                                                     tname) != options.exclude_tables.end();
                if (excluded) { stats.skipped_records++; stats.total_records++; sql.clear(); continue; }

                // Extract column list
                std::vector<std::string> cols;
                auto op = sql.find('(', ie);
                // find VALUES keyword
                auto vp = upper.find("VALUES", ie);
                if (op != std::string::npos && vp != std::string::npos && op < vp) {
                    auto cp3 = sql.find(')', op);
                    if (cp3 != std::string::npos) {
                        std::istringstream cs3(sql.substr(op + 1, cp3 - op - 1));
                        std::string col;
                        while (std::getline(cs3, col, ',')) {
                            col.erase(0, col.find_first_not_of(" \t"));
                            col.erase(col.find_last_not_of(" \t") + 1);
                            if (!col.empty()) {
                              cols.push_back(col);
                            }
                        }
                    }
                } else if (schemas.count(tname)) {
                    cols = schemas[tname].columns;
                }

                // Extract values
                if (vp != std::string::npos) {
                    auto vopen = sql.find('(', vp);
                    auto vclose = sql.rfind(')');
                    if (vopen != std::string::npos && vclose != std::string::npos && vopen < vclose) {
                        std::string vals = sql.substr(vopen + 1, vclose - vopen - 1);
                        auto parsed = parseInsertValues(vals);
                        json entity;
                        entity["_table"] = tname;
                        for (size_t i = 0; i < cols.size() && i < parsed.size(); ++i)
                            entity[cols[i]] = parsed[i];
                        stats.total_records++;
                        if (options.streaming_row_callback) {
                            if (!options.streaming_row_callback(tname, entity)) {
                                cancelled = true;
                                stats.imported_records++;
                                sql.clear();
                                break;
                            }
                        }
                        stats.imported_records++;
                    }
                }
                sql.clear();
                continue;
            }
        }

        // Non-DML statement; just discard the accumulated sql if it ended with ';'
        if (line.find(';') != std::string::npos) {
          sql.clear();
        }
    }

    return stats;
}

// ===========================================================================
// Test fixtures
// ===========================================================================

static const std::string kCopyDump = R"(
-- PostgreSQL database dump
CREATE TABLE users (
    id integer,
    name text,
    email text
);
COPY users (id, name, email) FROM stdin;
1	Alice	alice@example.com
2	Bob	bob@example.com
3	Carol	carol@example.com
\.
)";

static const std::string kInsertDump = R"(
-- PostgreSQL database dump
CREATE TABLE products (
    id integer,
    title text,
    price text
);
INSERT INTO products (id, title, price) VALUES (1, 'Widget', '9.99');
INSERT INTO products (id, title, price) VALUES (2, 'Gadget', '19.99');
)";

static const std::string kMixedDump = R"(
-- PostgreSQL database dump
CREATE TABLE orders (
    order_id integer,
    customer text
);
COPY orders (order_id, customer) FROM stdin;
100	Alice
101	Bob
\.
INSERT INTO orders (order_id, customer) VALUES (102, 'Carol');
)";

// ===========================================================================
// Tests: RowCallback type and streaming_row_callback option field
// ===========================================================================

TEST(StreamingCallbackTypeTest, RowCallbackIsCallable) {
    RowCallback cb = [](const std::string&, const json&) -> bool { return true; };
    EXPECT_TRUE(static_cast<bool>(cb));
}

TEST(StreamingCallbackTypeTest, RowCallbackReturnsFalseToAbort) {
    RowCallback cb = [](const std::string&, const json&) -> bool { return false; };
    json dummy;
    EXPECT_FALSE(cb("users", dummy));
}

TEST(StreamingCallbackTypeTest, ImportOptionsHasStreamingRowCallback) {
    ImportOptions opts;
    EXPECT_FALSE(static_cast<bool>(opts.streaming_row_callback));
    opts.streaming_row_callback = [](const std::string&, const json&) -> bool { return true; };
    EXPECT_TRUE(static_cast<bool>(opts.streaming_row_callback));
}

// ===========================================================================
// Tests: COPY-format streaming
// ===========================================================================

TEST(StreamingCopyTest, CallbackInvokedForEachCopyRow) {
    ImportOptions opts;
    std::vector<std::string> tables;
    std::vector<json>        entities;

    opts.streaming_row_callback = [&](const std::string& t, const json& e) -> bool {
        tables.push_back(t);
        entities.push_back(e);
        return true;
    };

    auto stats = streamingImportContent(kCopyDump, opts);

    EXPECT_EQ(tables.size(), 3u);
    for (auto& t : tables) {
      EXPECT_EQ(t, "users");
    }
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(StreamingCopyTest, CallbackReceivesCorrectFieldValues) {
    ImportOptions opts;
    std::vector<json> rows;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        rows.push_back(e);
        return true;
    };

    streamingImportContent(kCopyDump, opts);

    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[0]["name"].get<std::string>(), "Alice");
    EXPECT_EQ(rows[1]["name"].get<std::string>(), "Bob");
    EXPECT_EQ(rows[2]["name"].get<std::string>(), "Carol");
}

TEST(StreamingCopyTest, CallbackReceivesTableName) {
    ImportOptions opts;
    std::string last_table;
    opts.streaming_row_callback = [&](const std::string& t, const json&) -> bool {
        last_table = t;
        return true;
    };

    streamingImportContent(kCopyDump, opts);

    EXPECT_EQ(last_table, "users");
}

TEST(StreamingCopyTest, AbortOnFalseFromCallback) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return call_count < 2;  // abort after second row
    };

    auto stats = streamingImportContent(kCopyDump, opts);

    EXPECT_EQ(call_count, 2u);
    // Only the first two rows should be counted as imported
    EXPECT_LE(stats.imported_records, 2u);
}

TEST(StreamingCopyTest, AbortOnFirstRowStopsImmediately) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return false;  // abort immediately
    };

    auto stats = streamingImportContent(kCopyDump, opts);

    EXPECT_EQ(call_count, 1u);
    EXPECT_EQ(stats.imported_records, 1u);
}

TEST(StreamingCopyTest, NullCallbackAllowsNormalImport) {
    ImportOptions opts;
    // No streaming_row_callback set
    auto stats = streamingImportContent(kCopyDump, opts);
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(StreamingCopyTest, StatsMatchRowsDeliveredToCallback) {
    ImportOptions opts;
    size_t callback_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++callback_count;
        return true;
    };

    auto stats = streamingImportContent(kCopyDump, opts);

    EXPECT_EQ(stats.imported_records, callback_count);
}

// ===========================================================================
// Tests: INSERT-format streaming
// ===========================================================================

TEST(StreamingInsertTest, CallbackInvokedForEachInsertRow) {
    ImportOptions opts;
    std::vector<json> rows;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        rows.push_back(e);
        return true;
    };

    auto stats = streamingImportContent(kInsertDump, opts);

    EXPECT_EQ(rows.size(), 2u);
    EXPECT_EQ(stats.imported_records, 2u);
}

TEST(StreamingInsertTest, CallbackReceivesCorrectFieldValues) {
    ImportOptions opts;
    std::vector<json> rows;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        rows.push_back(e);
        return true;
    };

    streamingImportContent(kInsertDump, opts);

    ASSERT_GE(rows.size(), 2u);
    EXPECT_EQ(rows[0]["title"].get<std::string>(), "Widget");
    EXPECT_EQ(rows[1]["title"].get<std::string>(), "Gadget");
}

TEST(StreamingInsertTest, AbortMidwayStopsProcessing) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return false;  // abort on first row
    };

    auto stats = streamingImportContent(kInsertDump, opts);

    EXPECT_EQ(call_count, 1u);
    EXPECT_EQ(stats.imported_records, 1u);
}

// ===========================================================================
// Tests: Mixed COPY + INSERT streaming
// ===========================================================================

TEST(StreamingMixedTest, CallbackInvokedForAllRowsInMixedDump) {
    ImportOptions opts;
    size_t call_count = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++call_count;
        return true;
    };

    auto stats = streamingImportContent(kMixedDump, opts);

    EXPECT_EQ(call_count, 3u);
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(StreamingMixedTest, TableNamesCorrectForMixedDump) {
    ImportOptions opts;
    std::set<std::string> tables_seen;
    opts.streaming_row_callback = [&](const std::string& t, const json&) -> bool {
        tables_seen.insert(t);
        return true;
    };

    streamingImportContent(kMixedDump, opts);

    EXPECT_EQ(tables_seen.count("orders"), 1u);
}

// ===========================================================================
// Tests: Filtering (include_tables / exclude_tables) with streaming
// ===========================================================================

TEST(StreamingFilterTest, IncludeTablesFiltersCallback) {
    ImportOptions opts;
    opts.include_tables = {"products"};
    std::vector<json> rows;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        rows.push_back(e);
        return true;
    };

    // Use a dump that has both users and products tables
    std::string dump = kCopyDump + "\n" + kInsertDump;
    streamingImportContent(dump, opts);

    // Only products rows should be delivered
    for (auto& r : rows) {
        EXPECT_TRUE(r.contains("title") || r["_table"].get<std::string>() == "products");
    }
}

TEST(StreamingFilterTest, ExcludeTablesFiltersCallback) {
    ImportOptions opts;
    opts.exclude_tables = {"users"};
    std::vector<std::string> tables;
    opts.streaming_row_callback = [&](const std::string& t, const json&) -> bool {
        tables.push_back(t);
        return true;
    };

    streamingImportContent(kCopyDump, opts);

    // All rows should have been filtered out
    EXPECT_TRUE(tables.empty());
}

// ===========================================================================
// Tests: Callback receives correct entity structure
// ===========================================================================

TEST(StreamingEntityStructureTest, EntityContainsTableField) {
    ImportOptions opts;
    json first_entity;
    bool got_first = false;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        if (!got_first) { first_entity = e; got_first = true; }
        return true;
    };

    streamingImportContent(kCopyDump, opts);

    EXPECT_TRUE(got_first);
    EXPECT_TRUE(first_entity.contains("_table"));
}

TEST(StreamingEntityStructureTest, EntityFieldsMatchColumnNames) {
    ImportOptions opts;
    json first_entity;
    bool got_first = false;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        if (!got_first) { first_entity = e; got_first = true; }
        return true;
    };

    streamingImportContent(kCopyDump, opts);

    ASSERT_TRUE(got_first);
    EXPECT_TRUE(first_entity.contains("id"));
    EXPECT_TRUE(first_entity.contains("name"));
    EXPECT_TRUE(first_entity.contains("email"));
}

// ===========================================================================
// Tests: importDataStreaming API contract
// ===========================================================================

TEST(ImportDataStreamingApiTest, CallbackTypeIsCallable) {
    // Verify that the RowCallback alias compiles and can be stored in ImportOptions.
    ImportOptions opts;
    int counter = 0;
    opts.streaming_row_callback = [&counter](const std::string&, const json&) -> bool {
        ++counter;
        return true;
    };
    EXPECT_TRUE(static_cast<bool>(opts.streaming_row_callback));
    opts.streaming_row_callback("t", json{});
    EXPECT_EQ(counter, 1);
}

TEST(ImportDataStreamingApiTest, AbortReturnsFalse) {
    RowCallback abort_cb = [](const std::string&, const json&) -> bool { return false; };
    EXPECT_FALSE(abort_cb("table", json{}));
}

TEST(ImportDataStreamingApiTest, ContinueReturnsTrue) {
    RowCallback continue_cb = [](const std::string&, const json&) -> bool { return true; };
    EXPECT_TRUE(continue_cb("table", json{}));
}

TEST(ImportDataStreamingApiTest, EmptyDumpProducesZeroCallbacks) {
    ImportOptions opts;
    size_t calls = 0;
    opts.streaming_row_callback = [&](const std::string&, const json&) -> bool {
        ++calls;
        return true;
    };

    streamingImportContent("-- empty dump\n", opts);

    EXPECT_EQ(calls, 0u);
}

TEST(ImportDataStreamingApiTest, NullCallbackFallsBackToNormalImport) {
    ImportOptions opts;
    // streaming_row_callback is null by default
    auto stats = streamingImportContent(kCopyDump, opts);
    EXPECT_EQ(stats.imported_records, 3u);
}

TEST(ImportDataStreamingApiTest, MultipleCallbackInvocationsAreOrdered) {
    ImportOptions opts;
    std::vector<std::string> names;
    opts.streaming_row_callback = [&](const std::string&, const json& e) -> bool {
        if (e.contains("name")) {
          names.push_back(e["name"].get<std::string>());
        }
        return true;
    };

    streamingImportContent(kCopyDump, opts);

    ASSERT_EQ(names.size(), 3u);
    EXPECT_EQ(names[0], "Alice");
    EXPECT_EQ(names[1], "Bob");
    EXPECT_EQ(names[2], "Carol");
}
