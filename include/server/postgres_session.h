/**
 * @file postgres_session.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef THEMIS_ENABLE_POSTGRES_WIRE

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <array>
#include <atomic>
#include <mutex>
#include <chrono>

namespace asio = boost::asio;

// Forward declarations for ThemisDB components
namespace themis {
    class QueryEngine;
    class RocksDBWrapper;
    class SecondaryIndexManager;
}

class PostgresSession : public std::enable_shared_from_this<PostgresSession> {
public:
    explicit PostgresSession(asio::ip::tcp::socket socket);
    explicit PostgresSession(asio::ip::tcp::socket socket, 
                           themis::QueryEngine* queryEngine);
    ~PostgresSession();

    void start();
    void stop();

    // PostgreSQL protocol message handlers
    void handleStartupMessage(int32_t protocolVersion, const std::map<std::string, std::string>& params);
    void handleQuery(const std::string& query);
    void handleParse(const std::string& stmt, const std::string& query, const std::vector<int32_t>& paramTypes);
    void handleBind(const std::string& portal, const std::string& stmt, const std::vector<std::string>& params);
    void handleExecute(const std::string& portal, int32_t maxRows);
    void handleDescribe(char type, const std::string& name); // 'S' for statement, 'P' for portal
    void handleClose(char type, const std::string& name);
    void handleSync();
    void handleTerminate();
    void handleCopyData(const std::vector<uint8_t>& data);
    void handleCopyDone();
    void handleCopyFail(const std::string& message);

    // Send PostgreSQL protocol messages
    void sendAuthenticationOk();
    void sendAuthenticationCleartextPassword();
    void sendAuthenticationMD5Password(const std::array<uint8_t, 4>& salt);
    void sendParameterStatus(const std::string& name, const std::string& value);
    void sendBackendKeyData(int32_t processId, int32_t secretKey);
    void sendReadyForQuery(char transactionStatus); // 'I' idle, 'T' in transaction, 'E' error
    void sendRowDescription(const std::vector<FieldDescription>& fields);
    void sendDataRow(const std::vector<std::string>& values);
    void sendDataRowBinary(const std::vector<std::pair<std::vector<uint8_t>, int32_t>>& values);
    void sendPortalSuspended();
    void sendCommandComplete(const std::string& commandTag);
    void sendParseComplete();
    void sendBindComplete();
    void sendParameterDescription(const std::vector<int32_t>& paramTypes);
    void sendNoData();
    void sendCloseComplete();
    void sendCopyInResponse(const std::vector<int16_t>& formatCodes);
    void sendCopyOutResponse(const std::vector<int16_t>& formatCodes);
    void sendCopyBothResponse(const std::vector<int16_t>& formatCodes);
    void sendCopyData(const std::vector<uint8_t>& data);
    void sendCopyDone();
    void sendErrorResponse(const std::string& severity, const std::string& code, const std::string& message);

    struct FieldDescription {
        std::string name;
        int32_t tableOid;
        int16_t columnAttrNumber;
        int32_t dataTypeOid;
        int16_t dataTypeSize;
        int32_t typeModifier;
        int16_t formatCode; // 0=text, 1=binary
    };

private:
    void doRead();
    void doWrite();
    void writeMessage(char type, const std::vector<uint8_t>& payload);
    void enqueueWrite(std::vector<uint8_t> message);
    void closeSocket();
    void armReadTimeout();
    void cancelReadTimeout();
    void armWriteTimeout();
    void cancelWriteTimeout();
    char currentTransactionStatus() const;
    
    // SQL to Cypher translation for BI tools
    std::string translateQuery(const std::string& postgresQuery);
    bool isSchemaQuery(const std::string& query);
    void handleSchemaQuery(const std::string& query);
    
    struct QueryInfo {
        std::string type; // SELECT, INSERT, UPDATE, DELETE
        std::vector<std::string> selectColumns;
        std::string tableName;
        std::string whereClause;
        std::string orderBy;
        std::string groupBy;
        std::vector<std::string> aggregates;
        int limit = -1;
        int offset = -1;
        std::string joinTable;
        std::string joinCondition;
    };
    
    QueryInfo parseSelectQuery(const std::string& query);
    std::string buildCypherFromSelect(const QueryInfo& info);
    std::string parseInsertQuery(const std::string& query);
    std::string parseUpdateQuery(const std::string& query);
    std::string parseDeleteQuery(const std::string& query);
    
    asio::ip::tcp::socket socket_;
    asio::steady_timer readTimeoutTimer_;
    asio::steady_timer writeTimeoutTimer_;
    std::array<char, 8192> buffer_;
    std::string databaseName_;
    std::string userName_;
    std::atomic<bool> isAuthenticated_{false};
    std::atomic<bool> inStartup_{true};
    std::atomic<bool> stopped_{false};
    std::deque<std::vector<uint8_t>> writeQueue_;
    mutable std::mutex writeMutex_;
    bool writeInProgress_ = false;
    static constexpr std::chrono::seconds kReadTimeout{30};
    static constexpr std::chrono::seconds kWriteTimeout{30};
    
    // Transaction state tracking
    enum class TransactionState {
        IDLE,           // 'I' - not in a transaction
        IN_TRANSACTION, // 'T' - in a transaction block
        FAILED          // 'E' - in a failed transaction block
    };
    std::atomic<TransactionState> transactionState_{TransactionState::IDLE};
    
    // COPY protocol state
    std::atomic<bool> copyInProgress_{false};
    std::vector<std::string> copyBuffer_;
    std::string copyTableName_;   // table name extracted from COPY … FROM STDIN
    mutable std::mutex copyMutex_;
    
    // Prepared statements and portals
    struct PreparedStatement {
        std::string query = {};
        std::vector<int32_t> paramTypes;
    };
    
    struct Portal {
        std::string statementName;
        std::vector<std::string> params;
        size_t currentRow = 0;  // For result streaming
        std::vector<std::vector<std::string>> cachedResults;  // Cached query results
        bool resultsComplete = false;  // Whether all results have been fetched
    };
    
    std::map<std::string, PreparedStatement> preparedStatements_;
    std::map<std::string, Portal> portals_;
    mutable std::mutex preparedStatementsMutex_;
    mutable std::mutex portalsMutex_;
    
    // Optional: Query engine for database integration
    themis::QueryEngine* queryEngine_ = nullptr;
};

#endif // THEMIS_ENABLE_POSTGRES_WIRE
