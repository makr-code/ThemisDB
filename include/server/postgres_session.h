#pragma once

#ifdef THEMIS_ENABLE_POSTGRES_WIRE

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <array>

namespace asio = boost::asio;

class PostgresSession : public std::enable_shared_from_this<PostgresSession> {
public:
    explicit PostgresSession(asio::ip::tcp::socket socket);
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

    // Send PostgreSQL protocol messages
    void sendAuthenticationOk();
    void sendAuthenticationCleartextPassword();
    void sendAuthenticationMD5Password(const std::array<uint8_t, 4>& salt);
    void sendParameterStatus(const std::string& name, const std::string& value);
    void sendBackendKeyData(int32_t processId, int32_t secretKey);
    void sendReadyForQuery(char transactionStatus); // 'I' idle, 'T' in transaction, 'E' error
    void sendRowDescription(const std::vector<FieldDescription>& fields);
    void sendDataRow(const std::vector<std::string>& values);
    void sendCommandComplete(const std::string& commandTag);
    void sendParseComplete();
    void sendBindComplete();
    void sendParameterDescription(const std::vector<int32_t>& paramTypes);
    void sendNoData();
    void sendCloseComplete();
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
    std::array<char, 8192> buffer_;
    std::string databaseName_;
    std::string userName_;
    bool isAuthenticated_;
    bool inStartup_;
    std::deque<std::vector<uint8_t>> writeQueue_;
    
    // Transaction state tracking
    enum class TransactionState {
        IDLE,           // 'I' - not in a transaction
        IN_TRANSACTION, // 'T' - in a transaction block
        FAILED          // 'E' - in a failed transaction block
    };
    TransactionState transactionState_ = TransactionState::IDLE;
    
    // Prepared statements and portals
    struct PreparedStatement {
        std::string query;
        std::vector<int32_t> paramTypes;
    };
    
    struct Portal {
        std::string statementName;
        std::vector<std::string> params;
    };
    
    std::map<std::string, PreparedStatement> preparedStatements_;
    std::map<std::string, Portal> portals_;
};

#endif // THEMIS_ENABLE_POSTGRES_WIRE
