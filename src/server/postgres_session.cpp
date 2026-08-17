/**
 * @file postgres_session.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=10, M=68, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifdef THEMIS_ENABLE_POSTGRES_WIRE

#include "server/postgres_session.h"
#include <stdexcept>
#include "query/query_engine.h"
#include "query/aql_parser.h"
#include "query/aql_translator.h"
#include "version.h"
#include <boost/beast/core.hpp>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include "utils/logger.h"

namespace {
    // Helper function to escape SQL string literals
    std::string escapeSQLString(const std::string& input) {
        std::string result;
        result.reserve(input.size() + 10);
        
        for (char c : input) {
            if (c == '\'') {
                result += "''";  // PostgreSQL escapes single quotes by doubling
            } else if (c == '\\') {
                result += "\\\\";  // Escape backslashes
            } else if (c == '\0') {
                // Skip null bytes or handle specially
                continue;
            } else {
                result += c;
            }
        }
        
        return result;
    }
    
    // Helper function to safely bind parameter value
    std::string bindParameterValue(const std::string& param, int32_t paramType) {
        // Handle NULL
        if (param == "NULL") {
            return "NULL";
        }
        
        // For numeric types, validate and pass through
        if (paramType == 20 || paramType == 21 || paramType == 23 ||  // int8, int2, int4
            paramType == 700 || paramType == 701) {  // float4, float8
            // Basic numeric validation
            bool isValid = true;
            bool hasDot = false;
            for (size_t i = 0; i < param.size(); ++i) {
                char c = param[i];
                if (i == 0 && (c == '-' || c == '+')) continue;
                if (c == '.' && !hasDot) {
                    hasDot = true;
                    continue;
                }
                if (!std::isdigit(c)) {
                    isValid = false;
                    break;
                }
            }
            if (isValid && !param.empty()) {
                return param;
            }
        }
        
        // For all other types (text, varchar, etc.), escape and quote
        return "'" + escapeSQLString(param) + "'";
    }

    void logCurrentException(const char* context) {
        try {
            throw;
        } catch (const std::exception& e) {
            std::cerr << "[PostgresSession] " << context << ": " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[PostgresSession] " << context << ": unknown exception\n";
        }
    }
}

PostgresSession::PostgresSession(asio::ip::tcp::socket socket)
    : socket_(std::move(socket))
    , readTimeoutTimer_(socket_.get_executor())
    , writeTimeoutTimer_(socket_.get_executor())
    , queryEngine_(nullptr) {
}

PostgresSession::PostgresSession(asio::ip::tcp::socket socket, themis::QueryEngine* queryEngine)
    : socket_(std::move(socket))
    , readTimeoutTimer_(socket_.get_executor())
    , writeTimeoutTimer_(socket_.get_executor())
    , queryEngine_(queryEngine) {
}

PostgresSession::~PostgresSession() {
    stop();
}

void PostgresSession::start() {
    stopped_.store(false, std::memory_order_release);
    doRead();
}

void PostgresSession::stop() {
    if (stopped_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    auto weak_self = weak_from_this();
    auto close_fn = [this]() {
        closeSocket();
    };
    if (auto self = weak_self.lock()) {
        asio::dispatch(socket_.get_executor(), [self, close_fn]() {
            close_fn();
        });
        return;
    }
    close_fn();
}

void PostgresSession::closeSocket() {
    boost::beast::error_code ec;
    readTimeoutTimer_.cancel();
    writeTimeoutTimer_.cancel();
    {
        std::lock_guard<std::mutex> lock(writeMutex_);
        writeQueue_.clear();
        writeInProgress_ = false;
    }
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}

void PostgresSession::armReadTimeout() {
    auto self = shared_from_this();
    readTimeoutTimer_.expires_after(kReadTimeout);
    readTimeoutTimer_.async_wait([this, self](const boost::beast::error_code& ec) {
        if (ec || stopped_.load(std::memory_order_acquire)) {
            return;
        }
        try {
            sendErrorResponse("ERROR", "57014", "Connection timed out while waiting for client message");
            stop();
        } catch (...) {
            THEMIS_WARN("postgres_session: unhandled exception caught");
            logCurrentException("Read-timeout handler error");
            stop();
        }
    });
}

void PostgresSession::cancelReadTimeout() {
    boost::beast::error_code ec;
    readTimeoutTimer_.cancel(ec);
}

void PostgresSession::armWriteTimeout() {
    auto self = shared_from_this();
    writeTimeoutTimer_.expires_after(kWriteTimeout);
    writeTimeoutTimer_.async_wait([this, self](const boost::beast::error_code& ec) {
        if (ec || stopped_.load(std::memory_order_acquire)) {
            return;
        }
        try {
            sendErrorResponse("ERROR", "57014", "Connection timed out while sending response");
            stop();
        } catch (...) {
            THEMIS_WARN("postgres_session: unhandled exception caught");
            logCurrentException("Write-timeout handler error");
            stop();
        }
    });
}

void PostgresSession::cancelWriteTimeout() {
    boost::beast::error_code ec;
    writeTimeoutTimer_.cancel(ec);
}

char PostgresSession::currentTransactionStatus() const {
    switch (transactionState_.load(std::memory_order_acquire)) {
        case TransactionState::IN_TRANSACTION:
            return 'T';
        case TransactionState::FAILED:
            return 'E';
        case TransactionState::IDLE:
        default:
            return 'I';
    }
}

void PostgresSession::handleStartupMessage(int32_t protocolVersion, 
                                          const std::map<std::string, std::string>& params) {
    // PostgreSQL startup message handler
    // Validate protocol version (3.0 = 196608)
    const int32_t POSTGRES_PROTOCOL_V3 = 196608; // (3 << 16) | 0
    
    if (protocolVersion != POSTGRES_PROTOCOL_V3) {
        // Send error for unsupported protocol version
        sendErrorResponse("FATAL", "08P01", 
            "Unsupported protocol version: " + std::to_string(protocolVersion) + 
            ". Expected " + std::to_string(POSTGRES_PROTOCOL_V3));
        stop();
        return;
    }
    
    // Extract database, user, and options
    auto dbIt = params.find("database");
    auto userIt = params.find("user");
    
    if (dbIt != params.end()) databaseName_ = dbIt->second;
    if (userIt != params.end()) userName_ = userIt->second;
    
    // Validate that user and database are provided
    if (userName_.empty()) {
        sendErrorResponse("FATAL", "28P01", "No user specified in connection");
        stop();
        return;
    }
    
    if (databaseName_.empty()) {
        databaseName_ = userName_; // Default database name to username
    }
    
    inStartup_.store(false, std::memory_order_release);
    
    // Implement authentication
    // For ThemisDB, we accept connections but mark them as authenticated
    // In a full implementation, this would validate against user database
    // For now, we implement trust authentication (no password required)
    // Production systems should implement MD5 or SCRAM-SHA-256
    sendAuthenticationOk();
    
    // Send server parameters
    sendParameterStatus("server_version", "14.0 (ThemisDB compatibility)");
    sendParameterStatus("server_encoding", "UTF8");
    sendParameterStatus("client_encoding", "UTF8");
    sendParameterStatus("DateStyle", "ISO, MDY");
    sendParameterStatus("TimeZone", "UTC");
    sendParameterStatus("integer_datetimes", "on");
    sendParameterStatus("standard_conforming_strings", "on");
    
    sendBackendKeyData(12345, 67890);
    sendReadyForQuery('I');
    
    isAuthenticated_.store(true, std::memory_order_release);
}

void PostgresSession::handleQuery(const std::string& query) {
    // Trim query
    std::string trimmedQuery = query;
    trimmedQuery.erase(0, trimmedQuery.find_first_not_of(" \t\n\r"));
    trimmedQuery.erase(trimmedQuery.find_last_not_of(" \t\n\r;") + 1);
    
    std::string upperQuery = trimmedQuery;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
    
    // Handle transaction commands
    if (upperQuery == "BEGIN" || upperQuery == "START TRANSACTION" || upperQuery == "BEGIN TRANSACTION") {
        transactionState_.store(TransactionState::IN_TRANSACTION, std::memory_order_release);
        sendCommandComplete("BEGIN");
        sendReadyForQuery('T');
        return;
    }
    
    if (upperQuery == "COMMIT" || upperQuery == "END") {
        TransactionState state = transactionState_.load(std::memory_order_acquire);
        if (state == TransactionState::IN_TRANSACTION) {
            transactionState_.store(TransactionState::IDLE, std::memory_order_release);
            sendCommandComplete("COMMIT");
        } else if (state == TransactionState::FAILED) {
            sendErrorResponse("WARNING", "25P02", "Current transaction is aborted, commands ignored until end of transaction block");
            transactionState_.store(TransactionState::IDLE, std::memory_order_release);
        } else {
            sendErrorResponse("WARNING", "25P01", "There is no transaction in progress");
        }
        sendReadyForQuery('I');
        return;
    }
    
    if (upperQuery == "ROLLBACK" || upperQuery == "ABORT") {
        if (transactionState_.load(std::memory_order_acquire) != TransactionState::IDLE) {
            transactionState_.store(TransactionState::IDLE, std::memory_order_release);
            sendCommandComplete("ROLLBACK");
        } else {
            sendErrorResponse("WARNING", "25P01", "There is no transaction in progress");
        }
        sendReadyForQuery('I');
        return;
    }
    
    // Handle schema queries (pg_catalog, information_schema) for BI tool compatibility
    if (isSchemaQuery(query)) {
        handleSchemaQuery(query);
        sendReadyForQuery(currentTransactionStatus());
        return;
    }
    
    // Handle special PostgreSQL functions
    if (query.find("SELECT version()") != std::string::npos ||
        query.find("select version()") != std::string::npos) {
        std::vector<FieldDescription> fields = {
            {"version", 0, 0, 25, -1, -1, 0} // text type
        };
        sendRowDescription(fields);
        sendDataRow({"PostgreSQL 14.0 (ThemisDB " THEMIS_VERSION_STRING " compatibility mode)"});
        sendCommandComplete("SELECT 1");
        sendReadyForQuery(currentTransactionStatus());
        return;
    }
    
    if (query.find("SELECT current_database()") != std::string::npos ||
        query.find("select current_database()") != std::string::npos) {
        std::vector<FieldDescription> fields = {
            {"current_database", 0, 0, 19, -1, -1, 0}
        };
        sendRowDescription(fields);
        sendDataRow({databaseName_.empty() ? "themisdb" : databaseName_});
        sendCommandComplete("SELECT 1");
        sendReadyForQuery(currentTransactionStatus());
        return;
    }
    
    // Handle COPY commands
    if (upperQuery.find("COPY") == 0) {
        // COPY FROM STDIN or COPY TO STDOUT
        if (upperQuery.find("FROM STDIN") != std::string::npos) {
            // COPY table FROM STDIN — extract table name from between COPY and (
            std::string copyTableName;
            {
                // Original (non-uppercased) query for accurate table name
                constexpr size_t kCopyPrefixLen = sizeof("COPY ") - 1; // 5 chars
                std::string q = query;
                size_t start = kCopyPrefixLen;
                while (start < q.size() && q[start] == ' ') { ++start; }
                // Find end: first of '(' (column list), whitespace, or end-of-string
                size_t end = q.find_first_of(" (", start);
                if (end == std::string::npos) { end = q.size(); }
                copyTableName = q.substr(start, end - start);
            }
            {
                std::lock_guard<std::mutex> lock(copyMutex_);
                copyTableName_ = std::move(copyTableName);
                copyBuffer_.clear();
            }
            copyInProgress_.store(true, std::memory_order_release);
            std::vector<int16_t> formatCodes = {0}; // Text format
            sendCopyInResponse(formatCodes);
            return; // Don't send ReadyForQuery yet
        } else if (upperQuery.find("TO STDOUT") != std::string::npos) {
            // COPY table TO STDOUT
            // Send CopyOutResponse and start sending data
            std::vector<int16_t> formatCodes = {0}; // Text format
            sendCopyOutResponse(formatCodes);
            
            // Send sample data (in production, query database)
            // Format: tab-separated values, newline-terminated
            std::string sampleData = "1\tAlice\talice@example.com\n2\tBob\tbob@example.com\n";
            sendCopyData(std::vector<uint8_t>(sampleData.begin(), sampleData.end()));
            
            // End of data
            sendCopyDone();
            sendCommandComplete("COPY 2"); // 2 rows
            sendReadyForQuery(currentTransactionStatus());
            return;
        }
    }
    
    // Translate SQL to Cypher for regular queries
    try {
        std::string cypherQuery = translateQuery(query);
        
        // Execute Cypher query against ThemisDB
        // When QueryEngine is available, queries would be executed here
        // For protocol-only mode, return empty result
        sendCommandComplete("SELECT 0");
    } catch (const std::exception& e) {
        sendErrorResponse("ERROR", "42601", std::string("Query translation failed: ") + e.what());
        if (transactionState_.load(std::memory_order_acquire) == TransactionState::IN_TRANSACTION) {
            transactionState_.store(TransactionState::FAILED, std::memory_order_release);
        }
    }
    
    sendReadyForQuery(currentTransactionStatus());
}

void PostgresSession::handleParse(const std::string& stmt, const std::string& query, 
                                 const std::vector<int32_t>& paramTypes) {
    // PostgreSQL Parse message handler
    // Validates query syntax and stores prepared statement
    
    try {
        // Validate query syntax by attempting translation
        std::string trimmedQuery = query;
        trimmedQuery.erase(0, trimmedQuery.find_first_not_of(" \t\n\r"));
        trimmedQuery.erase(trimmedQuery.find_last_not_of(" \t\n\r;") + 1);
        
        if (trimmedQuery.empty()) {
            sendErrorResponse("ERROR", "42601", "Empty query string");
            return;
        }
        
        // Store the prepared statement with parameter types
        {
            std::lock_guard<std::mutex> lock(preparedStatementsMutex_);
            preparedStatements_[stmt] = {query, paramTypes};
        }
        sendParseComplete();
        
    } catch (const std::exception& e) {
        sendErrorResponse("ERROR", "42601", std::string("Parse error: ") + e.what());
    }
}

void PostgresSession::handleBind(const std::string& portal, const std::string& stmt, 
                                const std::vector<std::string>& params) {
    // PostgreSQL Bind message handler
    // Binds parameters to prepared statement and creates portal
    
    PreparedStatement preparedStmt;
    {
        std::lock_guard<std::mutex> lock(preparedStatementsMutex_);
        auto stmtIt = preparedStatements_.find(stmt);
        if (stmtIt == preparedStatements_.end()) {
            sendErrorResponse("ERROR", "26000", "Prepared statement not found: " + stmt);
            return;
        }
        preparedStmt = stmtIt->second;
    }
    // Validate parameter count
    if (!preparedStmt.paramTypes.empty() && params.size() != preparedStmt.paramTypes.size()) {
        sendErrorResponse("ERROR", "08P01", 
            "Parameter count mismatch: expected " + std::to_string(preparedStmt.paramTypes.size()) +
            ", got " + std::to_string(params.size()));
        return;
    }
    
    // Create portal with bound parameters
    {
        std::lock_guard<std::mutex> lock(portalsMutex_);
        portals_[portal] = {stmt, params};
    }
    sendBindComplete();
}

void PostgresSession::handleExecute(const std::string& portal, int32_t maxRows) {
    // PostgreSQL Execute message handler with result streaming
    // Executes portal with bound parameters and returns results (up to maxRows)
    
    Portal portalData;
    {
        std::lock_guard<std::mutex> lock(portalsMutex_);
        auto portalIt = portals_.find(portal);
        if (portalIt == portals_.end()) {
            sendErrorResponse("ERROR", "34000", "Portal not found: " + portal);
            return;
        }
        portalData = portalIt->second;
    }

    PreparedStatement preparedStmt;
    {
        std::lock_guard<std::mutex> lock(preparedStatementsMutex_);
        auto stmtIt = preparedStatements_.find(portalData.statementName);
        if (stmtIt == preparedStatements_.end()) {
            sendErrorResponse("ERROR", "26000", "Prepared statement not found");
            return;
        }
        preparedStmt = stmtIt->second;
    }
    
    try {
        // Get query with bound parameters
        std::string query = preparedStmt.query;
        const auto& params = portalData.params;
        const auto& paramTypes = preparedStmt.paramTypes;
        
        // Replace $1, $2, etc. with properly escaped parameter values
        for (size_t i = 0; i < params.size(); ++i) {
            std::string placeholder = "$" + std::to_string(i + 1);
            int32_t paramType = (i < paramTypes.size()) ? paramTypes[i] : 25; // default to text
            std::string escapedValue = bindParameterValue(params[i], paramType);
            
            size_t pos = 0;
            while ((pos = query.find(placeholder, pos)) != std::string::npos) {
                query.replace(pos, placeholder.length(), escapedValue);
                pos += escapedValue.length();
            }
        }
        
        // If this is the first execution, fetch and cache results
        if (!portalData.resultsComplete && portalData.currentRow == 0) {
            // Handle schema queries
            if (isSchemaQuery(query)) {
                handleSchemaQuery(query);
                portalData.resultsComplete = true;
                std::lock_guard<std::mutex> lock(portalsMutex_);
                auto portalIt = portals_.find(portal);
                if (portalIt != portals_.end()) {
                    portalIt->second = portalData;
                }
                return;
            }
            
            // Handle special PostgreSQL functions
            if (query.find("version()") != std::string::npos) {
                std::vector<FieldDescription> fields = {
                    {"version", 0, 0, 25, -1, -1, 0}
                };
                sendRowDescription(fields);
                sendDataRow({"PostgreSQL 14.0 (ThemisDB 1.3.0 compatibility mode)"});
                sendCommandComplete("SELECT 1");
                portalData.resultsComplete = true;
                std::lock_guard<std::mutex> lock(portalsMutex_);
                auto portalIt = portals_.find(portal);
                if (portalIt != portals_.end()) {
                    portalIt->second = portalData;
                }
                return;
            }
            
            // If query engine is available, execute actual query
            if (queryEngine_) {
                // Execute query using ThemisDB QueryEngine
                // Note: This requires SQL-to-AQL translation
                std::string aqlQuery = translateQuery(query);
                
                // For SELECT queries, execute and return results
                std::string upperQuery = query;
                std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
                
                if (upperQuery.find("SELECT") == 0) {
                    // Execute via AQLParser + QueryEngine
                    QueryInfo info = parseSelectQuery(query);
                    std::vector<FieldDescription> fields;
                    fields.reserve(info.selectColumns.size());  // OPTIMIZATION: Pre-allocate to avoid reallocations
                    for (const auto& col : info.selectColumns) {
                        if (col == "*") {
                            fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
                        } else {
                            std::string colName = col;
                            size_t dotPos = colName.find('.');
                            if (dotPos != std::string::npos) {
                                colName = colName.substr(dotPos + 1);
                            }
                            fields.push_back({colName, 0, 0, 25, -1, -1, 0});
                        }
                    }

                    // PG protocol: RowDescription MUST precede any DataRow/CommandComplete
                    if (!fields.empty()) {
                        sendRowDescription(fields);
                    }

                    // Guard: skip AQL execution if table name could not be determined
                    if (info.tableName.empty()) {
                        sendCommandComplete("SELECT 0");
                        portalData.resultsComplete = true;
                        std::lock_guard<std::mutex> lock(portalsMutex_);
                        auto portalIt = portals_.find(portal);
                        if (portalIt != portals_.end()) {
                            portalIt->second = portalData;
                        }
                        return;
                    }

                    // Build a simple AQL: FOR doc IN <table> [FILTER …] RETURN doc
                    std::string aql = "FOR doc IN " + info.tableName + " RETURN doc";
                    if (!info.whereClause.empty()) {
                        aql = "FOR doc IN " + info.tableName +
                              " FILTER " + info.whereClause + " RETURN doc";
                    }

                    size_t row_count = 0;
                    try {
                        query::AQLParser parser;
                        auto parse_res = parser.parse(aql);
                        if (parse_res.has_value()) {
                            auto trans = query::AQLTranslator::translate(*parse_res);
                            if (trans.success) {
                                auto exec_res = queryEngine_->executeAndEntities(trans.query);
                                if (exec_res.has_value()) {
                                    for (const auto& entity : exec_res.value()) {
                                        // Project columns from entity JSON
                                        nlohmann::json doc = nlohmann::json::parse(
                                            entity.toJson());
                                        std::vector<std::string> row_vals;
                                        row_vals.reserve(fields.size());  // OPTIMIZATION: Pre-allocate to avoid reallocations
                                        for (const auto& f : fields) {
                                            if (doc.contains(f.name)) {
                                                const auto& v = doc[f.name];
                                                // Return plain string values without extra quotes
                                                row_vals.push_back(
                                                    v.is_string() ? v.get<std::string>()
                                                                  : v.dump());
                                            } else {
                                                row_vals.push_back("");
                                            }
                                        }
                                        sendDataRow(row_vals);
                                        ++row_count;
                                    }
                                }
                            }
                        }
                    } catch (const std::exception& ex) {
                        // Log and fall through – RowDescription already sent; client will
                        // receive an empty result set with 0 rows
                        std::cerr << "PostgresSession: SELECT execution error: "
                                  << ex.what() << "\n";
                    }

                    sendCommandComplete("SELECT " + std::to_string(row_count));
                    portalData.resultsComplete = true;
                    std::lock_guard<std::mutex> lock(portalsMutex_);
                    auto portalIt = portals_.find(portal);
                    if (portalIt != portals_.end()) {
                        portalIt->second = portalData;
                    }
                    return;
                } else {
                    // Non-SELECT queries (INSERT, UPDATE, DELETE)
                    sendCommandComplete("SELECT 0");
                    portalData.resultsComplete = true;
                    std::lock_guard<std::mutex> lock(portalsMutex_);
                    auto portalIt = portals_.find(portal);
                    if (portalIt != portals_.end()) {
                        portalIt->second = portalData;
                    }
                    return;
                }
            }
            
            // No query engine available - return error
            sendErrorResponse("ERROR", "XX000", 
                "Query execution not available: QueryEngine not initialized. " 
                "This is a protocol-only implementation.");
            portalData.resultsComplete = true;
            std::lock_guard<std::mutex> lock(portalsMutex_);
            auto portalIt = portals_.find(portal);
            if (portalIt != portals_.end()) {
                portalIt->second = portalData;
            }
            return;
        }
        
        // Result streaming: Send cached results up to maxRows
        if (maxRows == 0) {
            // maxRows == 0 means no limit, send all remaining rows
            maxRows = portalData.cachedResults.size() - portalData.currentRow;
        }
        
        size_t rowsToSend = std::min(static_cast<size_t>(maxRows), 
                                      portalData.cachedResults.size() - portalData.currentRow);
        
        for (size_t i = 0; i < rowsToSend; ++i) {
            sendDataRow(portalData.cachedResults[portalData.currentRow + i]);
        }
        
        portalData.currentRow += rowsToSend;
        
        // Check if all results have been sent
        if (portalData.currentRow >= portalData.cachedResults.size()) {
            sendCommandComplete("SELECT " + std::to_string(portalData.cachedResults.size()));
            portalData.resultsComplete = true;
        } else {
            // More rows available, send PortalSuspended
            sendPortalSuspended();
        }
        std::lock_guard<std::mutex> lock(portalsMutex_);
        auto portalIt = portals_.find(portal);
        if (portalIt != portals_.end()) {
            portalIt->second = portalData;
        }
        
    } catch (const std::exception& e) {
        sendErrorResponse("ERROR", "XX000", std::string("Execute error: ") + e.what());
    }
}

void PostgresSession::handleDescribe(char type, const std::string& name) {
    // PostgreSQL Describe message handler
    // Returns description of statement or portal
    
    if (type == 'S') {
        // Describe statement - return ParameterDescription and RowDescription
        PreparedStatement stmt;
        {
            std::lock_guard<std::mutex> lock(preparedStatementsMutex_);
            auto stmtIt = preparedStatements_.find(name);
            if (stmtIt == preparedStatements_.end()) {
                sendErrorResponse("ERROR", "26000", "Prepared statement not found: " + name);
                return;
            }
            stmt = stmtIt->second;
        }
        // Send ParameterDescription
        sendParameterDescription(stmt.paramTypes);
        
        // Parse query to determine result columns
        try {
            std::string query = stmt.query;
            std::string upperQuery = query;
            std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
            
            if (upperQuery.find("SELECT") == 0) {
                // Parse SELECT to get columns
                QueryInfo info = parseSelectQuery(query);
                
                // Build field descriptions
                std::vector<FieldDescription> fields;
                for (const auto& col : info.selectColumns) {
                    if (col == "*") {
                        // Generic field for SELECT *
                        fields.push_back({"?column?", 0, 0, 25, -1, -1, 0}); // text type
                    } else {
                        // Extract column name
                        std::string colName = col;
                        size_t dotPos = colName.find('.');
                        if (dotPos != std::string::npos) {
                            colName = colName.substr(dotPos + 1);
                        }
                        fields.push_back({colName, 0, 0, 25, -1, -1, 0}); // text type
                    }
                }
                
                if (fields.empty()) {
                    fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
                }
                
                sendRowDescription(fields);
            } else {
                // Non-SELECT query - send NoData
                sendNoData();
            }
        } catch (const std::exception& e) {
            // If parsing fails, send generic row description
            std::cerr << "[PostgresSession] handleDescribe (statement): parse error: " << e.what() << "\n";
            std::vector<FieldDescription> fields = {
                {"?column?", 0, 0, 25, -1, -1, 0}
            };
            sendRowDescription(fields);
        }
        
    } else if (type == 'P') {
        // Describe portal - return RowDescription only
        Portal portal;
        {
            std::lock_guard<std::mutex> lock(portalsMutex_);
            auto portalIt = portals_.find(name);
            if (portalIt == portals_.end()) {
                sendErrorResponse("ERROR", "34000", "Portal not found: " + name);
                return;
            }
            portal = portalIt->second;
        }
        PreparedStatement stmt;
        {
            std::lock_guard<std::mutex> lock(preparedStatementsMutex_);
            auto stmtIt = preparedStatements_.find(portal.statementName);
            if (stmtIt == preparedStatements_.end()) {
                sendErrorResponse("ERROR", "26000", "Prepared statement not found");
                return;
            }
            stmt = stmtIt->second;
        }
        // Return same row description as for statement
        try {
            std::string query = stmt.query;
            std::string upperQuery = query;
            std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
            
            if (upperQuery.find("SELECT") == 0) {
                QueryInfo info = parseSelectQuery(query);
                std::vector<FieldDescription> fields;
                fields.reserve(info.selectColumns.size());  // OPTIMIZATION: Pre-allocate to avoid reallocations
                for (const auto& col : info.selectColumns) {
                    if (col == "*") {
                        fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
                    } else {
                        std::string colName = col;
                        size_t dotPos = colName.find('.');
                        if (dotPos != std::string::npos) {
                            colName = colName.substr(dotPos + 1);
                        }
                        fields.push_back({colName, 0, 0, 25, -1, -1, 0});
                    }
                }
                if (fields.empty()) {
                    fields.push_back({"?column?", 0, 0, 25, -1, -1, 0});
                }
                sendRowDescription(fields);
            } else {
                sendNoData();
            }
        } catch (const std::exception& e) {
            std::cerr << "[PostgresSession] handleDescribe (portal): parse error: " << e.what() << "\n";
            std::vector<FieldDescription> fields = {
                {"?column?", 0, 0, 25, -1, -1, 0}
            };
            sendRowDescription(fields);
        }
    }
}

void PostgresSession::handleClose(char type, const std::string& name) {
    // PostgreSQL Close message handler
    // Closes and deallocates a prepared statement or portal
    
    if (type == 'S') {
        // Close statement
        {
            std::lock_guard<std::mutex> lock(preparedStatementsMutex_);
            auto it = preparedStatements_.find(name);
            if (it != preparedStatements_.end()) {
                preparedStatements_.erase(it);
            }
        }
        sendCloseComplete();
    } else if (type == 'P') {
        // Close portal
        {
            std::lock_guard<std::mutex> lock(portalsMutex_);
            auto it = portals_.find(name);
            if (it != portals_.end()) {
                portals_.erase(it);
            }
        }
        sendCloseComplete();
    }
}

void PostgresSession::handleSync() {
    // PostgreSQL Sync message handler
    // Ends extended query protocol flow and reports transaction status
    sendReadyForQuery(currentTransactionStatus());
}

void PostgresSession::handleTerminate() {
    // PostgreSQL Terminate message handler
    stop();
}

void PostgresSession::handleCopyData(const std::vector<uint8_t>& data) {
    // PostgreSQL CopyData message handler
    // Receives data rows during COPY IN operation
    
    if (!copyInProgress_.load(std::memory_order_acquire)) {
        sendErrorResponse("ERROR", "57014", "COPY operation not in progress");
        return;
    }
    
    // Parse the data based on format (text or binary)
    // For text format (CSV/TSV), parse and accumulate rows
    std::string dataStr(data.begin(), data.end());
    
    // Split by newlines to get individual rows
    std::istringstream stream(dataStr);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            std::lock_guard<std::mutex> lock(copyMutex_);
            copyBuffer_.push_back(line);
        }
    }
}

void PostgresSession::handleCopyDone() {
    // PostgreSQL CopyDone message handler
    // Signals end of COPY IN operation
    
    if (!copyInProgress_.load(std::memory_order_acquire)) {
        sendErrorResponse("ERROR", "57014", "COPY operation not in progress");
        return;
    }
    
    // Process accumulated data
    size_t rowsInserted = 0;
    
    std::vector<std::string> copyBuffer;
    std::string copyTableName;
    {
        std::lock_guard<std::mutex> lock(copyMutex_);
        copyBuffer = copyBuffer_;
        copyTableName = copyTableName_;
    }

    if (queryEngine_) {
        // Insert each CSV row from copyBuffer_ as an AQL document.
        // Rows are CSV-formatted; we map each field to a sequential column name
        // since the protocol does not forward column names in COPY FROM STDIN.
        for (const auto& row : copyBuffer) {
            // Parse CSV fields with RFC 4180 quoted field support.
            std::vector<std::string> fields;
            {
                size_t pos = 0;
                const size_t len = row.size();
                while (pos <= len) {
                    std::string field;
                    if (pos < len && row[pos] == '"') {
                        // Quoted field
                        ++pos;
                        while (pos < len) {
                            if (row[pos] == '"') {
                                if (pos + 1 < len && row[pos + 1] == '"') {
                                    field += '"';
                                    pos += 2;
                                } else {
                                    ++pos;
                                    break;
                                }
                            } else {
                                field += row[pos++];
                            }
                        }
                        // Skip optional comma after closing quote
                        if (pos < len && row[pos] == ',') ++pos;
                    } else {
                        // Unquoted field — read until comma or end
                        size_t start = pos;
                        while (pos < len && row[pos] != ',') ++pos;
                        field = row.substr(start, pos - start);
                        // Trim surrounding whitespace
                        auto ltrim = field.find_first_not_of(" \t");
                        auto rtrim = field.find_last_not_of(" \t");
                        if (ltrim == std::string::npos) {
                            field.clear();
                        } else {
                            field = field.substr(ltrim, rtrim - ltrim + 1);
                        }
                        if (pos < len) ++pos;  // skip comma
                    }
                    fields.push_back(field);
                }
            }
            if (fields.empty()) continue;

            // Build JSON document: {col0: "v0", col1: "v1", ...}
            nlohmann::json doc;
            for (size_t i = 0; i < fields.size(); ++i) {
                doc["col" + std::to_string(i)] = fields[i];
            }

            // Build AQL: INSERT <doc> INTO <table>
            std::string aql = "INSERT " + doc.dump() + " INTO " + copyTableName;
            try {
                query::AQLParser parser;
                auto parse_res = parser.parse(aql);
                if (parse_res.has_value()) {
                    auto trans = query::AQLTranslator::translate(*parse_res);
                    if (trans.success) {
                        queryEngine_->executeAndEntities(trans.query);
                        ++rowsInserted;
                    }
                }
            } catch (const std::exception& ex) {
                std::cerr << "PostgresSession COPY INSERT error: " << ex.what() << "\n";
            }
        }
    } else {
        // No query engine - return error
        sendErrorResponse("ERROR", "XX000", 
            "COPY operation not available: QueryEngine not initialized");
        copyInProgress_.store(false, std::memory_order_release);
        std::lock_guard<std::mutex> lock(copyMutex_);
        copyBuffer_.clear();
        copyTableName_.clear();
        return;
    }
    
    // Send CommandComplete with actual row count
    sendCommandComplete("COPY " + std::to_string(rowsInserted));
    
    // Clean up
    copyInProgress_.store(false, std::memory_order_release);
    std::lock_guard<std::mutex> lock(copyMutex_);
    copyBuffer_.clear();
    copyTableName_.clear();
}

void PostgresSession::handleCopyFail(const std::string& message) {
    // PostgreSQL CopyFail message handler
    // Signals that client wants to abort COPY operation
    
    // Clean up any buffered data
    // Send error response if needed
    copyInProgress_.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(copyMutex_);
        copyBuffer_.clear();
        copyTableName_.clear();
    }
    sendErrorResponse("ERROR", "57014", "COPY operation canceled: " + message);
}

// Send methods

void PostgresSession::sendAuthenticationOk() {
    std::vector<uint8_t> payload = {0, 0, 0, 0}; // Authentication OK
    writeMessage('R', payload);
}

void PostgresSession::sendParameterStatus(const std::string& name, const std::string& value) {
    std::vector<uint8_t> payload;
    payload.insert(payload.end(), name.begin(), name.end());
    payload.push_back(0);
    payload.insert(payload.end(), value.begin(), value.end());
    payload.push_back(0);
    writeMessage('S', payload);
}

void PostgresSession::sendBackendKeyData(int32_t processId, int32_t secretKey) {
    std::vector<uint8_t> payload(8);
    // Big-endian encoding
    payload[0] = (processId >> 24) & 0xFF;
    payload[1] = (processId >> 16) & 0xFF;
    payload[2] = (processId >> 8) & 0xFF;
    payload[3] = processId & 0xFF;
    payload[4] = (secretKey >> 24) & 0xFF;
    payload[5] = (secretKey >> 16) & 0xFF;
    payload[6] = (secretKey >> 8) & 0xFF;
    payload[7] = secretKey & 0xFF;
    writeMessage('K', payload);
}

void PostgresSession::sendReadyForQuery(char transactionStatus) {
    std::vector<uint8_t> payload = {static_cast<uint8_t>(transactionStatus)};
    writeMessage('Z', payload);
}

void PostgresSession::sendRowDescription(const std::vector<FieldDescription>& fields) {
    std::vector<uint8_t> payload;
    
    // OPTIMIZATION: Reserve space to avoid repeated reallocations
    // Estimate: 2 (field count) + fields.size() * (19 + avg_name_length)
    size_t estimated_size = 2 + fields.size() * 30;
    payload.reserve(estimated_size);
    
    // Field count
    uint16_t fieldCount = fields.size();
    payload.push_back((fieldCount >> 8) & 0xFF);
    payload.push_back(fieldCount & 0xFF);
    
    for (const auto& field : fields) {
        // Field name (null-terminated)
        payload.insert(payload.end(), field.name.begin(), field.name.end());
        payload.push_back(0);
        
        // Table OID (4 bytes)
        payload.push_back((field.tableOid >> 24) & 0xFF);
        payload.push_back((field.tableOid >> 16) & 0xFF);
        payload.push_back((field.tableOid >> 8) & 0xFF);
        payload.push_back(field.tableOid & 0xFF);
        
        // Column attribute number (2 bytes)
        payload.push_back((field.columnAttrNumber >> 8) & 0xFF);
        payload.push_back(field.columnAttrNumber & 0xFF);
        
        // Data type OID (4 bytes)
        payload.push_back((field.dataTypeOid >> 24) & 0xFF);
        payload.push_back((field.dataTypeOid >> 16) & 0xFF);
        payload.push_back((field.dataTypeOid >> 8) & 0xFF);
        payload.push_back(field.dataTypeOid & 0xFF);
        
        // Data type size (2 bytes, signed)
        payload.push_back((field.dataTypeSize >> 8) & 0xFF);
        payload.push_back(field.dataTypeSize & 0xFF);
        
        // Type modifier (4 bytes, signed)
        payload.push_back((field.typeModifier >> 24) & 0xFF);
        payload.push_back((field.typeModifier >> 16) & 0xFF);
        payload.push_back((field.typeModifier >> 8) & 0xFF);
        payload.push_back(field.typeModifier & 0xFF);
        
        // Format code (2 bytes) - 0=text, 1=binary
        payload.push_back((field.formatCode >> 8) & 0xFF);
        payload.push_back(field.formatCode & 0xFF);
    }
    
    writeMessage('T', payload);
}

void PostgresSession::sendDataRow(const std::vector<std::string>& values) {
    std::vector<uint8_t> payload;
    
    // OPTIMIZATION: Reserve space to avoid repeated reallocations
    // Estimate: 2 (col count) + values.size() * (4 + avg_value_length)
    size_t estimated_size = 2;
    for (const auto& v : values) {
        estimated_size += 4 + v.size();
    }
    payload.reserve(estimated_size);
    
    // Column count
    uint16_t colCount = values.size();
    payload.push_back((colCount >> 8) & 0xFF);
    payload.push_back(colCount & 0xFF);
    
    for (const auto& value : values) {
        // Column length
        int32_t len = value.size();
        payload.push_back((len >> 24) & 0xFF);
        payload.push_back((len >> 16) & 0xFF);
        payload.push_back((len >> 8) & 0xFF);
        payload.push_back(len & 0xFF);
        
        // Column value
        payload.insert(payload.end(), value.begin(), value.end());
    }
    
    writeMessage('D', payload);
}

void PostgresSession::sendDataRowBinary(const std::vector<std::pair<std::vector<uint8_t>, int32_t>>& values) {
    // Send DataRow in binary format
    // Each value is a pair of (binary_data, type_oid)
    std::vector<uint8_t> payload;
    
    // OPTIMIZATION: Reserve space to avoid repeated reallocations
    // Estimate: 2 (col count) + values.size() * (4 + avg_data_length)
    size_t estimated_size = 2;
    for (const auto& [data, _] : values) {
        estimated_size += 4 + data.size();
    }
    payload.reserve(estimated_size);
    
    // Column count
    uint16_t colCount = values.size();
    payload.push_back((colCount >> 8) & 0xFF);
    payload.push_back(colCount & 0xFF);
    
    for (const auto& [data, typeOid] : values) {
        // Column length
        int32_t len = data.size();
        payload.push_back((len >> 24) & 0xFF);
        payload.push_back((len >> 16) & 0xFF);
        payload.push_back((len >> 8) & 0xFF);
        payload.push_back(len & 0xFF);
        
        // Binary column value
        payload.insert(payload.end(), data.begin(), data.end());
    }
    
    writeMessage('D', payload);
}

void PostgresSession::sendPortalSuspended() {
    // Send PortalSuspended message ('s')
    // Indicates that execution was suspended due to maxRows limit
    writeMessage('s', {});
}

void PostgresSession::sendCommandComplete(const std::string& commandTag) {
    std::vector<uint8_t> payload;
    // OPTIMIZATION: Reserve space for string + null terminator
    payload.reserve(commandTag.size() + 1);
    payload.insert(payload.end(), commandTag.begin(), commandTag.end());
    payload.push_back(0);
    writeMessage('C', payload);
}

void PostgresSession::sendParseComplete() {
    writeMessage('1', {});
}

void PostgresSession::sendBindComplete() {
    writeMessage('2', {});
}

void PostgresSession::sendParameterDescription(const std::vector<int32_t>& paramTypes) {
    std::vector<uint8_t> payload;
    
    // OPTIMIZATION: Reserve space: 2 (param count) + paramTypes.size() * 4
    payload.reserve(2 + paramTypes.size() * 4);
    
    // Number of parameters
    uint16_t paramCount = paramTypes.size();
    payload.push_back((paramCount >> 8) & 0xFF);
    payload.push_back(paramCount & 0xFF);
    
    // Parameter type OIDs
    for (int32_t typeOid : paramTypes) {
        payload.push_back((typeOid >> 24) & 0xFF);
        payload.push_back((typeOid >> 16) & 0xFF);
        payload.push_back((typeOid >> 8) & 0xFF);
        payload.push_back(typeOid & 0xFF);
    }
    
    writeMessage('t', payload);
}

void PostgresSession::sendNoData() {
    writeMessage('n', {});
}

void PostgresSession::sendCloseComplete() {
    writeMessage('3', {});
}

void PostgresSession::sendCopyInResponse(const std::vector<int16_t>& formatCodes) {
    // Send CopyInResponse message ('G')
    // Format: overall_format(1) + num_columns(2) + format_codes(2 * N)
    
    std::vector<uint8_t> payload;
    
    // OPTIMIZATION: Reserve space: 1 (format) + 2 (count) + formatCodes.size() * 2
    payload.reserve(3 + formatCodes.size() * 2);
    
    // Overall format: 0 = text, 1 = binary
    uint8_t overallFormat = formatCodes.empty() ? 0 : (formatCodes[0] == 1 ? 1 : 0);
    payload.push_back(overallFormat);
    
    // Number of columns
    uint16_t numColumns = formatCodes.size();
    payload.push_back((numColumns >> 8) & 0xFF);
    payload.push_back(numColumns & 0xFF);
    
    // Format code for each column (0 = text, 1 = binary)
    for (int16_t format : formatCodes) {
        payload.push_back((format >> 8) & 0xFF);
        payload.push_back(format & 0xFF);
    }
    
    writeMessage('G', payload);
}

void PostgresSession::sendCopyOutResponse(const std::vector<int16_t>& formatCodes) {
    // Send CopyOutResponse message ('H')
    // Same format as CopyInResponse
    
    std::vector<uint8_t> payload;
    
    // OPTIMIZATION: Reserve space: 1 (format) + 2 (count) + formatCodes.size() * 2
    payload.reserve(3 + formatCodes.size() * 2);
    
    uint8_t overallFormat = formatCodes.empty() ? 0 : (formatCodes[0] == 1 ? 1 : 0);
    payload.push_back(overallFormat);
    
    uint16_t numColumns = formatCodes.size();
    payload.push_back((numColumns >> 8) & 0xFF);
    payload.push_back(numColumns & 0xFF);
    
    for (int16_t format : formatCodes) {
        payload.push_back((format >> 8) & 0xFF);
        payload.push_back(format & 0xFF);
    }
    
    writeMessage('H', payload);
}

void PostgresSession::sendCopyBothResponse(const std::vector<int16_t>& formatCodes) {
    // Send CopyBothResponse message ('W')
    // Used for replication
    
    std::vector<uint8_t> payload;
    
    // OPTIMIZATION: Reserve space: 1 (format) + 2 (count) + formatCodes.size() * 2
    payload.reserve(3 + formatCodes.size() * 2);
    
    uint8_t overallFormat = formatCodes.empty() ? 0 : (formatCodes[0] == 1 ? 1 : 0);
    payload.push_back(overallFormat);
    
    uint16_t numColumns = formatCodes.size();
    payload.push_back((numColumns >> 8) & 0xFF);
    payload.push_back(numColumns & 0xFF);
    
    for (int16_t format : formatCodes) {
        payload.push_back((format >> 8) & 0xFF);
        payload.push_back(format & 0xFF);
    }
    
    writeMessage('W', payload);
}

void PostgresSession::sendCopyData(const std::vector<uint8_t>& data) {
    // Send CopyData message ('d')
    // Just the raw data bytes
    writeMessage('d', data);
}

void PostgresSession::sendCopyDone() {
    // Send CopyDone message ('c')
    // No payload
    writeMessage('c', {});
}

void PostgresSession::sendErrorResponse(const std::string& severity, const std::string& code, 
                                       const std::string& message) {
    std::vector<uint8_t> payload;
    
    // OPTIMIZATION: Reserve space: 1(S) + severity.size() + 1(null) + 1(C) + code.size() + 1(null) 
    //                              + 1(M) + message.size() + 1(null) + 1(terminator)
    payload.reserve(8 + severity.size() + code.size() + message.size());
    
    // Severity
    payload.push_back('S');
    payload.insert(payload.end(), severity.begin(), severity.end());
    payload.push_back(0);
    
    // Code
    payload.push_back('C');
    payload.insert(payload.end(), code.begin(), code.end());
    payload.push_back(0);
    
    // Message
    payload.push_back('M');
    payload.insert(payload.end(), message.begin(), message.end());
    payload.push_back(0);
    
    // Terminator
    payload.push_back(0);
    
    writeMessage('E', payload);
}

void PostgresSession::doRead() {
    auto self = shared_from_this();
    armReadTimeout();
    
    socket_.async_read_some(asio::buffer(buffer_),
        [this, self](boost::beast::error_code ec, std::size_t bytes_transferred) {
            cancelReadTimeout();
            if (ec || stopped_.load(std::memory_order_acquire)) {
                stop();
                return;
            }
            
            if (bytes_transferred < 5) {
                doRead(); // Need more data
                return;
            }
            
            size_t offset = 0;
            
            if (inStartup_.load(std::memory_order_acquire)) {
                // Startup message has no type byte, just length
                int32_t length = (buffer_[0] << 24) | (buffer_[1] << 16) |
                               (buffer_[2] << 8) | buffer_[3];
                
                if (bytes_transferred < static_cast<size_t>(length)) {
                    doRead(); // Need more data
                    return;
                }
                
                int32_t protocolVersion = (buffer_[4] << 24) | (buffer_[5] << 16) |
                                         (buffer_[6] << 8) | buffer_[7];
                
                // Parse parameters (null-terminated strings)
                std::map<std::string, std::string> params;
                offset = 8;
                while (offset < static_cast<size_t>(length) && buffer_[offset] != 0) {
                    std::string key(buffer_.data() + offset);
                    offset += key.size() + 1;
                    if (offset >= static_cast<size_t>(length)) break;
                    std::string value(buffer_.data() + offset);
                    offset += value.size() + 1;
                    params[key] = value;
                }
                
                handleStartupMessage(protocolVersion, params);
            } else {
                // Regular messages have type byte + length
                char messageType = buffer_[0];
                int32_t length = (buffer_[1] << 24) | (buffer_[2] << 16) |
                               (buffer_[3] << 8) | buffer_[4];
                
                if (bytes_transferred < static_cast<size_t>(length) + 1) {
                    doRead(); // Need more data
                    return;
                }
                
                offset = 5; // Skip type and length
                
                try {
                switch (messageType) {
                    case 'Q': { // Simple Query
                        std::string query(buffer_.data() + offset);
                        handleQuery(query);
                        break;
                    }
                    case 'P': { // Parse
                        std::string stmtName(buffer_.data() + offset);
                        offset += stmtName.size() + 1;
                        std::string query(buffer_.data() + offset);
                        offset += query.size() + 1;
                        
                        // Parse parameter types
                        std::vector<int32_t> paramTypes;
                        if (offset + 2 <= bytes_transferred) {
                            uint16_t numParams = (static_cast<uint8_t>(buffer_[offset]) << 8) | 
                                               static_cast<uint8_t>(buffer_[offset + 1]);
                            offset += 2;
                            
                            for (uint16_t i = 0; i < numParams && offset + 4 <= bytes_transferred; ++i) {
                                int32_t typeOid = (static_cast<uint8_t>(buffer_[offset]) << 24) |
                                                (static_cast<uint8_t>(buffer_[offset + 1]) << 16) |
                                                (static_cast<uint8_t>(buffer_[offset + 2]) << 8) |
                                                static_cast<uint8_t>(buffer_[offset + 3]);
                                paramTypes.push_back(typeOid);
                                offset += 4;
                            }
                        }
                        
                        handleParse(stmtName, query, paramTypes);
                        break;
                    }
                    case 'B': { // Bind
                        std::string portalName(buffer_.data() + offset);
                        offset += portalName.size() + 1;
                        std::string stmtName(buffer_.data() + offset);
                        offset += stmtName.size() + 1;
                        
                        // Parse parameter format codes
                        std::vector<int16_t> paramFormats;
                        if (offset + 2 <= bytes_transferred) {
                            uint16_t numFormats = (static_cast<uint8_t>(buffer_[offset]) << 8) | 
                                                static_cast<uint8_t>(buffer_[offset + 1]);
                            offset += 2;
                            
                            for (uint16_t i = 0; i < numFormats && offset + 2 <= bytes_transferred; ++i) {
                                int16_t format = (static_cast<uint8_t>(buffer_[offset]) << 8) | 
                                               static_cast<uint8_t>(buffer_[offset + 1]);
                                paramFormats.push_back(format);
                                offset += 2;
                            }
                        }
                        
                        // Parse parameter values
                        std::vector<std::string> params;
                        if (offset + 2 <= bytes_transferred) {
                            uint16_t numParams = (static_cast<uint8_t>(buffer_[offset]) << 8) | 
                                               static_cast<uint8_t>(buffer_[offset + 1]);
                            offset += 2;
                            
                            for (uint16_t i = 0; i < numParams && offset + 4 <= bytes_transferred; ++i) {
                                int32_t paramLen = (static_cast<uint8_t>(buffer_[offset]) << 24) |
                                                 (static_cast<uint8_t>(buffer_[offset + 1]) << 16) |
                                                 (static_cast<uint8_t>(buffer_[offset + 2]) << 8) |
                                                 static_cast<uint8_t>(buffer_[offset + 3]);
                                offset += 4;
                                
                                if (paramLen == -1) {
                                    // NULL parameter
                                    params.push_back("NULL");
                                } else if (paramLen >= 0 && offset + paramLen <= bytes_transferred) {
                                    // Non-NULL parameter
                                    std::string param(buffer_.data() + offset, paramLen);
                                    params.push_back(param);
                                    offset += paramLen;
                                }
                            }
                        }
                        
                        handleBind(portalName, stmtName, params);
                        break;
                    }
                    case 'E': { // Execute
                        // Guard: need at least 1 byte for portalName (the null terminator)
                        // and 4 bytes for maxRows after it.
                        if (offset >= bytes_transferred) {
                            sendErrorResponse("ERROR", "08P01", "Malformed Execute message: missing portal name");
                            break;
                        }
                        std::string portalName(buffer_.data() + offset);
                        offset += portalName.size() + 1;
                        // Guard: need 4 bytes for the maxRows int32.
                        if (offset + 4 > bytes_transferred) {
                            sendErrorResponse("ERROR", "08P01", "Malformed Execute message: missing maxRows field");
                            break;
                        }
                        int32_t maxRows = (static_cast<uint8_t>(buffer_[offset]) << 24)
                                        | (static_cast<uint8_t>(buffer_[offset+1]) << 16)
                                        | (static_cast<uint8_t>(buffer_[offset+2]) << 8)
                                        | static_cast<uint8_t>(buffer_[offset+3]);
                        handleExecute(portalName, maxRows);
                        break;
                    }
                    case 'D': { // Describe
                        // Guard: need 1 byte for descType + at least 1 byte (null) for name.
                        if (offset + 2 > bytes_transferred) {
                            sendErrorResponse("ERROR", "08P01", "Malformed Describe message");
                            break;
                        }
                        char descType = buffer_[offset];
                        std::string name(buffer_.data() + offset + 1);
                        handleDescribe(descType, name);
                        break;
                    }
                    case 'C': { // Close
                        // Guard: same layout as Describe.
                        if (offset + 2 > bytes_transferred) {
                            sendErrorResponse("ERROR", "08P01", "Malformed Close message");
                            break;
                        }
                        char closeType = buffer_[offset];
                        std::string name(buffer_.data() + offset + 1);
                        handleClose(closeType, name);
                        break;
                    }
                    case 'S': // Sync
                        handleSync();
                        break;
                    case 'X': // Terminate
                        handleTerminate();
                        return;
                    case 'd': { // CopyData
                        // Copy data from client
                        std::vector<uint8_t> copyData(buffer_.data() + offset, 
                                                     buffer_.data() + length + 1);
                        handleCopyData(copyData);
                        break;
                    }
                    case 'c': // CopyDone
                        handleCopyDone();
                        break;
                    case 'f': { // CopyFail
                        std::string errorMsg(buffer_.data() + offset);
                        handleCopyFail(errorMsg);
                        break;
                    }
                    default:
                        break;
                }
                } catch (const std::exception& e) {
                    std::cerr << "[PostgresSession] Message handler error (type='" << messageType << "'): " << e.what() << "\n";
                    sendErrorResponse("ERROR", "XX000", std::string("Internal error: ") + e.what());
                } catch (...) {
                    std::cerr << "[PostgresSession] Message handler unknown error (type='" << messageType << "')\n";
                    sendErrorResponse("ERROR", "XX000", "Internal error: unknown exception");
                }
            }
            
            doRead(); // Continue reading
        });
}

void PostgresSession::doWrite() {
    std::shared_ptr<std::vector<uint8_t>> message;
    {
        std::lock_guard<std::mutex> lock(writeMutex_);
        if (writeQueue_.empty()) {
            writeInProgress_ = false;
            return;
        }
        message = std::make_shared<std::vector<uint8_t>>(writeQueue_.front());
    }

    auto self = shared_from_this();
    armWriteTimeout();
    asio::async_write(socket_, asio::buffer(*message),
        [this, self, message](boost::beast::error_code ec, std::size_t /*bytes_transferred*/) {
            try {
                cancelWriteTimeout();
                if (ec || stopped_.load(std::memory_order_acquire)) {
                    stop();
                    return;
                }

                bool shouldContinue = false;
                {
                    std::lock_guard<std::mutex> lock(writeMutex_);
                    if (!writeQueue_.empty()) {
                        writeQueue_.pop_front();
                    }
                    shouldContinue = !writeQueue_.empty();
                    if (!shouldContinue) {
                        writeInProgress_ = false;
                    }
                }
                if (shouldContinue) {
                    doWrite();
                }
            } catch (const std::exception& e) {
                std::cerr << "[PostgresSession] Write completion handler error: " << e.what() << "\n";
                stop();
            } catch (...) {
                THEMIS_WARN("postgres_session: unhandled exception caught");
                logCurrentException("Write completion handler error");
                stop();
            }
        });
}

void PostgresSession::writeMessage(char type, const std::vector<uint8_t>& payload) {
    // Format PostgreSQL wire protocol message
    // Format: [Type(1 byte), Length(4 bytes, big-endian), Payload]
    // Length includes the 4 bytes of the length field itself
    
    std::vector<uint8_t> message;
    message.push_back(type);
    
    int32_t length = payload.size() + 4;
    message.push_back((length >> 24) & 0xFF);
    message.push_back((length >> 16) & 0xFF);
    message.push_back((length >> 8) & 0xFF);
    message.push_back(length & 0xFF);
    
    message.insert(message.end(), payload.begin(), payload.end());

    auto weak_self = weak_from_this();
    if (auto self = weak_self.lock()) {
        asio::dispatch(socket_.get_executor(), [self, message = std::move(message)]() mutable {
            self->enqueueWrite(std::move(message));
        });
        return;
    }

    enqueueWrite(std::move(message));
}

void PostgresSession::enqueueWrite(std::vector<uint8_t> message) {
    if (stopped_.load(std::memory_order_acquire)) {
        return;
    }

    bool shouldStartWrite = false;
    {
        std::lock_guard<std::mutex> lock(writeMutex_);
        shouldStartWrite = !writeInProgress_ && writeQueue_.empty();
        writeQueue_.push_back(std::move(message));
        if (shouldStartWrite) {
            writeInProgress_ = true;
        }
    }

    if (shouldStartWrite) {
        doWrite();
    }
}

bool PostgresSession::isSchemaQuery(const std::string& query) {
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    return lowerQuery.find("pg_catalog") != std::string::npos ||
           lowerQuery.find("information_schema") != std::string::npos ||
           lowerQuery.find("pg_type") != std::string::npos ||
           lowerQuery.find("pg_class") != std::string::npos ||
           lowerQuery.find("pg_namespace") != std::string::npos ||
           lowerQuery.find("pg_attribute") != std::string::npos ||
           lowerQuery.find("pg_database") != std::string::npos;
}

void PostgresSession::handleSchemaQuery(const std::string& query) {
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    // Handle common BI tool schema introspection queries
    // These provide PostgreSQL-compatible system catalog responses
    // In a full implementation with QueryEngine, these would query actual database metadata
    
    if (lowerQuery.find("pg_catalog.pg_type") != std::string::npos) {
        // Return PostgreSQL type information for common types
        std::vector<FieldDescription> fields = {
            {"oid", 0, 0, 26, 4, -1, 0},
            {"typname", 0, 0, 19, 64, -1, 0},
            {"typlen", 0, 0, 21, 2, -1, 0}
        };
        sendRowDescription(fields);
        // Standard PostgreSQL types required for BI tool compatibility
        sendDataRow({"16", "bool", "1"});
        sendDataRow({"20", "int8", "8"});
        sendDataRow({"21", "int2", "2"});
        sendDataRow({"23", "int4", "4"});
        sendDataRow({"25", "text", "-1"});
        sendDataRow({"1043", "varchar", "-1"});
        sendDataRow({"700", "float4", "4"});
        sendDataRow({"701", "float8", "8"});
        sendDataRow({"1082", "date", "4"});
        sendDataRow({"1114", "timestamp", "8"});
        sendCommandComplete("SELECT 10");
    } else if (lowerQuery.find("pg_catalog.pg_namespace") != std::string::npos) {
        // Return schema/namespace information
        std::vector<FieldDescription> fields = {
            {"oid", 0, 0, 26, 4, -1, 0},
            {"nspname", 0, 0, 19, 64, -1, 0}
        };
        sendRowDescription(fields);
        // Standard PostgreSQL namespaces
        sendDataRow({"2200", "public"});
        sendDataRow({"11", "pg_catalog"});
        sendDataRow({"99", "pg_toast"});
        sendDataRow({"2200", "information_schema"});
        sendCommandComplete("SELECT 4");
    } else if (lowerQuery.find("pg_catalog.pg_class") != std::string::npos) {
        // Return table information
        // Note: In production with QueryEngine, this would query actual table metadata
        std::vector<FieldDescription> fields = {
            {"oid", 0, 0, 26, 4, -1, 0},
            {"relname", 0, 0, 19, 64, -1, 0},
            {"relkind", 0, 0, 18, 1, -1, 0},
            {"relnamespace", 0, 0, 26, 4, -1, 0}
        };
        sendRowDescription(fields);
        
        if (queryEngine_) {
            // Query actual collections from the database via the key-schema scan.
            auto collections = queryEngine_->listCollections();
            int oid = 16384;
            for (const auto& col : collections) {
                sendDataRow({std::to_string(oid++), col, "r", "2200"});
            }
            if (collections.empty()) {
                // No data yet: return a sentinel row so BI tools see the structure.
                sendDataRow({"16384", "themisdb_default", "r", "2200"});
                sendCommandComplete("SELECT 1");
            } else {
                sendCommandComplete("SELECT " + std::to_string(collections.size()));
            }
        } else {
            // No query engine - return empty result
            sendCommandComplete("SELECT 0");
        }
    } else if (lowerQuery.find("pg_catalog.pg_attribute") != std::string::npos) {
        // Return column information
        std::vector<FieldDescription> fields = {
            {"attrelid", 0, 0, 26, 4, -1, 0},
            {"attname", 0, 0, 19, 64, -1, 0},
            {"atttypid", 0, 0, 26, 4, -1, 0},
            {"attnum", 0, 0, 21, 2, -1, 0}
        };
        sendRowDescription(fields);
        
        if (queryEngine_) {
            // Sample the first document from each collection to derive column names.
            // This gives best-effort schema introspection without requiring separate
            // metadata storage.
            auto collections = queryEngine_->listCollections();
            int total_cols = 0;
            int oid = 16384;
            for (const auto& col : collections) {
                std::string aql = "FOR doc IN " + col + " LIMIT 1 RETURN doc";
                try {
                    query::AQLParser parser;
                    auto parse_res = parser.parse(aql);
                    if (parse_res.has_value()) {
                        auto trans = query::AQLTranslator::translate(*parse_res);
                        if (trans.success) {
                            auto exec_res = queryEngine_->executeAndEntities(trans.query);
                            if (exec_res.has_value() && !exec_res.value().empty()) {
                                nlohmann::json doc = nlohmann::json::parse(
                                    exec_res.value().front().toJson());
                                int attnum = 1;
                                for (auto it = doc.begin(); it != doc.end(); ++it, ++attnum) {
                                    // atttypid 25 = text (generic fallback)
                                    sendDataRow({std::to_string(oid), it.key(), "25",
                                                 std::to_string(attnum)});
                                    ++total_cols;
                                }
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[PostgresSession] pg_attribute query: document parse error: " << e.what() << "\n";
                }
                ++oid;
            }
            sendCommandComplete("SELECT " + std::to_string(total_cols));
        } else {
            // No query engine - return empty result
            sendCommandComplete("SELECT 0");
        }
    } else if (lowerQuery.find("information_schema.tables") != std::string::npos) {
        // INFORMATION_SCHEMA.TABLES
        std::vector<FieldDescription> fields = {
            {"table_catalog", 0, 0, 19, 64, -1, 0},
            {"table_schema", 0, 0, 19, 64, -1, 0},
            {"table_name", 0, 0, 19, 64, -1, 0},
            {"table_type", 0, 0, 19, 64, -1, 0}
        };
        sendRowDescription(fields);
        sendDataRow({"themisdb", "public", "users", "BASE TABLE"});
        sendDataRow({"themisdb", "public", "orders", "BASE TABLE"});
        sendDataRow({"themisdb", "public", "products", "BASE TABLE"});
        sendCommandComplete("SELECT 3");
    } else if (lowerQuery.find("information_schema.columns") != std::string::npos) {
        // INFORMATION_SCHEMA.COLUMNS
        std::vector<FieldDescription> fields = {
            {"table_name", 0, 0, 19, 64, -1, 0},
            {"column_name", 0, 0, 19, 64, -1, 0},
            {"data_type", 0, 0, 19, 64, -1, 0},
            {"ordinal_position", 0, 0, 23, 4, -1, 0}
        };
        sendRowDescription(fields);
        sendDataRow({"users", "id", "integer", "1"});
        sendDataRow({"users", "name", "text", "2"});
        sendDataRow({"users", "email", "text", "3"});
        sendCommandComplete("SELECT 3");
    } else {
        // Generic schema query - return empty result
        sendCommandComplete("SELECT 0");
    }
}

PostgresSession::QueryInfo PostgresSession::parseSelectQuery(const std::string& query) {
    QueryInfo info;
    info.type = "SELECT";
    
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
    
    // Find SELECT clause
    size_t selectPos = upperQuery.find("SELECT");
    size_t fromPos = upperQuery.find("FROM");
    
    if (selectPos == std::string::npos || fromPos == std::string::npos) {
        throw std::runtime_error("Invalid SELECT query");
    }
    
    // Extract columns
    std::string columnsStr = query.substr(selectPos + 6, fromPos - selectPos - 6);
    // Trim whitespace
    columnsStr.erase(0, columnsStr.find_first_not_of(" \t\n\r"));
    columnsStr.erase(columnsStr.find_last_not_of(" \t\n\r") + 1);
    
    // Simple column parsing (split by comma)
    size_t start = 0;
    size_t end = columnsStr.find(',');
    while (end != std::string::npos) {
        std::string col = columnsStr.substr(start, end - start);
        col.erase(0, col.find_first_not_of(" \t\n\r"));
        col.erase(col.find_last_not_of(" \t\n\r") + 1);
        
        // Check for aggregates
        if (col.find("COUNT(") != std::string::npos || col.find("count(") != std::string::npos ||
            col.find("SUM(") != std::string::npos || col.find("sum(") != std::string::npos ||
            col.find("AVG(") != std::string::npos || col.find("avg(") != std::string::npos ||
            col.find("MIN(") != std::string::npos || col.find("min(") != std::string::npos ||
            col.find("MAX(") != std::string::npos || col.find("max(") != std::string::npos) {
            info.aggregates.push_back(col);
        }
        info.selectColumns.push_back(col);
        
        start = end + 1;
        end = columnsStr.find(',', start);
    }
    // Last column
    std::string col = columnsStr.substr(start);
    col.erase(0, col.find_first_not_of(" \t\n\r"));
    col.erase(col.find_last_not_of(" \t\n\r") + 1);
    if (col.find("COUNT(") != std::string::npos || col.find("count(") != std::string::npos ||
        col.find("SUM(") != std::string::npos || col.find("sum(") != std::string::npos ||
        col.find("AVG(") != std::string::npos || col.find("avg(") != std::string::npos ||
        col.find("MIN(") != std::string::npos || col.find("min(") != std::string::npos ||
        col.find("MAX(") != std::string::npos || col.find("max(") != std::string::npos) {
        info.aggregates.push_back(col);
    }
    info.selectColumns.push_back(col);
    
    // Extract table name
    size_t wherePos = upperQuery.find("WHERE", fromPos);
    size_t orderPos = upperQuery.find("ORDER BY", fromPos);
    size_t groupPos = upperQuery.find("GROUP BY", fromPos);
    size_t limitPos = upperQuery.find("LIMIT", fromPos);
    size_t joinPos = upperQuery.find("JOIN", fromPos);
    
    size_t tableEnd = std::string::npos;
    if (wherePos != std::string::npos) tableEnd = wherePos;
    else if (joinPos != std::string::npos) tableEnd = joinPos;
    else if (groupPos != std::string::npos) tableEnd = groupPos;
    else if (orderPos != std::string::npos) tableEnd = orderPos;
    else if (limitPos != std::string::npos) tableEnd = limitPos;
    else tableEnd = query.size();
    
    std::string tableName = query.substr(fromPos + 4, tableEnd - fromPos - 4);
    tableName.erase(0, tableName.find_first_not_of(" \t\n\r"));
    tableName.erase(tableName.find_last_not_of(" \t\n\r") + 1);
    info.tableName = tableName;
    
    // Extract WHERE clause
    if (wherePos != std::string::npos) {
        size_t whereEnd = std::string::npos;
        if (groupPos != std::string::npos && groupPos > wherePos) whereEnd = groupPos;
        else if (orderPos != std::string::npos && orderPos > wherePos) whereEnd = orderPos;
        else if (limitPos != std::string::npos && limitPos > wherePos) whereEnd = limitPos;
        else whereEnd = query.size();
        
        info.whereClause = query.substr(wherePos + 5, whereEnd - wherePos - 5);
        info.whereClause.erase(0, info.whereClause.find_first_not_of(" \t\n\r"));
        info.whereClause.erase(info.whereClause.find_last_not_of(" \t\n\r") + 1);
    }
    
    // Extract ORDER BY
    if (orderPos != std::string::npos) {
        size_t orderEnd = limitPos != std::string::npos ? limitPos : query.size();
        info.orderBy = query.substr(orderPos + 8, orderEnd - orderPos - 8);
        info.orderBy.erase(0, info.orderBy.find_first_not_of(" \t\n\r"));
        info.orderBy.erase(info.orderBy.find_last_not_of(" \t\n\r") + 1);
    }
    
    // Extract GROUP BY
    if (groupPos != std::string::npos) {
        size_t groupEnd = std::string::npos;
        if (orderPos != std::string::npos && orderPos > groupPos) groupEnd = orderPos;
        else if (limitPos != std::string::npos && limitPos > groupPos) groupEnd = limitPos;
        else groupEnd = query.size();
        
        info.groupBy = query.substr(groupPos + 8, groupEnd - groupPos - 8);
        info.groupBy.erase(0, info.groupBy.find_first_not_of(" \t\n\r"));
        info.groupBy.erase(info.groupBy.find_last_not_of(" \t\n\r") + 1);
    }
    
    // Extract LIMIT
    if (limitPos != std::string::npos) {
        std::string limitStr = query.substr(limitPos + 5);
        limitStr.erase(0, limitStr.find_first_not_of(" \t\n\r"));
        info.limit = std::stoi(limitStr);
    }
    
    // Extract JOIN
    if (joinPos != std::string::npos) {
        size_t joinEnd = wherePos != std::string::npos ? wherePos : query.size();
        std::string joinClause = query.substr(joinPos, joinEnd - joinPos);
        // Basic JOIN parsing - extract table name
        // Full JOIN support would require more complex parsing
        size_t onPos = joinClause.find(" ON ");
        if (onPos != std::string::npos) {
            std::string tablePart = joinClause.substr(0, onPos);
            // Extract table name (skip "JOIN" keyword)
            size_t tableStart = tablePart.find("JOIN") + 4;
            tablePart = tablePart.substr(tableStart);
            tablePart.erase(0, tablePart.find_first_not_of(" \t\n\r"));
            tablePart.erase(tablePart.find_last_not_of(" \t\n\r") + 1);
            info.joinTable = tablePart;
        }
    }
    
    return info;
}

std::string PostgresSession::buildCypherFromSelect(const QueryInfo& info) {
    std::string cypher = "MATCH (n:" + info.tableName + ")";
    
    // Add WHERE clause
    if (!info.whereClause.empty()) {
        // Simple WHERE translation: convert SQL comparisons to Cypher property access
        std::string whereClause = info.whereClause;
        
        // Replace table.column with n.column
        size_t pos = 0;
        while ((pos = whereClause.find(info.tableName + ".", pos)) != std::string::npos) {
            whereClause.replace(pos, info.tableName.length() + 1, "n.");
            pos += 2;
        }
        
        cypher += " WHERE " + whereClause;
    }
    
    // Build RETURN clause
    cypher += " RETURN ";
    
    if (!info.aggregates.empty()) {
        // Handle aggregates
        for (size_t i = 0; i < info.aggregates.size(); ++i) {
            if (i > 0) cypher += ", ";
            
            std::string agg = info.aggregates[i];
            // Convert SQL aggregate to Cypher (e.g., COUNT(*) -> count(n))
            if (agg.find("COUNT(*)") != std::string::npos || agg.find("count(*)") != std::string::npos) {
                cypher += "count(n)";
            } else if (agg.find("COUNT(") != std::string::npos || agg.find("count(") != std::string::npos) {
                // Extract column name
                size_t start = agg.find('(') + 1;
                size_t end = agg.find(')');
                std::string col = agg.substr(start, end - start);
                cypher += "count(n." + col + ")";
            } else if (agg.find("SUM(") != std::string::npos || agg.find("sum(") != std::string::npos) {
                size_t start = agg.find('(') + 1;
                size_t end = agg.find(')');
                std::string col = agg.substr(start, end - start);
                cypher += "sum(n." + col + ")";
            } else if (agg.find("AVG(") != std::string::npos || agg.find("avg(") != std::string::npos) {
                size_t start = agg.find('(') + 1;
                size_t end = agg.find(')');
                std::string col = agg.substr(start, end - start);
                cypher += "avg(n." + col + ")";
            } else if (agg.find("MIN(") != std::string::npos || agg.find("min(") != std::string::npos) {
                size_t start = agg.find('(') + 1;
                size_t end = agg.find(')');
                std::string col = agg.substr(start, end - start);
                cypher += "min(n." + col + ")";
            } else if (agg.find("MAX(") != std::string::npos || agg.find("max(") != std::string::npos) {
                size_t start = agg.find('(') + 1;
                size_t end = agg.find(')');
                std::string col = agg.substr(start, end - start);
                cypher += "max(n." + col + ")";
            }
        }
    } else if (info.selectColumns.size() == 1 && info.selectColumns[0] == "*") {
        cypher += "n";
    } else {
        // Regular columns
        for (size_t i = 0; i < info.selectColumns.size(); ++i) {
            if (i > 0) cypher += ", ";
            std::string col = info.selectColumns[i];
            if (col == "*") {
                cypher += "n";
            } else {
                cypher += "n." + col;
            }
        }
    }
    
    // Add ORDER BY
    if (!info.orderBy.empty()) {
        std::string orderBy = info.orderBy;
        // Replace column references with n.column
        size_t pos = 0;
        while ((pos = orderBy.find(info.tableName + ".", pos)) != std::string::npos) {
            orderBy.replace(pos, info.tableName.length() + 1, "n.");
            pos += 2;
        }
        cypher += " ORDER BY " + orderBy;
    }
    
    // Add LIMIT
    if (info.limit > 0) {
        cypher += " LIMIT " + std::to_string(info.limit);
    }
    
    return cypher;
}

// Parse INSERT INTO statement
std::string PostgresSession::parseInsertQuery(const std::string& query) {
    // INSERT INTO table (col1, col2, ...) VALUES (val1, val2, ...)
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
    
    size_t intoPos = upperQuery.find("INTO");
    if (intoPos == std::string::npos) {
        throw std::runtime_error("Invalid INSERT statement: missing INTO");
    }
    
    // Extract table name
    size_t tableStart = intoPos + 4;
    while (tableStart < query.size() && std::isspace(query[tableStart])) tableStart++;
    
    size_t tableEnd = tableStart;
    while (tableEnd < query.size() && !std::isspace(query[tableEnd]) && query[tableEnd] != '(') tableEnd++;
    
    std::string tableName = query.substr(tableStart, tableEnd - tableStart);
    
    // Extract columns
    size_t colsStart = query.find('(', tableEnd);
    size_t colsEnd = query.find(')', colsStart);
    if (colsStart == std::string::npos || colsEnd == std::string::npos) {
        throw std::runtime_error("Invalid INSERT statement: missing column list");
    }
    
    std::string colsList = query.substr(colsStart + 1, colsEnd - colsStart - 1);
    std::vector<std::string> columns;
    size_t pos = 0;
    while (pos < colsList.size()) {
        size_t commaPos = colsList.find(',', pos);
        if (commaPos == std::string::npos) commaPos = colsList.size();
        
        std::string col = colsList.substr(pos, commaPos - pos);
        col.erase(0, col.find_first_not_of(" \t"));
        col.erase(col.find_last_not_of(" \t") + 1);
        columns.push_back(col);
        
        pos = commaPos + 1;
    }
    
    // Extract values
    size_t valuesPos = upperQuery.find("VALUES", colsEnd);
    if (valuesPos == std::string::npos) {
        throw std::runtime_error("Invalid INSERT statement: missing VALUES");
    }
    
    size_t valsStart = query.find('(', valuesPos);
    size_t valsEnd = query.find(')', valsStart);
    if (valsStart == std::string::npos || valsEnd == std::string::npos) {
        throw std::runtime_error("Invalid INSERT statement: missing values list");
    }
    
    std::string valsList = query.substr(valsStart + 1, valsEnd - valsStart - 1);
    std::vector<std::string> values;
    pos = 0;
    bool inQuote = false;
    std::string currentValue;
    char prevChar = '\0';
    
    for (char c : valsList) {
        // Handle SQL string literals with '' escape sequence
        if (c == '\'' && !inQuote) {
            inQuote = true;
            currentValue += c;
        } else if (c == '\'' && inQuote) {
            // Check if this is an escaped quote (two consecutive quotes)
            currentValue += c;
            // Peek ahead would require iterating differently, so we accept this limitation
            inQuote = false;
        } else if (c == ',' && !inQuote) {
            currentValue.erase(0, currentValue.find_first_not_of(" \t"));
            currentValue.erase(currentValue.find_last_not_of(" \t") + 1);
            values.push_back(currentValue);
            currentValue.clear();
        } else {
            currentValue += c;
        }
        prevChar = c;
    }
    if (!currentValue.empty()) {
        currentValue.erase(0, currentValue.find_first_not_of(" \t"));
        currentValue.erase(currentValue.find_last_not_of(" \t") + 1);
        values.push_back(currentValue);
    }
    
    // Build Cypher CREATE statement
    std::string cypher = "CREATE (n:" + tableName + " {";
    for (size_t i = 0; i < columns.size() && i < values.size(); ++i) {
        if (i > 0) cypher += ", ";
        cypher += columns[i] + ": " + values[i];
    }
    cypher += "})";
    
    return cypher;
}

// Parse UPDATE statement
std::string PostgresSession::parseUpdateQuery(const std::string& query) {
    // UPDATE table SET col1=val1, col2=val2 WHERE condition
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
    
    size_t updatePos = upperQuery.find("UPDATE");
    size_t setPos = upperQuery.find("SET", updatePos);
    size_t wherePos = upperQuery.find("WHERE", setPos);
    
    if (setPos == std::string::npos) {
        throw std::runtime_error("Invalid UPDATE statement: missing SET");
    }
    
    // Extract table name - use upperQuery for finding but query for extraction
    size_t tableStart = updatePos + 6; // Length of "UPDATE"
    std::string tableName = query.substr(tableStart, setPos - tableStart);
    tableName.erase(0, tableName.find_first_not_of(" \t\n\r"));
    tableName.erase(tableName.find_last_not_of(" \t\n\r") + 1);
    
    // Extract SET clause
    size_t setEnd = (wherePos != std::string::npos) ? wherePos : query.size();
    std::string setClause = query.substr(setPos + 3, setEnd - setPos - 3);
    setClause.erase(0, setClause.find_first_not_of(" \t\n\r"));
    setClause.erase(setClause.find_last_not_of(" \t\n\r") + 1);
    
    // Build Cypher MATCH...SET statement
    std::string cypher = "MATCH (n:" + tableName + ")";
    
    // Add WHERE clause if present
    if (wherePos != std::string::npos) {
        std::string whereClause = query.substr(wherePos + 5);
        whereClause.erase(0, whereClause.find_first_not_of(" \t\n\r"));
        whereClause.erase(whereClause.find_last_not_of(" \t\n\r;") + 1);
        
        // Replace table.column with n.column
        size_t pos = 0;
        while ((pos = whereClause.find(tableName + ".", pos)) != std::string::npos) {
            whereClause.replace(pos, tableName.length() + 1, "n.");
            pos += 2;
        }
        
        cypher += " WHERE " + whereClause;
    }
    
    // Add SET clause - simple approach: prepend n. only to left-hand side of assignments
    // This handles cases like "SET price = price * 1.1" correctly
    std::string cypherSetClause = setClause;
    
    // Replace explicit table.column references
    size_t pos = 0;
    while ((pos = cypherSetClause.find(tableName + ".", pos)) != std::string::npos) {
        cypherSetClause.replace(pos, tableName.length() + 1, "n.");
        pos += 2;
    }
    
    // For assignments, only prefix the LHS column name with n. if not already prefixed
    // Split by commas to handle multiple assignments
    std::vector<std::string> assignments;
    size_t start = 0;
    bool inQuote = false;
    for (size_t i = 0; i < cypherSetClause.size(); ++i) {
        if (cypherSetClause[i] == '\'' && (i == 0 || cypherSetClause[i-1] != '\\')) {
            inQuote = !inQuote;
        } else if (cypherSetClause[i] == ',' && !inQuote) {
            assignments.push_back(cypherSetClause.substr(start, i - start));
            start = i + 1;
        }
    }
    if (start < cypherSetClause.size()) {
        assignments.push_back(cypherSetClause.substr(start));
    }
    
    // Process each assignment
    cypherSetClause.clear();
    for (size_t i = 0; i < assignments.size(); ++i) {
        std::string assignment = assignments[i];
        assignment.erase(0, assignment.find_first_not_of(" \t"));
        
        // Find the = sign
        size_t eqPos = assignment.find('=');
        if (eqPos != std::string::npos) {
            std::string lhs = assignment.substr(0, eqPos);
            std::string rhs = assignment.substr(eqPos);
            
            lhs.erase(0, lhs.find_first_not_of(" \t"));
            lhs.erase(lhs.find_last_not_of(" \t") + 1);
            
            // Prefix LHS with n. if not already done
            if (lhs.find("n.") != 0 && lhs.find('.') == std::string::npos) {
                lhs = "n." + lhs;
            }
            
            assignment = lhs + rhs;
        }
        
        if (i > 0) cypherSetClause += ", ";
        cypherSetClause += assignment;
    }
    
    cypher += " SET " + cypherSetClause;
    
    return cypher;
}

// Parse DELETE statement
std::string PostgresSession::parseDeleteQuery(const std::string& query) {
    // DELETE FROM table WHERE condition
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
    
    size_t deletePos = upperQuery.find("DELETE");
    size_t fromPos = upperQuery.find("FROM", deletePos);
    size_t wherePos = upperQuery.find("WHERE", fromPos);
    
    if (fromPos == std::string::npos) {
        throw std::runtime_error("Invalid DELETE statement: missing FROM");
    }
    
    // Extract table name
    size_t tableStart = fromPos + 4;
    while (tableStart < query.size() && std::isspace(query[tableStart])) tableStart++;
    
    size_t tableEnd = (wherePos != std::string::npos) ? wherePos : query.size();
    std::string tableName = query.substr(tableStart, tableEnd - tableStart);
    tableName.erase(0, tableName.find_first_not_of(" \t\n\r"));
    tableName.erase(tableName.find_last_not_of(" \t\n\r;") + 1);
    
    // Build Cypher MATCH...DELETE statement
    std::string cypher = "MATCH (n:" + tableName + ")";
    
    // Add WHERE clause if present
    if (wherePos != std::string::npos) {
        std::string whereClause = query.substr(wherePos + 5);
        whereClause.erase(0, whereClause.find_first_not_of(" \t\n\r"));
        whereClause.erase(whereClause.find_last_not_of(" \t\n\r;") + 1);
        
        // Replace table.column with n.column
        size_t pos = 0;
        while ((pos = whereClause.find(tableName + ".", pos)) != std::string::npos) {
            whereClause.replace(pos, tableName.length() + 1, "n.");
            pos += 2;
        }
        
        cypher += " WHERE " + whereClause;
    }
    
    cypher += " DELETE n";
    
    return cypher;
}

std::string PostgresSession::translateQuery(const std::string& postgresQuery) {
    // Trim and convert to uppercase for parsing
    std::string query = postgresQuery;
    query.erase(0, query.find_first_not_of(" \t\n\r"));
    query.erase(query.find_last_not_of(" \t\n\r;") + 1);
    
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
    
    // Handle different SQL statement types.
    // Parser helpers (parseSelectQuery, parseInsertQuery, etc.) throw
    // std::runtime_error on malformed input.  We propagate these as-is so that
    // the catch blocks in handleQuery/handleExecute/handleDescribe can convert
    // them into PostgreSQL ErrorResponse messages.  All callers of translateQuery
    // are already wrapped in try { … } catch (const std::exception& e) { … }.
    if (upperQuery.find("SELECT") == 0) {
        QueryInfo info = parseSelectQuery(query);
        return buildCypherFromSelect(info);
    } else if (upperQuery.find("INSERT INTO") == 0) {
        return parseInsertQuery(query);
    } else if (upperQuery.find("UPDATE") == 0) {
        return parseUpdateQuery(query);
    } else if (upperQuery.find("DELETE") == 0) {
        return parseDeleteQuery(query);
    } else if (upperQuery.find("BEGIN") == 0 || upperQuery.find("COMMIT") == 0 || 
               upperQuery.find("ROLLBACK") == 0) {
        // Transaction commands - accept but don't execute (no ACID guarantees yet)
        return "// Transaction: " + query;
    } else {
        throw std::runtime_error("Unsupported SQL statement type: " + query.substr(0, 32));
    }
}

#endif // THEMIS_ENABLE_POSTGRES_WIRE

