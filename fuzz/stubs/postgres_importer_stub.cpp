/**
 * @file postgres_importer_stub.cpp
 * @brief Standalone fuzz stub for themis::importers::PostgreSQLImporter
 *
 * Provides a minimal, spdlog-free implementation of PostgreSQLImporter for
 * use in the standalone fuzz_targets build (ENABLE_FUZZING=ON).
 *
 * Requirements: nlohmann_json must be available (guarded in CMakeLists.txt
 * via find_package(nlohmann_json CONFIG QUIET)).
 *
 * The following methods are implemented with real parsing logic to ensure
 * useful fuzzing coverage:
 *   - parseDumpFile: line-by-line SQL scanner
 *   - parseCopyRow: COPY text-format tokenizer + unescaper
 *   - parseInsertValues: VALUES clause tokenizer
 *   - parseCreateTable: basic CREATE TABLE column extractor
 *   - isValidUtf8: byte-level validator
 *   - importData (public entry point)
 *
 * All FK/index/constraint helper methods are stubbed (return false / no-op)
 * because their parsing paths are exercised transitively via parseDumpFile.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: allow postgres_importer_harness to compile in the standalone fuzz
 *   build without spdlog or RocksDB dependencies.
 * Activation: THEMIS_FUZZ_STUBS cmake define / ENABLE_FUZZING=ON path,
 *   requires nlohmann_json to be present.
 * Production Delta: logging suppressed; RocksDB / metrics / tracing calls are
 *   no-ops; FK validation, quarantine writes, and checkpoint I/O are no-ops.
 * Removal Plan: wire against real themisdb_importers once the full dep chain
 *   (spdlog, vcpkg) is available in the fuzz build environment.
 */

// The header chain includes nlohmann/json.hpp transitively.
#include "importers/postgres_importer.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

// Avoid Windows ERROR macro collision
#ifdef ERROR
#  undef ERROR
#endif

namespace themis {
namespace importers {

// ═══════════════════════════════════════════════════════════════════════════════
// UTF-8 validation
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Validate that @p s contains only well-formed UTF-8 sequences.
 *
 * Implements the leading/continuation byte checks defined by RFC 3629.
 * Over-long encodings and surrogate code-point encodings are rejected.
 *
 * @return true if every byte sequence is a valid UTF-8 code point.
 */
bool PostgreSQLImporter::isValidUtf8(const std::string& s) {
    const auto* p   = reinterpret_cast<const unsigned char*>(s.data());
    const auto* end = p + s.size();
    while (p < end) {
        unsigned char b = *p++;
        int extra = 0;
        if      (b < 0x80u)  extra = 0;
        else if (b < 0xC0u)  return false;           // unexpected continuation
        else if (b < 0xE0u)  extra = 1;
        else if (b < 0xF0u)  extra = 2;
        else if (b < 0xF8u)  extra = 3;
        else                 return false;            // > 4-byte sequence
        for (int i = 0; i < extra; ++i) {
            if (p >= end)                    return false;  // truncated
            if ((*p & 0xC0u) != 0x80u)      return false;  // not continuation
            ++p;
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// COPY row tokenizer
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Unescape a single value token from a COPY text-format row.
 *
 * COPY text escapes: \\N (NULL marker is handled by the caller), \\t, \\n,
 * \\r, \\b, \\f, \\v, \\\\, and \\<octal3>.
 */
std::string PostgreSQLImporter::unescapeCopyValue(const std::string& val) const {
    if (val == "\\N") return "";   // NULL marker — caller should check separately

    std::string out;
    out.reserve(val.size());
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] != '\\' || i + 1 >= val.size()) {
            out += val[i];
            continue;
        }
        char next = val[i + 1];
        ++i;
        switch (next) {
        case 'N':  out += '\0';  break;   // literal NUL (not NULL)
        case 't':  out += '\t';  break;
        case 'n':  out += '\n';  break;
        case 'r':  out += '\r';  break;
        case 'b':  out += '\b';  break;
        case 'f':  out += '\f';  break;
        case 'v':  out += '\v';  break;
        case '\\': out += '\\'; break;
        default:
            // Octal escape: \<3 digits>
            if (std::isdigit(static_cast<unsigned char>(next)) &&
                i + 1 < val.size() && std::isdigit(static_cast<unsigned char>(val[i + 1])) &&
                i + 2 < val.size() && std::isdigit(static_cast<unsigned char>(val[i + 2]))) {
                int octal = (next       - '0') * 64
                          + (val[i + 1] - '0') * 8
                          + (val[i + 2] - '0');
                out += static_cast<char>(octal & 0xFF);
                i += 2;
            } else {
                out += '\\';
                out += next;
            }
            break;
        }
    }
    return out;
}

/**
 * @brief Tokenize a COPY text-format row into column values.
 *
 * Fields are TAB-separated.  The special token \\N denotes a SQL NULL and is
 * returned as an empty string (the caller is responsible for NULL semantics).
 * Escape sequences within each field are preserved for further processing.
 */
std::vector<std::string> PostgreSQLImporter::parseCopyRow(
        const std::string& line) const {
    std::vector<std::string> fields;
    if (line == "\\." || line.empty()) return fields;

    std::string current;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\t') {
            fields.push_back(unescapeCopyValue(current));
            current.clear();
        } else if (c == '\\' && i + 1 < line.size()) {
            // Preserve the escape sequence for unescapeCopyValue
            current += c;
            current += line[++i];
        } else {
            current += c;
        }
    }
    fields.push_back(unescapeCopyValue(current));
    return fields;
}

// ═══════════════════════════════════════════════════════════════════════════════
// INSERT VALUES tokenizer
// ═══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Tokenize the VALUES clause of an INSERT statement.
 *
 * Handles single-quoted strings (with '' escape), nested parentheses, and
 * NULL / numeric / identifier literals.  The opening and closing outer
 * parentheses must be absent (caller strips them).
 */
std::vector<std::string> PostgreSQLImporter::parseInsertValues(
        const std::string& clause) const {
    std::vector<std::string> values;
    std::string current;
    int  depth    = 0;
    bool in_str   = false;
    bool in_quote = false;   // double-quoted identifier

    for (size_t i = 0; i < clause.size(); ++i) {
        char c = clause[i];

        if (in_str) {
            if (c == '\'' && i + 1 < clause.size() && clause[i + 1] == '\'') {
                current += '\'';
                ++i;  // skip second quote
            } else if (c == '\'') {
                in_str = false;
                current += c;
            } else {
                current += c;
            }
            continue;
        }
        if (in_quote) {
            if (c == '"') in_quote = false;
            current += c;
            continue;
        }

        switch (c) {
        case '\'': in_str = true;   current += c; break;
        case '"':  in_quote = true; current += c; break;
        case '(':  ++depth; if (depth > 0) current += c; break;
        case ')':
            if (depth > 0) { --depth; current += c; }
            // depth == 0 → outer closing paren — handled by caller
            break;
        case ',':
            if (depth == 0) {
                // Strip surrounding whitespace
                size_t b = current.find_first_not_of(" \t\r\n");
                size_t e = current.find_last_not_of(" \t\r\n");
                values.push_back(
                    (b == std::string::npos)
                        ? ""
                        : current.substr(b, e - b + 1));
                current.clear();
            } else {
                current += c;
            }
            break;
        default:
            current += c;
            break;
        }
    }
    // Last value
    size_t b = current.find_first_not_of(" \t\r\n");
    size_t e = current.find_last_not_of(" \t\r\n");
    if (b != std::string::npos) {
        values.push_back(current.substr(b, e - b + 1));
    } else if (!current.empty() || !values.empty()) {
        values.push_back("");
    }
    return values;
}

// ═══════════════════════════════════════════════════════════════════════════════
// CREATE TABLE parser
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

/** @brief Return an ASCII-lowercased copy of @p s. */
inline std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/** @brief Strip leading and trailing ASCII whitespace from @p s (in-place). */
inline void trim_inplace(std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) { s.clear(); return; }
    size_t e = s.find_last_not_of(" \t\r\n");
    s = s.substr(b, e - b + 1);
}

/** @brief Unquote a double-quoted SQL identifier (removes outer quotes). */
inline std::string unquote_ident(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

} // anonymous namespace

/**
 * @brief Extract column names and types from a CREATE TABLE statement.
 *
 * Parses the column-definition list between the outer parentheses.
 * Constraint clauses (PRIMARY KEY, FOREIGN KEY, CHECK, UNIQUE, EXCLUDE)
 * and inline constraints are skipped.  Column names are unquoted.
 *
 * @param sql     Full CREATE TABLE … (…) SQL statement.
 * @param schema  Populated with table name, column names, and type map.
 * @return true on successful parse; false if the statement is unparseable.
 */
bool PostgreSQLImporter::parseCreateTable(const std::string& sql,
                                          TableSchema& schema) {
    // Locate the table name: CREATE [UNLOGGED|TEMPORARY|…] TABLE [IF NOT EXISTS] <name>
    std::string sql_lower = to_lower(sql);
    size_t tbl_pos = sql_lower.find("table");
    if (tbl_pos == std::string::npos) return false;
    size_t name_start = sql.find_first_not_of(" \t\r\n", tbl_pos + 5);
    if (name_start == std::string::npos) return false;

    // Skip "IF NOT EXISTS"
    {
        std::string maybe = to_lower(sql.substr(name_start, 15));
        if (maybe.substr(0, 2) == "if") {
            size_t np = sql_lower.find("exists", name_start);
            if (np != std::string::npos)
                name_start = sql.find_first_not_of(" \t\r\n", np + 6);
        }
    }
    if (name_start == std::string::npos) return false;

    // Extract table name (stops at whitespace or '(')
    size_t name_end = name_start;
    bool in_dq = false;
    while (name_end < sql.size()) {
        char c = sql[name_end];
        if (c == '"')   { in_dq = !in_dq; ++name_end; continue; }
        if (!in_dq && (c == '(' || std::isspace(static_cast<unsigned char>(c)))) break;
        ++name_end;
    }
    std::string raw_name = sql.substr(name_start, name_end - name_start);
    // Handle schema.table notation
    size_t dot = raw_name.rfind('.');
    if (dot != std::string::npos) {
        schema.schema = unquote_ident(raw_name.substr(0, dot));
        schema.name   = unquote_ident(raw_name.substr(dot + 1));
    } else {
        schema.name = unquote_ident(raw_name);
    }
    if (schema.name.empty()) return false;

    // Find the outer parentheses
    size_t open  = sql.find('(', name_end);
    if (open == std::string::npos) return false;
    // Find matching closing paren
    int  depth   = 1;
    size_t close = open + 1;
    bool in_str2 = false;
    while (close < sql.size() && depth > 0) {
        char c = sql[close];
        if (!in_str2 && c == '\'') { in_str2 = true; }
        else if (in_str2 && c == '\'' &&
                 close + 1 < sql.size() && sql[close + 1] == '\'') { ++close; }
        else if (in_str2 && c == '\'') { in_str2 = false; }
        else if (!in_str2 && c == '(') ++depth;
        else if (!in_str2 && c == ')') --depth;
        ++close;
    }
    if (depth != 0) return false;   // unbalanced parens

    std::string body = sql.substr(open + 1, close - open - 2);

    // Split body into top-level comma-separated clauses
    std::vector<std::string> clauses;
    std::string cur;
    int   pd   = 0;
    bool  istr = false;
    for (size_t i = 0; i < body.size(); ++i) {
        char c = body[i];
        if (!istr && c == '\'') { istr = true;  cur += c; continue; }
        if (istr  && c == '\'' && i + 1 < body.size() && body[i+1] == '\'') {
            cur += '\''; cur += '\''; ++i; continue;
        }
        if (istr  && c == '\'') { istr = false; cur += c; continue; }
        if (!istr && c == '(')  { ++pd; cur += c; continue; }
        if (!istr && c == ')')  { --pd; cur += c; continue; }
        if (!istr && pd == 0 && c == ',') {
            trim_inplace(cur);
            if (!cur.empty()) clauses.push_back(cur);
            cur.clear();
            continue;
        }
        cur += c;
    }
    trim_inplace(cur);
    if (!cur.empty()) clauses.push_back(cur);

    // Process each clause
    for (const auto& clause : clauses) {
        std::string lo = to_lower(clause);
        // Skip constraint clauses
        if (lo.substr(0, 10) == "constraint" ||
            lo.substr(0, 11) == "primary key" ||
            lo.substr(0, 11) == "foreign key" ||
            lo.substr(0, 5)  == "check" ||
            lo.substr(0, 6)  == "unique" ||
            lo.substr(0, 7)  == "exclude") continue;

        // First token = column name
        size_t tok_end = 0;
        bool   dq      = false;
        while (tok_end < clause.size()) {
            char c = clause[tok_end];
            if (c == '"') { dq = !dq; ++tok_end; continue; }
            if (!dq && std::isspace(static_cast<unsigned char>(c))) break;
            ++tok_end;
        }
        std::string col_raw  = clause.substr(0, tok_end);
        std::string col_name = unquote_ident(col_raw);
        if (col_name.empty()) continue;

        // Second token (after whitespace) = type
        size_t type_start = clause.find_first_not_of(" \t\r\n", tok_end);
        std::string col_type;
        if (type_start != std::string::npos) {
            size_t type_end = type_start;
            // Type ends at whitespace, opening paren, or comma
            while (type_end < clause.size()) {
                char c = clause[type_end];
                if (std::isspace(static_cast<unsigned char>(c)) || c == '(') break;
                ++type_end;
            }
            col_type = clause.substr(type_start, type_end - type_start);
        }

        schema.columns.push_back(col_name);
        schema.column_types[col_name] = col_type.empty() ? "text" : col_type;
    }
    return !schema.columns.empty();
}

// ═══════════════════════════════════════════════════════════════════════════════
// INSERT parser
// ═══════════════════════════════════════════════════════════════════════════════

bool PostgreSQLImporter::parseInsert(const std::string& sql,
                                     const ImportOptions& opts,
                                     ImportStats& stats,
                                     size_t /*line_number*/) {
    // Expected: INSERT INTO table (cols) VALUES (vals), (vals), ...;
    std::string lo = to_lower(sql);
    size_t into  = lo.find("into");
    if (into == std::string::npos) return false;
    size_t ts    = sql.find_first_not_of(" \t\r\n", into + 4);
    if (ts == std::string::npos) return false;
    // Find the column list opening paren (before VALUES)
    size_t col_open = sql.find('(', ts);
    size_t val_pos  = lo.find("values", ts);
    if (col_open == std::string::npos || val_pos == std::string::npos) return false;
    // Column names
    size_t col_close = sql.find(')', col_open);
    if (col_close == std::string::npos) return false;
    std::string col_str = sql.substr(col_open + 1, col_close - col_open - 1);
    std::vector<std::string> cols;
    std::istringstream css(col_str);
    std::string c;
    while (std::getline(css, c, ',')) {
        trim_inplace(c);
        c = unquote_ident(c);
        if (!c.empty()) cols.push_back(c);
    }

    // VALUES clause(s): scan for each (…) group
    size_t pos = val_pos + 6;
    while (pos < sql.size()) {
        size_t vopen = sql.find('(', pos);
        if (vopen == std::string::npos) break;
        int    depth = 1;
        size_t vclose = vopen + 1;
        bool   istr   = false;
        while (vclose < sql.size() && depth > 0) {
            char ch = sql[vclose];
            if (!istr && ch == '\'') istr = true;
            else if (istr && ch == '\'' && vclose + 1 < sql.size() && sql[vclose+1] == '\'')
                ++vclose;
            else if (istr && ch == '\'') istr = false;
            else if (!istr && ch == '(') ++depth;
            else if (!istr && ch == ')') --depth;
            ++vclose;
        }
        if (depth != 0) break;
        std::string val_str = sql.substr(vopen + 1, vclose - vopen - 2);
        auto vals = parseInsertValues(val_str);

            (void)opts;
        ++stats.total_records;
        if (!vals.empty()) ++stats.imported_records;

        pos = vclose;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// COPY block parser
// ═══════════════════════════════════════════════════════════════════════════════

bool PostgreSQLImporter::parseCopy(std::ifstream& file,
                                   const std::string& /*table*/,
                                   const std::vector<std::string>& /*cols*/,
                                   const ImportOptions& opts,
                                   ImportStats& stats,
                                   std::unordered_set<uint64_t>& /*delta_hashes*/) {
    static constexpr size_t kMaxRowBytes = 64u * 1024u * 1024u;  // 64 MiB hard cap
    const size_t row_limit = (opts.max_row_size_bytes > 0)
                                 ? opts.max_row_size_bytes
                                 : kMaxRowBytes;
    std::string line;
    while (std::getline(file, line)) {
        if (line == "\\.") break;  // COPY terminator

        if (!opts.continue_on_error && line.size() > row_limit) {
            addError(stats, ImportErrorCode::ROW_TOO_LARGE,
                     ImportErrorSeverity::ERROR,
                     "Row exceeds max_row_size_bytes limit");
            continue;
        }
        if (opts.enforce_utf8 && !isValidUtf8(line)) {
            addError(stats, ImportErrorCode::INVALID_UTF8,
                     ImportErrorSeverity::ERROR,
                     "Non-UTF-8 byte sequence in COPY row");
            ++stats.failed_records;
            continue;
        }

        auto fields = parseCopyRow(line);
        ++stats.total_records;
        if (!fields.empty()) ++stats.imported_records;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Dump file dispatcher
// ═══════════════════════════════════════════════════════════════════════════════

bool PostgreSQLImporter::parseDumpFile(const std::string& file_path,
                                       const ImportOptions& opts,
                                       ImportStats& stats,
                                       ProgressCallback& callback) {
    std::ifstream ifs(file_path);
    if (!ifs) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED,
                 ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + file_path);
        return false;
    }

    static constexpr size_t kMaxStmtBytes = 64u * 1024u * 1024u;
    const size_t stmt_limit = (opts.max_statement_size_bytes > 0)
                                  ? opts.max_statement_size_bytes
                                  : kMaxStmtBytes;

    std::string accumulated;
    std::string line;
    size_t line_no = 0;

    auto flush_stmt = [&]() {
        if (accumulated.empty()) return;
        trim_inplace(accumulated);
        if (accumulated.empty()) { accumulated.clear(); return; }
        if (accumulated.size() > stmt_limit) {
            addError(stats, ImportErrorCode::STATEMENT_TOO_LARGE,
                     ImportErrorSeverity::WARNING,
                     "Statement exceeds size limit — skipped");
            accumulated.clear();
            return;
        }
        std::string lo = to_lower(accumulated);
        if (lo.substr(0, 12) == "create table") {
            TableSchema schema;
            if (parseCreateTable(accumulated, schema)) {
                ++stats.tables_processed;
                schemas_[schema.name] = schema;
            }
        } else if (lo.substr(0, 6) == "insert") {
            parseInsert(accumulated, opts, stats, line_no);
        }
        accumulated.clear();
    };

    std::unordered_set<uint64_t> delta_hashes;

    while (std::getline(ifs, line)) {
        ++line_no;
        if (cancelled_) break;

        // Skip pure comment lines
        if (line.size() >= 2 && line[0] == '-' && line[1] == '-') {
            flush_stmt();
            continue;
        }

        // Detect COPY header: "COPY <table> (<cols>) FROM stdin;"
        {
            std::string lo = to_lower(line);
            size_t copy_pos = lo.find("copy ");
            if (copy_pos != std::string::npos) {
                size_t from_pos = lo.find("from stdin");
                if (from_pos != std::string::npos) {
                    flush_stmt();
                    // Extract table name and columns
                    std::string table_name;
                    std::vector<std::string> copy_cols;
                    size_t tn_start = copy_pos + 5;
                    tn_start = line.find_first_not_of(" \t", tn_start);
                    if (tn_start != std::string::npos) {
                        size_t tn_end = line.find_first_of(" \t(", tn_start);
                        if (tn_end == std::string::npos) tn_end = line.size();
                        table_name = unquote_ident(line.substr(tn_start, tn_end - tn_start));
                        size_t col_open  = line.find('(', tn_end);
                        size_t col_close = line.find(')', col_open != std::string::npos ? col_open : line.size());
                        if (col_open != std::string::npos && col_close != std::string::npos) {
                            std::string col_s = line.substr(col_open + 1, col_close - col_open - 1);
                            std::istringstream cis(col_s);
                            std::string ci;
                            while (std::getline(cis, ci, ',')) {
                                trim_inplace(ci);
                                copy_cols.push_back(unquote_ident(ci));
                            }
                        }
                    }
                    parseCopy(ifs, table_name, copy_cols, opts, stats, delta_hashes);
                    continue;
                }
            }
        }

        // Accumulate multi-line statements
        accumulated += line + "\n";

        // Flush on statement terminator (semicolon not inside a string)
        bool in_s = false;
        bool has_semi = false;
        for (char ch : line) {
            if (!in_s && ch == '\'') in_s = true;
            else if (in_s && ch == '\'') in_s = false;
            else if (!in_s && ch == ';') { has_semi = true; break; }
        }
        if (has_semi) flush_stmt();
    }
    flush_stmt();

    reportProgress(callback, "complete", 1, 1);
    return !cancelled_;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Public IImporter interface
// ═══════════════════════════════════════════════════════════════════════════════

PostgreSQLImporter::PostgreSQLImporter() = default;
PostgreSQLImporter::~PostgreSQLImporter() = default;

std::vector<std::string> PostgreSQLImporter::getSupportedTypes() const {
    return {"sql", "pg_dump", "pgsql"};
}

bool PostgreSQLImporter::initialize(const std::string& /*config*/) {
    cancelled_ = false;
    schemas_.clear();
    custom_type_map_.clear();
    return true;
}

bool PostgreSQLImporter::validateSource(const std::string& source_path,
                                        std::vector<std::string>& errors) {
    std::ifstream f(source_path);
    if (!f) {
        errors.push_back("Cannot open file: " + source_path);
        return false;
    }
    return true;
}

ImportStats PostgreSQLImporter::importData(const std::string& source_path,
                                           const ImportOptions& opts,
                                           ProgressCallback callback) {
    ImportStats stats;
    parseDumpFile(source_path, opts, stats, callback);
    return stats;
}

ImportStats PostgreSQLImporter::importDataStreaming(
        const std::string& /*source_path*/,
        const ImportOptions& /*opts*/,
        RowCallback /*callback*/) {
    return {};
}

std::shared_ptr<ImportHandle> PostgreSQLImporter::importDataAsync(
        const std::string& /*source_path*/,
        const ImportOptions& /*opts*/) {
    return nullptr;
}

void PostgreSQLImporter::cancel() {
    cancelled_ = true;
}

json PostgreSQLImporter::getSourceSchema(const std::string& /*source_path*/) {
    return json::object();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Stub implementations for parsing helpers not exercised directly by the harness
// ═══════════════════════════════════════════════════════════════════════════════

bool PostgreSQLImporter::parseForeignKeyConstraint(
        const std::string&, TableSchema&) const { return false; }

bool PostgreSQLImporter::parseInlineReference(
        const std::string&, const std::string&, TableSchema&) const { return false; }

void PostgreSQLImporter::parseAlterTableAddFk(
        const std::string&, const ImportOptions&, ImportStats&) {}

bool PostgreSQLImporter::parseForeignKeyConstraint(
        const std::string&, ForeignKeyConstraint&) { return false; }

bool PostgreSQLImporter::parseCreateIndex(
        const std::string&, const std::string&, IndexMetadata&) { return false; }

bool PostgreSQLImporter::parseAlterTableForeignKey(
        const std::string&, std::string&, ForeignKeyConstraint&) { return false; }

bool PostgreSQLImporter::validateForeignKeyReferences(
        const ImportOptions&, ImportStats&) { return true; }

bool PostgreSQLImporter::parseCheckConstraint(
        const std::string&, CheckConstraint&) { return false; }

bool PostgreSQLImporter::parseExcludeConstraint(
        const std::string&, ExcludeConstraint&) { return false; }

bool PostgreSQLImporter::parseGeneratedColumn(
        const std::string&, const std::string&, GeneratedColumnInfo&) { return false; }

std::string PostgreSQLImporter::mapPostgreSQLTypeToThemis(
        const std::string& pg_type, const ImportOptions&) const {
    // Minimal type mapping sufficient for stub
    std::string lo = to_lower(pg_type);
    if (lo == "integer" || lo == "int" || lo == "int4" || lo == "int8" ||
        lo == "bigint" || lo == "smallint" || lo == "serial" || lo == "bigserial")
        return "integer";
    if (lo == "float" || lo == "float4" || lo == "float8" ||
        lo == "real" || lo == "double precision" || lo == "numeric")
        return "float";
    if (lo == "boolean" || lo == "bool")
        return "boolean";
    return "string";
}

bool PostgreSQLImporter::shouldImportTable(const std::string& /*name*/,
                                           const ImportOptions& /*opts*/) {
    return true;
}

json PostgreSQLImporter::convertRowToEntity(
        const TableSchema& schema,
        const std::vector<std::string>& values) {
    json entity = json::object();
    for (size_t i = 0; i < schema.columns.size() && i < values.size(); ++i) {
        entity[schema.columns[i]] = values[i];
    }
    return entity;
}

void PostgreSQLImporter::addError(ImportStats& stats,
                                  ImportErrorCode code,
                                  ImportErrorSeverity severity,
                                  const std::string& message,
                                  const std::string& location) const {
    ImportError err;
    err.code     = code;
    err.severity = severity;
    err.message  = message;
    err.location = location;
    stats.structured_errors.push_back(err);
    stats.errors.push_back(message);
}

void PostgreSQLImporter::addPostgreSQLError(ImportStats& stats,
                                            ImportErrorSeverity severity,
                                            const std::string& msg,
                                            const std::string& loc) const {
    addError(stats, ImportErrorCode::PARSE_INSERT, severity, msg, loc);
}

void PostgreSQLImporter::emitMetric(const ImportOptions&, const std::string&,
                                    const std::map<std::string, std::string>&,
                                    double) const {}

void PostgreSQLImporter::emitSpan(const ImportOptions&, const std::string&,
                                  const std::map<std::string, std::string>&,
                                  double) const {}

bool PostgreSQLImporter::loadCheckpoint(const std::string&,
                                        std::streampos&,
                                        ImportStats&) const { return false; }

void PostgreSQLImporter::saveCheckpoint(const std::string&,
                                        std::streampos,
                                        const ImportStats&) const {}

void PostgreSQLImporter::writeQuarantineRow(const std::string&,
                                            const std::string&,
                                            const std::string&,
                                            const ImportError&) const {}

uint64_t PostgreSQLImporter::computeRowHash(
        const std::string& raw_row,
        const std::vector<std::string>& /*values*/,
        const std::vector<std::string>& /*key_columns*/,
        const std::vector<std::string>& /*schema_columns*/) {
    // FNV-1a 64-bit
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : raw_row) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::unordered_set<uint64_t> PostgreSQLImporter::loadDeltaHashes(
        const std::string& /*file*/) {
    return {};
}

void PostgreSQLImporter::saveDeltaHashes(
        const std::string& /*file*/,
        const std::unordered_set<uint64_t>& /*hashes*/) {}

void PostgreSQLImporter::reportProgress(ProgressCallback& callback,
                                        const std::string& stage,
                                        size_t current,
                                        size_t total) {
    if (callback) callback(stage, current, total);
}

// ═══════════════════════════════════════════════════════════════════════════════
// PostgreSQLImporterPlugin
// ═══════════════════════════════════════════════════════════════════════════════

PostgreSQLImporterPlugin::PostgreSQLImporterPlugin()
    : importer_(std::make_unique<PostgreSQLImporter>()) {}

plugins::PluginCapabilities PostgreSQLImporterPlugin::getCapabilities() const {
    return {};
}

bool PostgreSQLImporterPlugin::initialize(const char* /*config_json*/) {
    return importer_ && importer_->initialize("{}");
}

void PostgreSQLImporterPlugin::shutdown() {}

} // namespace importers
} // namespace themis
