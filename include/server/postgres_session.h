/**
 * @file postgres_session.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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
    /**
     * @brief Postgres Session.
     * @param[in] socket Input parameter.
     * @return Return value.
     */
    explicit PostgresSession(asio::ip::tcp::socket socket);
    /**
     * @brief Postgres Session.
     * @param[in] socket Input parameter.
     * @param[in,out] queryEngine Input/output parameter.
     * @return Return value.
     */
    explicit PostgresSession(asio::ip::tcp::socket socket, 
                           themis::QueryEngine* queryEngine);
    ~PostgresSession();

    /**
     * @brief Start.
     */
    void start();
    /**
     * @brief Stop.
     */
    void stop();

    // PostgreSQL protocol message handlers
    void handleStartupMessage(int32_t protocolVersion, const std::map<std::string, std::string>& params);
    /**
     * @brief Handle Query.
     * @param[in] query Input parameter.
     */
    void handleQuery(const std::string& query);
    /**
     * @brief Handle Parse.
     * @param[in] stmt Input parameter.
     * @param[in] query Input parameter.
     * @param[in] paramTypes Input parameter.
     */
    void handleParse(const std::string& stmt, const std::string& query, const std::vector<int32_t>& paramTypes);
    /**
     * @brief Handle Bind.
     * @param[in] portal Input parameter.
     * @param[in] stmt Input parameter.
     * @param[in] params Input parameter.
     */
    void handleBind(const std::string& portal, const std::string& stmt, const std::vector<std::string>& params);
    /**
     * @brief Handle Execute.
     * @param[in] portal Input parameter.
     * @param[in] maxRows Input parameter.
     */
    void handleExecute(const std::string& portal, int32_t maxRows);
    /**
     * @brief Handle Describe.
     * @param[in] type Input parameter.
     * @param[in] name Input parameter.
     */
    void handleDescribe(char type, const std::string& name); // 'S' for statement, 'P' for portal
    /**
     * @brief Handle Close.
     * @param[in] type Input parameter.
     * @param[in] name Input parameter.
     */
    void handleClose(char type, const std::string& name);
    /**
     * @brief Handle Sync.
     */
    void handleSync();
    /**
     * @brief Handle Terminate.
     */
    void handleTerminate();
    /**
     * @brief Handle Copy Data.
     * @param[in] data Input parameter.
     */
    void handleCopyData(const std::vector<uint8_t>& data);
    /**
     * @brief Handle Copy Done.
     */
    void handleCopyDone();
    /**
     * @brief Handle Copy Fail.
     * @param[in] message Input parameter.
     */
    void handleCopyFail(const std::string& message);

    /**
     * @brief Send Authentication Ok.
     */
    void sendAuthenticationOk();
    /**
     * @brief Send Authentication Cleartext Password.
     */
    void sendAuthenticationCleartextPassword();
    void sendAuthenticationMD5Password(const std::array<uint8_t, 4>& salt);
    /**
     * @brief Send Parameter Status.
     * @param[in] name Input parameter.
     * @param[in] value Input parameter.
     */
    void sendParameterStatus(const std::string& name, const std::string& value);
    /**
     * @brief Send Backend Key Data.
     * @param[in] processId Input parameter.
     * @param[in] secretKey Input parameter.
     */
    void sendBackendKeyData(int32_t processId, int32_t secretKey);
    /**
     * @brief Send Ready For Query.
     * @param[in] transactionStatus Input parameter.
     */
    void sendReadyForQuery(char transactionStatus); // 'I' idle, 'T' in transaction, 'E' error
    /**
     * @brief Send Row Description.
     * @param[in] fields Input parameter.
     */
    void sendRowDescription(const std::vector<FieldDescription>& fields);
    /**
     * @brief Send Data Row.
     * @param[in] values Input parameter.
     */
    void sendDataRow(const std::vector<std::string>& values);
    void sendDataRowBinary(const std::vector<std::pair<std::vector<uint8_t>, int32_t>>& values);
    /**
     * @brief Send Portal Suspended.
     */
    void sendPortalSuspended();
    /**
     * @brief Send Command Complete.
     * @param[in] commandTag Input parameter.
     */
    void sendCommandComplete(const std::string& commandTag);
    /**
     * @brief Send Parse Complete.
     */
    void sendParseComplete();
    /**
     * @brief Send Bind Complete.
     */
    void sendBindComplete();
    /**
     * @brief Send Parameter Description.
     * @param[in] paramTypes Input parameter.
     */
    void sendParameterDescription(const std::vector<int32_t>& paramTypes);
    /**
     * @brief Send No Data.
     */
    void sendNoData();
    /**
     * @brief Send Close Complete.
     */
    void sendCloseComplete();
    /**
     * @brief Send Copy In Response.
     * @param[in] formatCodes Input parameter.
     */
    void sendCopyInResponse(const std::vector<int16_t>& formatCodes);
    /**
     * @brief Send Copy Out Response.
     * @param[in] formatCodes Input parameter.
     */
    void sendCopyOutResponse(const std::vector<int16_t>& formatCodes);
    /**
     * @brief Send Copy Both Response.
     * @param[in] formatCodes Input parameter.
     */
    void sendCopyBothResponse(const std::vector<int16_t>& formatCodes);
    /**
     * @brief Send Copy Data.
     * @param[in] data Input parameter.
     */
    void sendCopyData(const std::vector<uint8_t>& data);
    /**
     * @brief Send Copy Done.
     */
    void sendCopyDone();
    /**
     * @brief Send Error Response.
     * @param[in] severity Input parameter.
     * @param[in] code Input parameter.
     * @param[in] message Input parameter.
     */
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
    /**
     * @brief Do Read.
     */
    void doRead();
    /**
     * @brief Do Write.
     */
    void doWrite();
    /**
     * @brief Write Message.
     * @param[in] type Input parameter.
     * @param[in] payload Input parameter.
     */
    void writeMessage(char type, const std::vector<uint8_t>& payload);
    /**
     * @brief Enqueue Write.
     * @param[in] message Input parameter.
     */
    void enqueueWrite(std::vector<uint8_t> message);
    /**
     * @brief Close Socket.
     */
    void closeSocket();
    /**
     * @brief Arm Read Timeout.
     */
    void armReadTimeout();
    /**
     * @brief Cancel Read Timeout.
     */
    void cancelReadTimeout();
    /**
     * @brief Arm Write Timeout.
     */
    void armWriteTimeout();
    /**
     * @brief Cancel Write Timeout.
     */
    void cancelWriteTimeout();
    /**
     * @brief Current Transaction Status.
     * @return Return value.
     */
    char currentTransactionStatus() const;
    
    /**
     * @brief Translate Query.
     * @param[in] postgresQuery Input parameter.
     * @return Return value.
     */
    std::string translateQuery(const std::string& postgresQuery);
    /**
     * @brief Is Schema Query.
     * @param[in] query Input parameter.
     * @return True when the operation succeeds.
     */
    bool isSchemaQuery(const std::string& query);
    /**
     * @brief Handle Schema Query.
     * @param[in] query Input parameter.
     */
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
    
    /**
     * @brief Parse Select Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    QueryInfo parseSelectQuery(const std::string& query);
    /**
     * @brief Build Cypher From Select.
     * @param[in] info Input parameter.
     * @return Return value.
     */
    std::string buildCypherFromSelect(const QueryInfo& info);
    /**
     * @brief Parse Insert Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string parseInsertQuery(const std::string& query);
    /**
     * @brief Parse Update Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string parseUpdateQuery(const std::string& query);
    /**
     * @brief Parse Delete Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
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
