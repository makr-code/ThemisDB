// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_EXCEPTIONS_H
#define THEMISDB_SHARDING_EXCEPTIONS_H

#include "sharding/error_handling.h"
#include <stdexcept>
#include <string>

namespace themisdb {
namespace sharding {

/**
 * @brief Base exception class for ThemisDB distributed system errors
 */
class ThemisDBException : public std::runtime_error {
public:
    explicit ThemisDBException(
        DistributedSystemError error,
        const std::string& message,
        const std::string& component = ""
    );
    
    DistributedSystemError error() const { return error_; }
    const std::string& component() const { return component_; }
    
private:
    DistributedSystemError error_;
    std::string component_;
};

/**
 * @brief Transaction-related exceptions
 */
class TransactionException : public ThemisDBException {
public:
    explicit TransactionException(
        DistributedSystemError error,
        const std::string& message,
        const std::string& transaction_id = ""
    );
    
    const std::string& transactionId() const { return transaction_id_; }
    
private:
    std::string transaction_id_;
};

/**
 * @brief Consensus-related exceptions
 */
class ConsensusException : public ThemisDBException {
public:
    explicit ConsensusException(
        DistributedSystemError error,
        const std::string& message,
        const std::string& component = "Consensus"
    ) : ThemisDBException(error, message, component) {}
};

/**
 * @brief Replication-related exceptions
 */
class ReplicationException : public ThemisDBException {
public:
    explicit ReplicationException(
        DistributedSystemError error,
        const std::string& message,
        const std::string& component = "Replication"
    ) : ThemisDBException(error, message, component) {}
};

/**
 * @brief Network-related exceptions
 */
class NetworkException : public ThemisDBException {
public:
    explicit NetworkException(
        DistributedSystemError error,
        const std::string& message,
        const std::string& component = "Network"
    ) : ThemisDBException(error, message, component) {}
};

/**
 * @brief Timeout exception
 */
class TimeoutException : public ThemisDBException {
public:
    explicit TimeoutException(
        const std::string& message,
        const std::string& component = ""
    ) : ThemisDBException(DistributedSystemError::TRANSACTION_TIMEOUT, message, component) {}
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_EXCEPTIONS_H
