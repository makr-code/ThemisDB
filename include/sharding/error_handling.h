// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_ERROR_HANDLING_H
#define THEMISDB_SHARDING_ERROR_HANDLING_H

#include <string>
#include <optional>
#include <chrono>
#include <vector>
#include <map>
#include <type_traits>

namespace themisdb {
namespace sharding {

/**
 * @brief Comprehensive error codes for distributed system operations
 */
enum class DistributedSystemError {
    // Success
    OK = 0,
    
    // Transaction Errors (100-199)
    TRANSACTION_NOT_FOUND = 100,
    TRANSACTION_ALREADY_EXISTS = 101,
    TRANSACTION_TIMEOUT = 102,
    TRANSACTION_ABORTED = 103,
    TRANSACTION_CONFLICT = 104,
    DEADLOCK_DETECTED = 105,
    
    // Participant Errors (200-299)
    PARTICIPANT_UNREACHABLE = 200,
    PARTICIPANT_REJECTED = 201,
    PARTICIPANT_TIMEOUT = 202,
    PREPARE_VOTE_REJECTED = 203,
    COMMIT_FAILED = 204,
    
    // Consensus Errors (300-399)
    CONSENSUS_FAILED = 300,
    QUORUM_NOT_REACHED = 301,
    LEADER_ELECTION_FAILED = 302,
    BALLOT_NUMBER_CONFLICT = 303,
    
    // Replication Errors (400-499)
    REPLICATION_FAILED = 400,
    REPLICA_UNAVAILABLE = 401,
    WAL_WRITE_FAILED = 402,
    WRITE_CONCERN_NOT_MET = 403,
    
    // Network Errors (500-599)
    CONNECTION_REFUSED = 500,
    CONNECTION_TIMEOUT = 501,
    NETWORK_PARTITION = 502,
    NETWORK_UNSTABLE = 503,
    
    // Health Monitoring Errors (600-699)
    HEALTH_CHECK_FAILED = 600,
    HEALTH_CHECK_TIMEOUT = 601,
    NO_HEALTHY_REPLICAS = 602,
    
    // Authentication/Security Errors (700-799)
    AUTHENTICATION_FAILED = 700,
    CERTIFICATE_INVALID = 701,
    AUTHORIZATION_DENIED = 702,
    
    // Configuration Errors (800-899)
    INVALID_CONFIGURATION = 800,
    INVALID_ARGUMENT = 801,
    RESOURCE_EXHAUSTED = 802,
    
    // Internal Errors (900-999)
    INTERNAL_ERROR = 999
};

/**
 * @brief Convert error code to human-readable string
 */
std::string errorToString(DistributedSystemError error);

/**
 * @brief Check if error is retriable
 */
bool isRetriableError(DistributedSystemError error);

/**
 * @brief Error context for debugging and tracing
 */
struct ErrorContext {
    std::string transaction_id;
    std::string operation_name;
    std::chrono::system_clock::time_point timestamp;
    std::string source_component;
    std::vector<std::string> call_stack;
    std::map<std::string, std::string> additional_context;
    
    ErrorContext() : timestamp(std::chrono::system_clock::now()) {}
    
    explicit ErrorContext(const std::string& operation)
        : operation_name(operation), timestamp(std::chrono::system_clock::now()) {}
    
    ErrorContext(const std::string& operation, const std::string& component)
        : operation_name(operation), 
          source_component(component),
          timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Result type for operations that can fail
 * 
 * Provides type-safe error handling without exceptions.
 * Usage:
 *   Result<int> result = performOperation();
 *   if (result) {
 *     int value = *result;
 *   } else {
 *     handleError(result.error, result.error_message);
 *   }
 */
template<typename T>
struct Result {
    bool success;
    T value;
    DistributedSystemError error;
    std::string error_message;
    std::optional<ErrorContext> context;
    
    // Success constructor
    explicit Result(T val)
        : success(true), value(std::move(val)), error(DistributedSystemError::OK) {}
    
    // Error constructor
    Result(DistributedSystemError err, const std::string& msg)
        : success(false), value{}, error(err), error_message(msg) {}
    
    Result(DistributedSystemError err, const std::string& msg, const ErrorContext& ctx)
        : success(false), value{}, error(err), error_message(msg), context(ctx) {}
    
    // Convenience methods
    explicit operator bool() const { return success; }
    T& operator*() { return value; }
    const T& operator*() const { return value; }
    T* operator->() { return &value; }
    const T* operator->() const { return &value; }
    
    // Get value or throw
    T& valueOrThrow();
    const T& valueOrThrow() const;
};

/**
 * @brief Specialization for void operations
 */
template<>
struct Result<void> {
    bool success;
    DistributedSystemError error;
    std::string error_message;
    std::optional<ErrorContext> context;
    
    // Success constructor
    Result() : success(true), error(DistributedSystemError::OK) {}
    
    // Error constructor
    Result(DistributedSystemError err, const std::string& msg)
        : success(false), error(err), error_message(msg) {}
    
    Result(DistributedSystemError err, const std::string& msg, const ErrorContext& ctx)
        : success(false), error(err), error_message(msg), context(ctx) {}
    
    explicit operator bool() const { return success; }
};

// Helper functions to create Result types
template<typename T>
Result<T> Ok(T value) {
    return Result<T>(std::move(value));
}

inline Result<void> Ok() {
    return Result<void>();
}

template<typename T>
Result<T> Err(DistributedSystemError error, const std::string& message) {
    return Result<T>(error, message);
}

inline Result<void> Err(DistributedSystemError error, const std::string& message) {
    return Result<void>(error, message);
}

template<typename T>
Result<T> Err(DistributedSystemError error, const std::string& message, const ErrorContext& ctx) {
    return Result<T>(error, message, ctx);
}

inline Result<void> Err(DistributedSystemError error, const std::string& message, const ErrorContext& ctx) {
    return Result<void>(error, message, ctx);
}

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_ERROR_HANDLING_H
