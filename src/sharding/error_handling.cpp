// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/error_handling.h"
#include "sharding/exceptions.h"

namespace themisdb {
namespace sharding {

std::string errorToString(DistributedSystemError error) {
    switch (error) {
        case DistributedSystemError::OK:
            return "OK";
        
        // Transaction Errors
        case DistributedSystemError::TRANSACTION_NOT_FOUND:
            return "Transaction not found";
        case DistributedSystemError::TRANSACTION_ALREADY_EXISTS:
            return "Transaction already exists";
        case DistributedSystemError::TRANSACTION_TIMEOUT:
            return "Transaction timeout";
        case DistributedSystemError::TRANSACTION_ABORTED:
            return "Transaction aborted";
        case DistributedSystemError::TRANSACTION_CONFLICT:
            return "Transaction conflict";
        case DistributedSystemError::DEADLOCK_DETECTED:
            return "Deadlock detected";
        
        // Participant Errors
        case DistributedSystemError::PARTICIPANT_UNREACHABLE:
            return "Participant unreachable";
        case DistributedSystemError::PARTICIPANT_REJECTED:
            return "Participant rejected";
        case DistributedSystemError::PARTICIPANT_TIMEOUT:
            return "Participant timeout";
        case DistributedSystemError::PREPARE_VOTE_REJECTED:
            return "Prepare vote rejected";
        case DistributedSystemError::COMMIT_FAILED:
            return "Commit failed";
        
        // Consensus Errors
        case DistributedSystemError::CONSENSUS_FAILED:
            return "Consensus failed";
        case DistributedSystemError::QUORUM_NOT_REACHED:
            return "Quorum not reached";
        case DistributedSystemError::LEADER_ELECTION_FAILED:
            return "Leader election failed";
        case DistributedSystemError::BALLOT_NUMBER_CONFLICT:
            return "Ballot number conflict";
        
        // Replication Errors
        case DistributedSystemError::REPLICATION_FAILED:
            return "Replication failed";
        case DistributedSystemError::REPLICA_UNAVAILABLE:
            return "Replica unavailable";
        case DistributedSystemError::WAL_WRITE_FAILED:
            return "WAL write failed";
        case DistributedSystemError::WRITE_CONCERN_NOT_MET:
            return "Write concern not met";
        
        // Network Errors
        case DistributedSystemError::CONNECTION_REFUSED:
            return "Connection refused";
        case DistributedSystemError::CONNECTION_TIMEOUT:
            return "Connection timeout";
        case DistributedSystemError::NETWORK_PARTITION:
            return "Network partition";
        case DistributedSystemError::NETWORK_UNSTABLE:
            return "Network unstable";
        
        // Health Monitoring Errors
        case DistributedSystemError::HEALTH_CHECK_FAILED:
            return "Health check failed";
        case DistributedSystemError::HEALTH_CHECK_TIMEOUT:
            return "Health check timeout";
        case DistributedSystemError::NO_HEALTHY_REPLICAS:
            return "No healthy replicas";
        
        // Authentication/Security Errors
        case DistributedSystemError::AUTHENTICATION_FAILED:
            return "Authentication failed";
        case DistributedSystemError::CERTIFICATE_INVALID:
            return "Certificate invalid";
        case DistributedSystemError::AUTHORIZATION_DENIED:
            return "Authorization denied";
        
        // Configuration Errors
        case DistributedSystemError::INVALID_CONFIGURATION:
            return "Invalid configuration";
        case DistributedSystemError::INVALID_ARGUMENT:
            return "Invalid argument";
        case DistributedSystemError::RESOURCE_EXHAUSTED:
            return "Resource exhausted";
        
        // Internal Errors
        case DistributedSystemError::INTERNAL_ERROR:
            return "Internal error";
        
        default:
            return "Unknown error";
    }
}

bool isRetriableError(DistributedSystemError error) {
    switch (error) {
        // Retriable errors
        case DistributedSystemError::TRANSACTION_TIMEOUT:
        case DistributedSystemError::PARTICIPANT_UNREACHABLE:
        case DistributedSystemError::PARTICIPANT_TIMEOUT:
        case DistributedSystemError::CONNECTION_TIMEOUT:
        case DistributedSystemError::NETWORK_UNSTABLE:
        case DistributedSystemError::HEALTH_CHECK_TIMEOUT:
        case DistributedSystemError::REPLICA_UNAVAILABLE:
        case DistributedSystemError::QUORUM_NOT_REACHED:
        case DistributedSystemError::RESOURCE_EXHAUSTED:
            return true;
        
        // Non-retriable errors
        case DistributedSystemError::TRANSACTION_NOT_FOUND:
        case DistributedSystemError::TRANSACTION_ALREADY_EXISTS:
        case DistributedSystemError::TRANSACTION_ABORTED:
        case DistributedSystemError::TRANSACTION_CONFLICT:
        case DistributedSystemError::DEADLOCK_DETECTED:
        case DistributedSystemError::PARTICIPANT_REJECTED:
        case DistributedSystemError::PREPARE_VOTE_REJECTED:
        case DistributedSystemError::COMMIT_FAILED:
        case DistributedSystemError::CONSENSUS_FAILED:
        case DistributedSystemError::CONNECTION_REFUSED:
        case DistributedSystemError::NETWORK_PARTITION:
        case DistributedSystemError::AUTHENTICATION_FAILED:
        case DistributedSystemError::CERTIFICATE_INVALID:
        case DistributedSystemError::AUTHORIZATION_DENIED:
        case DistributedSystemError::INVALID_CONFIGURATION:
        case DistributedSystemError::INVALID_ARGUMENT:
        case DistributedSystemError::INTERNAL_ERROR:
        default:
            return false;
    }
}

// Template specialization implementations
template<typename T>
T& Result<T>::valueOrThrow() {
    if (!success) {
        throw ThemisDBException(error, error_message);
    }
    return value;
}

template<typename T>
const T& Result<T>::valueOrThrow() const {
    if (!success) {
        throw ThemisDBException(error, error_message);
    }
    return value;
}

// Explicit instantiations for common types
template class Result<int>;
template class Result<bool>;
template class Result<std::string>;

} // namespace sharding
} // namespace themisdb
