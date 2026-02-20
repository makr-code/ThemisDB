#include "importers/postgres_importer.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

namespace themis {
namespace importers {

// ============================================================================
// PostgreSQLImporter Implementation
// ============================================================================

PostgreSQLImporter::PostgreSQLImporter() {
}

PostgreSQLImporter::~PostgreSQLImporter() {
    cancel();
}

std::vector<std::string> PostgreSQLImporter::getSupportedTypes() const {
    return {"postgresql", "postgres", "pg_dump"};
}

bool PostgreSQLImporter::initialize(const std::string& config) {
    cancelled_ = false;
    schemas_.clear();
    
    THEMIS_INFO("PostgreSQL Importer initialized");
    return true;
}

bool PostgreSQLImporter::validateSource(const std::string& source_path, std::vector<std::string>& errors) {
    std::ifstream file(source_path);
    if (!file) {
        errors.push_back("Cannot open file: " + source_path);
        return false;
    }
    
    // Check if it looks like a PostgreSQL dump
    std::string line;
    bool found_pg_dump = false;
    int lines_checked = 0;
    
    while (std::getline(file, line) && lines_checked < 100) {
        if (line.find("PostgreSQL database dump") != std::string::npos ||
            line.find("pg_dump") != std::string::npos ||
            line.find("-- Dumped from database version") != std::string::npos) {
            found_pg_dump = true;
            break;
        }
        lines_checked++;
    }
    
    if (!found_pg_dump) {
        errors.push_back("File does not appear to be a PostgreSQL dump");
        return false;
    }
    
    THEMIS_INFO("Source validation successful: {}", source_path);
    return true;
}

ImportStats PostgreSQLImporter::importData(
    const std::string& source_path,
    const ImportOptions& options,
    ProgressCallback progress_callback
) {
    ImportStats stats;
    auto start_time = std::chrono::steady_clock::now();
    
    THEMIS_INFO("Starting PostgreSQL import from: {}", source_path);
    THEMIS_INFO("Options: {}", options.toJson().dump());
    
    if (options.dry_run) {
        THEMIS_INFO("DRY RUN MODE - No data will be imported");
    }
    
    // Parse dump file
    if (!parseDumpFile(source_path, options, stats, progress_callback)) {
        if (stats.structured_errors.empty()) {
            addError(stats, ImportErrorCode::FILE_READ_FAILED,
                     ImportErrorSeverity::CRITICAL, "Failed to parse dump file");
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
    
    // Structured JSON completion summary (Phase 3 observability)
    THEMIS_INFO("Import summary: {}", stats.toJson().dump());
    THEMIS_INFO("Import completed: {} records imported, {} failed, {} skipped in {:.2f}s",
        stats.imported_records, stats.failed_records, stats.skipped_records, stats.elapsed_seconds);
    
    return stats;
}

void PostgreSQLImporter::cancel() {
    cancelled_ = true;
    THEMIS_INFO("Import cancelled");
}

json PostgreSQLImporter::getSourceSchema(const std::string& source_path) {
    schemas_.clear();
    
    std::ifstream file(source_path);
    if (!file) {
        return json::array();
    }
    
    std::string line;
    std::string current_sql;
    
    while (std::getline(file, line)) {
        // Skip comments
        if (line.empty() || line[0] == '-') continue;
        
        current_sql += line + " ";
        
        // Complete statement?
        if (line.find(';') != std::string::npos) {
            if (current_sql.find("CREATE TABLE") != std::string::npos) {
                TableSchema schema;
                if (parseCreateTable(current_sql, schema)) {
                    schemas_[schema.name] = schema;
                }
            }
            current_sql.clear();
        }
    }
    
    // Convert to JSON
    json result = json::array();
    for (const auto& [name, schema] : schemas_) {
        json table_json = {
            {"name", schema.name},
            {"schema", schema.schema},
            {"columns", schema.columns},
            {"column_types", schema.column_types},
            {"primary_keys", schema.primary_keys}
        };
        result.push_back(table_json);
    }
    
    return result;
}

// ============================================================================
// Private Methods
// ============================================================================

bool PostgreSQLImporter::parseDumpFile(const std::string& file_path, const ImportOptions& options,
                                        ImportStats& stats, ProgressCallback& callback) {
    std::ifstream file(file_path);
    if (!file) {
        addError(stats, ImportErrorCode::FILE_OPEN_FAILED, ImportErrorSeverity::CRITICAL,
                 "Cannot open file: " + file_path);
        return false;
    }

    // --- Checkpoint / resume support ---
    std::streampos resume_offset = 0;
    if (!options.checkpoint_file.empty()) {
        ImportStats dummy;
        if (loadCheckpoint(options.checkpoint_file, resume_offset, dummy)) {
            THEMIS_INFO("Resuming import from byte offset {}", static_cast<long>(resume_offset));
            file.seekg(resume_offset);
            // Carry accumulated counts from the checkpoint
            stats.imported_records = dummy.imported_records;
            stats.failed_records   = dummy.failed_records;
            stats.skipped_records  = dummy.skipped_records;
            stats.total_records    = dummy.total_records;
            stats.tables_processed = dummy.tables_processed;
        }
    }

    std::string line;
    std::string current_sql;
    size_t line_number = 0;
    size_t batch_row_count = 0;
    
    while (std::getline(file, line) && !cancelled_) {
        line_number++;
        
        // Skip blank lines and SQL comments
        if (line.empty() || (line.size() >= 2 && line[0] == '-' && line[1] == '-')) {
            continue;
        }
        
        current_sql += line + " ";

        // Statement-size guard
        if (options.max_statement_size_bytes > 0 &&
            current_sql.size() > options.max_statement_size_bytes) {
            addError(stats, ImportErrorCode::STATEMENT_TOO_LARGE,
                     ImportErrorSeverity::WARNING,
                     "SQL statement exceeds max_statement_size_bytes (" +
                     std::to_string(options.max_statement_size_bytes) + ")",
                     "line " + std::to_string(line_number));
            stats.warnings.push_back("Statement too large near line " +
                                     std::to_string(line_number));
            current_sql.clear();
            if (!options.continue_on_error) return false;
            continue;
        }
        
        // Complete statement?
        if (line.find(';') != std::string::npos) {
            // Parse different statement types
            if (current_sql.find("CREATE TABLE") != std::string::npos ||
                current_sql.find("CREATE SCHEMA") != std::string::npos) {
                TableSchema schema;
                if (parseCreateTable(current_sql, schema)) {
                    if (shouldImportTable(schema.name, options)) {
                        schemas_[schema.name] = schema;
                        stats.tables_processed++;
                        THEMIS_DEBUG("Parsed table schema: {}", schema.name);
                        reportProgress(callback, "schema", stats.tables_processed, 0);
                    }
                } else {
                    addError(stats, ImportErrorCode::PARSE_CREATE_TABLE,
                             ImportErrorSeverity::WARNING,
                             "Failed to parse CREATE TABLE statement",
                             "line " + std::to_string(line_number));
                    stats.warnings.push_back("Failed to parse CREATE TABLE near line " +
                                             std::to_string(line_number));
                }
            }
            else if (current_sql.find("INSERT INTO") != std::string::npos) {
                stats.total_records++;
                if (!options.dry_run) {
                    parseInsert(current_sql, options, stats, line_number);
                }
                batch_row_count++;
            }
            else if (current_sql.find("COPY ") != std::string::npos) {
                // Extract table name and optional column list from COPY header
                // Pattern: COPY [schema.]table [(col1, col2, ...)] FROM stdin;
                std::regex copy_regex(
                    R"(COPY\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?\s+FROM\s+stdin)",
                    std::regex_constants::icase);
                std::smatch match;
                if (std::regex_search(current_sql, match, copy_regex)) {
                    std::string table_name = match[1].str();
                    std::vector<std::string> col_list;
                    if (match[2].matched && !match[2].str().empty()) {
                        std::istringstream css(match[2].str());
                        std::string col;
                        while (std::getline(css, col, ',')) {
                            col.erase(0, col.find_first_not_of(" \t"));
                            col.erase(col.find_last_not_of(" \t") + 1);
                            if (!col.empty()) col_list.push_back(col);
                        }
                    }
                    size_t before_copy = stats.imported_records;
                    if (!options.dry_run) {
                        parseCopy(file, table_name, col_list, options, stats);
                    } else {
                        // skip COPY data block in dry-run mode
                        std::string skip_line;
                        while (std::getline(file, skip_line)) {
                            if (skip_line == "\\." || skip_line.rfind("\\.", 0) == 0) break;
                            stats.total_records++;
                        }
                    }
                    batch_row_count += stats.imported_records - before_copy;
                } else {
                    addError(stats, ImportErrorCode::PARSE_COPY_HEADER,
                             ImportErrorSeverity::WARNING,
                             "Could not parse COPY header",
                             "line " + std::to_string(line_number));
                    stats.warnings.push_back("Could not parse COPY header near line " +
                                             std::to_string(line_number));
                }
            }
            
            current_sql.clear();

            // Checkpoint after each batch
            if (!options.checkpoint_file.empty() &&
                options.batch_size > 0 &&
                batch_row_count >= options.batch_size) {
                std::streampos current_pos = file.tellg();
                saveCheckpoint(options.checkpoint_file, current_pos, stats);
                batch_row_count = 0;
                reportProgress(callback, "data", stats.imported_records, 0);
            }
        }
    }

    // Final checkpoint on clean completion
    if (!options.checkpoint_file.empty() && !cancelled_) {
        saveCheckpoint(options.checkpoint_file, file.tellg(), stats);
    }
    
    return !cancelled_;
}

bool PostgreSQLImporter::parseCreateTable(const std::string& sql, TableSchema& schema) {
    // Regex-based parsing for CREATE TABLE statements.
    // Handles schema-qualified names: CREATE TABLE [schema.]table (...)
    std::regex table_regex(R"(CREATE TABLE\s+(?:(\w+)\.)?(\w+)\s*\()");
    std::smatch match;
    
    if (std::regex_search(sql, match, table_regex)) {
        if (match.size() > 2) {
            schema.schema = match[1].str();
            schema.name = match[2].str();
        } else {
            schema.name = match[1].str();
        }
        
        // Extract columns (simplified; does not handle nested parentheses in defaults)
        size_t start = sql.find('(');
        size_t end = sql.find_last_of(')');
        if (start != std::string::npos && end != std::string::npos) {
            std::string columns_str = sql.substr(start + 1, end - start - 1);
            
            // Split by comma (simplified - doesn't handle nested parentheses)
            std::stringstream ss(columns_str);
            std::string column_def;
            
            while (std::getline(ss, column_def, ',')) {
                // Trim whitespace
                column_def.erase(0, column_def.find_first_not_of(" \t\n\r"));
                column_def.erase(column_def.find_last_not_of(" \t\n\r") + 1);
                
                if (column_def.empty()) continue;

                // Skip table-level constraints
                if (column_def.find("CONSTRAINT") != std::string::npos ||
                    column_def.find("PRIMARY KEY") != std::string::npos ||
                    column_def.find("FOREIGN KEY") != std::string::npos ||
                    column_def.find("UNIQUE") != std::string::npos ||
                    column_def.find("CHECK") != std::string::npos) {
                    continue;
                }
                
                // Extract column name and type
                std::istringstream col_ss(column_def);
                std::string col_name, col_type;
                col_ss >> col_name >> col_type;
                
                // Strip surrounding quotes from column name
                if (!col_name.empty() && col_name.front() == '"') {
                    col_name = col_name.substr(1, col_name.size() - 2);
                }

                if (!col_name.empty() && !col_type.empty()) {
                    schema.columns.push_back(col_name);
                    schema.column_types[col_name] = col_type;
                }
            }
        }
        
        return !schema.name.empty();
    }
    
    return false;
}

bool PostgreSQLImporter::parseInsert(const std::string& sql, const ImportOptions& options,
                                      ImportStats& stats, size_t line_number) {
    // Extract table name: INSERT INTO [schema.]table [(col1,...)] VALUES (...)
    std::regex insert_regex(R"(INSERT INTO\s+(?:\w+\.)?(\w+)\s*(?:\(([^)]*)\))?\s+VALUES\s*\((.+)\)\s*;?\s*$)",
                            std::regex_constants::icase);
    std::smatch match;
    
    if (!std::regex_search(sql, match, insert_regex)) {
        addError(stats, ImportErrorCode::PARSE_INSERT, ImportErrorSeverity::WARNING,
                 "Could not parse INSERT statement",
                 "line " + std::to_string(line_number));
        stats.failed_records++;
        return false;
    }

    std::string table_name = match[1].str();

    if (!shouldImportTable(table_name, options)) {
        stats.skipped_records++;
        return true;
    }

    // Resolve effective column list
    std::vector<std::string> col_list;
    if (match[2].matched && !match[2].str().empty()) {
        std::istringstream css(match[2].str());
        std::string col;
        while (std::getline(css, col, ',')) {
            col.erase(0, col.find_first_not_of(" \t"));
            col.erase(col.find_last_not_of(" \t") + 1);
            if (!col.empty()) col_list.push_back(col);
        }
    } else if (schemas_.count(table_name)) {
        col_list = schemas_[table_name].columns;
    }

    // Parse the values clause
    std::string values_str = match[3].str();
    std::vector<std::string> values = parseInsertValues(values_str);

    // Build entity JSON
    TableSchema eff_schema;
    eff_schema.name = table_name;
    if (schemas_.count(table_name)) eff_schema = schemas_[table_name];
    if (!col_list.empty()) eff_schema.columns = col_list;

    json entity = convertRowToEntity(eff_schema, values);
    THEMIS_DEBUG("INSERT entity: {}", entity.dump());

    stats.imported_records++;
    return true;
}

bool PostgreSQLImporter::parseCopy(std::ifstream& file, const std::string& table_name,
                                    const std::vector<std::string>& columns,
                                    const ImportOptions& options, ImportStats& stats) {
    if (!shouldImportTable(table_name, options)) {
        // Skip until end marker
        std::string line;
        while (std::getline(file, line)) {
            if (line == "\\." || line.rfind("\\.", 0) == 0) break;
            stats.skipped_records++;
        }
        return true;
    }

    // Resolve effective column list from schema or provided list
    TableSchema eff_schema;
    if (schemas_.count(table_name)) eff_schema = schemas_[table_name];
    if (!columns.empty()) eff_schema.columns = columns;
    eff_schema.name = table_name;

    std::string line;
    size_t row_num = 0;
    while (std::getline(file, line) && !cancelled_) {
        if (line == "\\." || line.rfind("\\.", 0) == 0) {
            break;  // End of COPY data
        }
        
        row_num++;
        stats.total_records++;

        // Row-size guard
        if (options.max_row_size_bytes > 0 && line.size() > options.max_row_size_bytes) {
            addError(stats, ImportErrorCode::ROW_TOO_LARGE,
                     ImportErrorSeverity::WARNING,
                     "COPY row exceeds max_row_size_bytes (" +
                     std::to_string(options.max_row_size_bytes) + ")",
                     "table " + table_name + ", row " + std::to_string(row_num));
            stats.warnings.push_back("Row too large in table " + table_name +
                                     " row " + std::to_string(row_num));
            if (!options.continue_on_error) {
                stats.failed_records++;
                return false;
            }
            stats.failed_records++;
            continue;
        }

        // UTF-8 encoding guard
        if (options.enforce_utf8 && !isValidUtf8(line)) {
            addError(stats, ImportErrorCode::INVALID_UTF8,
                     ImportErrorSeverity::WARNING,
                     "COPY row contains invalid UTF-8 byte sequence",
                     "table " + table_name + ", row " + std::to_string(row_num));
            stats.warnings.push_back("Invalid UTF-8 in table " + table_name +
                                     " row " + std::to_string(row_num));
            if (!options.continue_on_error) {
                stats.failed_records++;
                return false;
            }
            stats.failed_records++;
            continue;
        }

        // Parse tab-separated values with PostgreSQL COPY escape rules
        std::vector<std::string> values = parseCopyRow(line);

        if (!eff_schema.columns.empty() && values.size() != eff_schema.columns.size()) {
            addError(stats, ImportErrorCode::COLUMN_COUNT_MISMATCH,
                     ImportErrorSeverity::WARNING,
                     "COPY row has " + std::to_string(values.size()) +
                     " columns, expected " + std::to_string(eff_schema.columns.size()),
                     "table " + table_name + ", row " + std::to_string(row_num));
            stats.warnings.push_back("Column count mismatch in table " + table_name +
                                     " row " + std::to_string(row_num));
            if (!options.continue_on_error) {
                stats.failed_records++;
                return false;
            }
            stats.failed_records++;
            continue;
        }

        json entity = convertRowToEntity(eff_schema, values);
        THEMIS_DEBUG("COPY entity: {}", entity.dump());

        stats.imported_records++;
    }
    
    return true;
}

std::vector<std::string> PostgreSQLImporter::parseCopyRow(const std::string& line) const {
    // PostgreSQL COPY text format: columns separated by TAB.
    // Special sequences: \N = SQL NULL, \t = tab, \n = newline, \r = CR, \\ = backslash.
    std::vector<std::string> result;
    size_t start = 0;

    // Process each tab-delimited raw field, then unescape
    for (size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == '\t') {
            std::string raw = line.substr(start, i - start);
            result.push_back(unescapeCopyValue(raw));
            start = i + 1;
        }
    }
    return result;
}

std::string PostgreSQLImporter::unescapeCopyValue(const std::string& val) const {
    // \N in COPY format means SQL NULL – represent as empty string sentinel
    if (val == "\\N") {
        return "";  // NULL value
    }
    // Apply other escape sequences
    std::string out;
    out.reserve(val.size());
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] == '\\' && i + 1 < val.size()) {
            char next = val[++i];
            switch (next) {
                case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
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

std::vector<std::string> PostgreSQLImporter::parseInsertValues(const std::string& values_clause) const {
    // Parse a VALUES clause like: 1, 'hello', NULL, 'it''s'
    // Handles single-quoted strings with escaped quotes ('') and numeric/NULL literals.
    std::vector<std::string> result;
    size_t i = 0;
    const size_t n = values_clause.size();

    while (i < n) {
        // Skip leading whitespace
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t')) ++i;
        if (i >= n) break;

        if (values_clause[i] == '\'') {
            // Quoted string
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
            // Unquoted token (number, NULL, true, false, etc.)
            size_t start = i;
            while (i < n && values_clause[i] != ',' && values_clause[i] != ')') ++i;
            std::string token = values_clause.substr(start, i - start);
            // Trim trailing whitespace
            token.erase(token.find_last_not_of(" \t") + 1);
            result.push_back(token);
        }

        // Skip comma separator
        while (i < n && (values_clause[i] == ' ' || values_clause[i] == '\t' ||
                          values_clause[i] == ',')) ++i;
    }

    return result;
}

std::string PostgreSQLImporter::mapPostgreSQLTypeToThemis(const std::string& pg_type,
                                                            const ImportOptions& options) const {
    // Check user-configurable overrides first
    auto it = options.type_overrides.find(pg_type);
    if (it != options.type_overrides.end()) {
        return it->second;
    }

    std::string lower_type = pg_type;
    std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);

    // Also check lowercase override
    it = options.type_overrides.find(lower_type);
    if (it != options.type_overrides.end()) {
        return it->second;
    }

    // Array types
    if (lower_type.back() == ']' || lower_type.find("[]") != std::string::npos ||
        lower_type.rfind("array", 0) == 0) {
        return "array";
    }

    // Exact / prefix matches for PostgreSQL built-in types
    if (lower_type == "bigserial" || lower_type == "bigint" || lower_type == "int8") return "long";
    if (lower_type == "smallint" || lower_type == "int2" || lower_type == "smallserial") return "integer";
    if (lower_type == "integer" || lower_type == "int" || lower_type == "int4" ||
        lower_type == "serial" || lower_type == "serial4") return "integer";
    if (lower_type == "real" || lower_type == "float4") return "float";
    if (lower_type == "double precision" || lower_type == "float8") return "double";
    if (lower_type == "numeric" || lower_type == "decimal") return "double";
    if (lower_type == "money") return "double";
    if (lower_type == "boolean" || lower_type == "bool") return "boolean";
    if (lower_type == "text" || lower_type == "name") return "string";
    if (lower_type == "uuid") return "string";
    if (lower_type == "inet" || lower_type == "cidr" || lower_type == "macaddr" ||
        lower_type == "macaddr8") return "string";
    if (lower_type == "xml") return "string";
    if (lower_type == "bytea") return "binary";
    if (lower_type == "json" || lower_type == "jsonb") return "json";
    if (lower_type == "interval") return "string";
    if (lower_type == "point" || lower_type == "line" || lower_type == "lseg" ||
        lower_type == "box" || lower_type == "path" || lower_type == "polygon" ||
        lower_type == "circle") return "geo";
    if (lower_type == "tsvector" || lower_type == "tsquery") return "string";
    if (lower_type == "oid" || lower_type == "xid" || lower_type == "cid") return "integer";

    // Prefix-based fallbacks
    if (lower_type.find("int") != std::string::npos) return "integer";
    if (lower_type.find("serial") != std::string::npos) return "integer";
    if (lower_type.find("float") != std::string::npos) return "double";
    if (lower_type.find("char") != std::string::npos) return "string";
    if (lower_type.find("varchar") != std::string::npos) return "string";
    if (lower_type.find("timestamp") != std::string::npos) return "datetime";
    if (lower_type.find("date") != std::string::npos) return "date";
    if (lower_type.find("time") != std::string::npos) return "time";
    if (lower_type.find("json") != std::string::npos) return "json";

    return "string";  // Default: treat unknown types as strings
}

bool PostgreSQLImporter::shouldImportTable(const std::string& table_name, const ImportOptions& options) {
    // Check exclude list
    if (std::find(options.exclude_tables.begin(), options.exclude_tables.end(), table_name) != options.exclude_tables.end()) {
        return false;
    }
    
    // Check include list (if specified)
    if (!options.include_tables.empty()) {
        return std::find(options.include_tables.begin(), options.include_tables.end(), table_name) != options.include_tables.end();
    }
    
    return true;
}

json PostgreSQLImporter::convertRowToEntity(const TableSchema& schema, const std::vector<std::string>& values) {
    json entity;
    entity["_type"] = schema.name;
    
    for (size_t i = 0; i < values.size() && i < schema.columns.size(); i++) {
        entity[schema.columns[i]] = values[i];
    }
    
    return entity;
}

void PostgreSQLImporter::addError(ImportStats& stats, ImportErrorCode code,
                                   ImportErrorSeverity severity, const std::string& message,
                                   const std::string& location) const {
    ImportError err;
    err.code     = code;
    err.severity = severity;
    err.message  = message;
    err.location = location;
    stats.structured_errors.push_back(err);
    if (severity == ImportErrorSeverity::ERROR || severity == ImportErrorSeverity::CRITICAL) {
        stats.errors.push_back(message);
    }
}

bool PostgreSQLImporter::isValidUtf8(const std::string& s) {
    // Validate that every byte sequence in s is valid UTF-8.
    // Uses the standard multi-byte decoding rules:
    //   1-byte  (ASCII):       0xxxxxxx
    //   2-byte continuation:   110xxxxx 10xxxxxx
    //   3-byte continuation:   1110xxxx 10xxxxxx 10xxxxxx
    //   4-byte continuation:   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    // Rejects overlong encodings, surrogates (U+D800–U+DFFF), and values > U+10FFFF.
    const auto* bytes = reinterpret_cast<const unsigned char*>(s.data());
    const size_t len  = s.size();
    size_t i = 0;
    while (i < len) {
        unsigned char c = bytes[i];
        size_t extra = 0;
        uint32_t codepoint = 0;
        if (c <= 0x7F) {
            // ASCII
            ++i;
            continue;
        } else if ((c & 0xE0) == 0xC0) {
            extra     = 1;
            codepoint = c & 0x1F;
        } else if ((c & 0xF0) == 0xE0) {
            extra     = 2;
            codepoint = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
            extra     = 3;
            codepoint = c & 0x07;
        } else {
            return false;  // Invalid lead byte
        }
        if (i + extra >= len) return false;  // Truncated sequence
        for (size_t j = 1; j <= extra; ++j) {
            unsigned char cc = bytes[i + j];
            if ((cc & 0xC0) != 0x80) return false;  // Invalid continuation byte
            codepoint = (codepoint << 6) | (cc & 0x3F);
        }
        // Reject overlong encodings
        if (extra == 1 && codepoint < 0x80)   return false;
        if (extra == 2 && codepoint < 0x800)  return false;
        if (extra == 3 && codepoint < 0x10000) return false;
        // Reject surrogates (U+D800–U+DFFF)
        if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return false;
        // Reject values above U+10FFFF
        if (codepoint > 0x10FFFF) return false;
        i += 1 + extra;
    }
    return true;
}

bool PostgreSQLImporter::loadCheckpoint(const std::string& checkpoint_file,
                                         std::streampos& offset,
                                         ImportStats& accumulated_stats) const {
    std::ifstream f(checkpoint_file);
    if (!f) return false;

    try {
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        json doc = json::parse(content);
        offset = static_cast<std::streampos>(doc.value("byte_offset", (long long)0));
        accumulated_stats.imported_records = doc.value("imported_records", (size_t)0);
        accumulated_stats.failed_records   = doc.value("failed_records",   (size_t)0);
        accumulated_stats.skipped_records  = doc.value("skipped_records",  (size_t)0);
        accumulated_stats.total_records    = doc.value("total_records",    (size_t)0);
        accumulated_stats.tables_processed = doc.value("tables_processed", (size_t)0);
        THEMIS_INFO("Checkpoint loaded from {}: byte_offset={}", checkpoint_file,
                    static_cast<long>(offset));
        return true;
    } catch (const json::parse_error& e) {
        THEMIS_INFO("Could not parse checkpoint file {}: JSON error at byte {}: {}, starting fresh",
                    checkpoint_file, e.byte, e.what());
        return false;
    } catch (const std::exception& e) {
        THEMIS_INFO("Could not load checkpoint file {}: {}, starting fresh",
                    checkpoint_file, e.what());
        return false;
    }
}

void PostgreSQLImporter::saveCheckpoint(const std::string& checkpoint_file,
                                         std::streampos offset,
                                         const ImportStats& stats) const {
    std::ofstream f(checkpoint_file, std::ios::trunc);
    if (!f) {
        THEMIS_INFO("Could not write checkpoint file {}", checkpoint_file);
        return;
    }
    json doc = {
        {"byte_offset",       static_cast<long long>(offset)},
        {"imported_records",  stats.imported_records},
        {"failed_records",    stats.failed_records},
        {"skipped_records",   stats.skipped_records},
        {"total_records",     stats.total_records},
        {"tables_processed",  stats.tables_processed}
    };
    f << doc.dump(2);
    THEMIS_DEBUG("Checkpoint saved to {}: byte_offset={}", checkpoint_file,
                 static_cast<long>(offset));
}

void PostgreSQLImporter::reportProgress(ProgressCallback& callback, const std::string& stage, size_t current, size_t total) {
    if (callback) {
        callback(stage, current, total);
    }
}

// ============================================================================
// PostgreSQLImporterPlugin Implementation
// ============================================================================

PostgreSQLImporterPlugin::PostgreSQLImporterPlugin() 
    : importer_(std::make_unique<PostgreSQLImporter>()) {
}

plugins::PluginCapabilities PostgreSQLImporterPlugin::getCapabilities() const {
    plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching = true;
    caps.thread_safe = false;  // Not thread-safe (uses instance state)
    return caps;
}

bool PostgreSQLImporterPlugin::initialize(const char* config_json) {
    if (!importer_) {
        return false;
    }
    return importer_->initialize(config_json ? config_json : "{}");
}

void PostgreSQLImporterPlugin::shutdown() {
    if (importer_) {
        importer_->cancel();
    }
}

} // namespace importers
} // namespace themis

// ============================================================================
// Plugin Entry Points
// ============================================================================

extern "C" {
    themis::plugins::IThemisPlugin* createPlugin() {
        return new themis::importers::PostgreSQLImporterPlugin();
    }
    
    void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {
        delete plugin;
    }
}

