#ifdef THEMIS_ENABLE_POSTGRES_WIRE

#include "server/postgres_session.h"
#include <boost/beast/core.hpp>
#include <algorithm>
#include <iostream>

PostgresSession::PostgresSession(asio::ip::tcp::socket socket)
    : socket_(std::move(socket))
    , isAuthenticated_(false)
    , inStartup_(true) {
}

PostgresSession::~PostgresSession() {
    stop();
}

void PostgresSession::start() {
    doRead();
}

void PostgresSession::stop() {
    boost::beast::error_code ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}

void PostgresSession::handleStartupMessage(int32_t protocolVersion, 
                                          const std::map<std::string, std::string>& params) {
    // PostgreSQL startup message handler (stub)
    // TODO: Validate protocol version (3.0 = 196608)
    // TODO: Extract database, user, options, etc.
    
    auto dbIt = params.find("database");
    auto userIt = params.find("user");
    
    if (dbIt != params.end()) databaseName_ = dbIt->second;
    if (userIt != params.end()) userName_ = userIt->second;
    
    inStartup_ = false;
    
    // TODO: Implement proper authentication
    // For now, skip authentication
    sendAuthenticationOk();
    
    // Send server parameters
    sendParameterStatus("server_version", "14.0 (ThemisDB compatibility)");
    sendParameterStatus("server_encoding", "UTF8");
    sendParameterStatus("client_encoding", "UTF8");
    sendParameterStatus("DateStyle", "ISO, MDY");
    sendParameterStatus("TimeZone", "UTC");
    
    sendBackendKeyData(12345, 67890);
    sendReadyForQuery('I');
    
    isAuthenticated_ = true;
}

void PostgresSession::handleQuery(const std::string& query) {
    // Handle schema queries (pg_catalog, information_schema) for BI tool compatibility
    if (isSchemaQuery(query)) {
        handleSchemaQuery(query);
        sendReadyForQuery('I');
        return;
    }
    
    // Handle special PostgreSQL functions
    if (query.find("SELECT version()") != std::string::npos ||
        query.find("select version()") != std::string::npos) {
        std::vector<FieldDescription> fields = {
            {"version", 0, 0, 25, -1, -1, 0} // text type
        };
        sendRowDescription(fields);
        sendDataRow({"PostgreSQL 14.0 (ThemisDB 1.2.0 compatibility mode)"});
        sendCommandComplete("SELECT 1");
        sendReadyForQuery('I');
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
        sendReadyForQuery('I');
        return;
    }
    
    // Translate SQL to Cypher for regular queries
    try {
        std::string cypherQuery = translateQuery(query);
        
        // TODO: Execute Cypher query against ThemisDB
        // For now, send mock response to indicate successful translation
        sendCommandComplete("SELECT 0");
    } catch (const std::exception& e) {
        sendErrorResponse("ERROR", "42601", std::string("Query translation failed: ") + e.what());
    }
    
    sendReadyForQuery('I');
}

void PostgresSession::handleParse(const std::string& stmt, const std::string& query, 
                                 const std::vector<int32_t>& paramTypes) {
    // PostgreSQL Parse message handler (stub)
    // TODO: Parse and validate query
    // TODO: Store prepared statement
    
    preparedStatements_[stmt] = {query, paramTypes};
    sendParseComplete();
}

void PostgresSession::handleBind(const std::string& portal, const std::string& stmt, 
                                const std::vector<std::string>& params) {
    // PostgreSQL Bind message handler (stub)
    // TODO: Bind parameters to prepared statement
    // TODO: Create portal
    
    portals_[portal] = {stmt, params};
    sendBindComplete();
}

void PostgresSession::handleExecute(const std::string& portal, int32_t maxRows) {
    // PostgreSQL Execute message handler (stub)
    // TODO: Execute portal
    // TODO: Return results (up to maxRows)
    
    auto it = portals_.find(portal);
    if (it != portals_.end()) {
        const auto& portalData = it->second;
        auto stmtIt = preparedStatements_.find(portalData.statementName);
        if (stmtIt != preparedStatements_.end()) {
            // TODO: Execute with bound parameters
            std::string query = stmtIt->second.query;
            // Execute...
            sendCommandComplete("SELECT 0");
        }
    }
}

void PostgresSession::handleDescribe(char type, const std::string& name) {
    // PostgreSQL Describe message handler (stub)
    // TODO: Return description of statement or portal
    if (type == 'S') {
        // Describe statement
        // TODO: Send ParameterDescription and RowDescription
    } else if (type == 'P') {
        // Describe portal
        // TODO: Send RowDescription
    }
}

void PostgresSession::handleClose(char type, const std::string& name) {
    // PostgreSQL Close message handler (stub)
    if (type == 'S') {
        preparedStatements_.erase(name);
    } else if (type == 'P') {
        portals_.erase(name);
    }
}

void PostgresSession::handleSync() {
    // PostgreSQL Sync message handler
    sendReadyForQuery('I');
}

void PostgresSession::handleTerminate() {
    // PostgreSQL Terminate message handler
    stop();
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
    
    // Field count
    uint16_t fieldCount = fields.size();
    payload.push_back((fieldCount >> 8) & 0xFF);
    payload.push_back(fieldCount & 0xFF);
    
    for (const auto& field : fields) {
        // Field name
        payload.insert(payload.end(), field.name.begin(), field.name.end());
        payload.push_back(0);
        
        // Table OID, column number, type OID, type size, type modifier, format code
        // TODO: Proper encoding of all fields
        for (int i = 0; i < 18; ++i) payload.push_back(0);
    }
    
    writeMessage('T', payload);
}

void PostgresSession::sendDataRow(const std::vector<std::string>& values) {
    std::vector<uint8_t> payload;
    
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

void PostgresSession::sendCommandComplete(const std::string& commandTag) {
    std::vector<uint8_t> payload;
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

void PostgresSession::sendErrorResponse(const std::string& severity, const std::string& code, 
                                       const std::string& message) {
    std::vector<uint8_t> payload;
    
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
    
    socket_.async_read_some(asio::buffer(buffer_),
        [this, self](boost::beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                stop();
                return;
            }
            
            if (bytes_transferred < 5) {
                doRead(); // Need more data
                return;
            }
            
            size_t offset = 0;
            
            if (inStartup_) {
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
                        // Parameter types would follow
                        handleParse(stmtName, query, {});
                        break;
                    }
                    case 'B': { // Bind
                        std::string portalName(buffer_.data() + offset);
                        offset += portalName.size() + 1;
                        std::string stmtName(buffer_.data() + offset);
                        // Parameters would follow
                        handleBind(portalName, stmtName, {});
                        break;
                    }
                    case 'E': { // Execute
                        std::string portalName(buffer_.data() + offset);
                        offset += portalName.size() + 1;
                        int32_t maxRows = (buffer_[offset] << 24) | (buffer_[offset+1] << 16) |
                                        (buffer_[offset+2] << 8) | buffer_[offset+3];
                        handleExecute(portalName, maxRows);
                        break;
                    }
                    case 'D': { // Describe
                        char descType = buffer_[offset];
                        std::string name(buffer_.data() + offset + 1);
                        handleDescribe(descType, name);
                        break;
                    }
                    case 'C': { // Close
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
                    default:
                        break;
                }
            }
            
            doRead(); // Continue reading
        });
}

void PostgresSession::doWrite() {
    if (writeQueue_.empty()) {
        return;
    }
    
    auto self = shared_from_this();
    
    asio::async_write(socket_, asio::buffer(writeQueue_.front()),
        [this, self](boost::beast::error_code ec, std::size_t /*bytes_transferred*/) {
            if (!ec) {
                writeQueue_.pop_front();
                if (!writeQueue_.empty()) {
                    doWrite();
                }
            } else {
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
    
    writeQueue_.push_back(std::move(message));
    doWrite();
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
    if (lowerQuery.find("pg_catalog.pg_type") != std::string::npos) {
        // Return mock type information for common types
        std::vector<FieldDescription> fields = {
            {"oid", 0, 0, 26, 4, -1, 0},
            {"typname", 0, 0, 19, 64, -1, 0},
            {"typlen", 0, 0, 21, 2, -1, 0}
        };
        sendRowDescription(fields);
        // Common PostgreSQL types
        sendDataRow({"16", "bool", "1"});
        sendDataRow({"20", "int8", "8"});
        sendDataRow({"21", "int2", "2"});
        sendDataRow({"23", "int4", "4"});
        sendDataRow({"25", "text", "-1"});
        sendDataRow({"1043", "varchar", "-1"});
        sendCommandComplete("SELECT 6");
    } else if (lowerQuery.find("pg_catalog.pg_namespace") != std::string::npos) {
        // Return schema/namespace information
        std::vector<FieldDescription> fields = {
            {"oid", 0, 0, 26, 4, -1, 0},
            {"nspname", 0, 0, 19, 64, -1, 0}
        };
        sendRowDescription(fields);
        sendDataRow({"2200", "public"});
        sendDataRow({"11", "pg_catalog"});
        sendDataRow({"99", "pg_toast"});
        sendCommandComplete("SELECT 3");
    } else if (lowerQuery.find("pg_catalog.pg_class") != std::string::npos) {
        // Return table information (mock: users, orders as examples)
        std::vector<FieldDescription> fields = {
            {"oid", 0, 0, 26, 4, -1, 0},
            {"relname", 0, 0, 19, 64, -1, 0},
            {"relkind", 0, 0, 18, 1, -1, 0},
            {"relnamespace", 0, 0, 26, 4, -1, 0}
        };
        sendRowDescription(fields);
        sendDataRow({"16384", "users", "r", "2200"});
        sendDataRow({"16385", "orders", "r", "2200"});
        sendDataRow({"16386", "products", "r", "2200"});
        sendCommandComplete("SELECT 3");
    } else if (lowerQuery.find("pg_catalog.pg_attribute") != std::string::npos) {
        // Return column information
        std::vector<FieldDescription> fields = {
            {"attrelid", 0, 0, 26, 4, -1, 0},
            {"attname", 0, 0, 19, 64, -1, 0},
            {"atttypid", 0, 0, 26, 4, -1, 0},
            {"attnum", 0, 0, 21, 2, -1, 0}
        };
        sendRowDescription(fields);
        // Example columns for 'users' table
        sendDataRow({"16384", "id", "23", "1"});
        sendDataRow({"16384", "name", "25", "2"});
        sendDataRow({"16384", "email", "25", "3"});
        sendCommandComplete("SELECT 3");
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
        // TODO: Parse JOIN properly (INNER JOIN table ON condition)
        info.joinTable = ""; // Placeholder
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
    
    for (char c : valsList) {
        if (c == '\'' && (currentValue.empty() || currentValue.back() != '\\')) {
            inQuote = !inQuote;
            currentValue += c;
        } else if (c == ',' && !inQuote) {
            currentValue.erase(0, currentValue.find_first_not_of(" \t"));
            currentValue.erase(currentValue.find_last_not_of(" \t") + 1);
            values.push_back(currentValue);
            currentValue.clear();
        } else {
            currentValue += c;
        }
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
    
    // Extract table name
    std::string tableName = query.substr(updatePos + 6, setPos - updatePos - 6);
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
    
    // Add SET clause (convert column references to n.column)
    std::string cypherSetClause = setClause;
    size_t pos = 0;
    while ((pos = cypherSetClause.find(tableName + ".", pos)) != std::string::npos) {
        cypherSetClause.replace(pos, tableName.length() + 1, "n.");
        pos += 2;
    }
    // Also handle bare column names (col = value)
    pos = 0;
    while (pos < cypherSetClause.size()) {
        if (!std::isspace(cypherSetClause[pos]) && cypherSetClause[pos] != ',' && 
            cypherSetClause[pos] != '=' && cypherSetClause[pos] != '\'' && cypherSetClause[pos] != '"') {
            size_t nameEnd = pos;
            while (nameEnd < cypherSetClause.size() && 
                   (std::isalnum(cypherSetClause[nameEnd]) || cypherSetClause[nameEnd] == '_')) {
                nameEnd++;
            }
            if (nameEnd < cypherSetClause.size() && 
                (cypherSetClause[nameEnd] == '=' || std::isspace(cypherSetClause[nameEnd]))) {
                std::string colName = cypherSetClause.substr(pos, nameEnd - pos);
                if (colName.find("n.") != 0) {
                    cypherSetClause.insert(pos, "n.");
                    pos += 2;
                }
            }
            pos = nameEnd;
        }
        pos++;
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
    
    // Handle different SQL statement types
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
        throw std::runtime_error("Unsupported SQL statement type");
    }
}

#endif // THEMIS_ENABLE_POSTGRES_WIRE
