/**
 * @file distributed_txn_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/distributed_transaction.h"
#include "sharding/truetime.h"
#include <boost/beast/http.hpp>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;

namespace themis::server {

class DistributedTxnApiHandler {
public:
    /**
     * @brief Distributed Txn Api Handler.
     * @param[in] coordinator Input parameter.
     * @return Return value.
     */
    explicit DistributedTxnApiHandler(
        std::shared_ptr<sharding::DistributedTransactionCoordinator> coordinator
    );


    /**
     * @brief Handle Begin.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleBegin(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle Operation.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleOperation(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle Commit.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCommit(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle Abort.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleAbort(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle Read Only.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleReadOnly(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStats(
        const http::request<http::string_body>& req
    );

private:
    std::shared_ptr<sharding::DistributedTransactionCoordinator> coordinator_;

    /**
     * @brief Ok.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> ok(
        const nlohmann::json& body,
        const http::request<http::string_body>& req
    ) const;

    /**
     * @brief Error.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> error(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    ) const;

    /**
     * @brief State To String.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    static std::string stateToString(sharding::TransactionState state);
};

} // namespace themis::server

