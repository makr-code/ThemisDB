// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/exceptions.h"

namespace themisdb {
namespace sharding {

ThemisDBException::ThemisDBException(
    DistributedSystemError error,
    const std::string& message,
    const std::string& component
) : std::runtime_error(message),
    error_(error),
    component_(component) {}

TransactionException::TransactionException(
    DistributedSystemError error,
    const std::string& message,
    const std::string& transaction_id
) : ThemisDBException(error, message, "Transaction"),
    transaction_id_(transaction_id) {}

} // namespace sharding
} // namespace themisdb
