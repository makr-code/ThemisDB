#include <iostream>
#include <cassert>

// Minimal includes for standalone test
#include "sharding/error_handling.h"
#include "sharding/exceptions.h"
#include "sharding/retry_strategy.h"

using namespace themisdb::sharding;

int main() {
    std::cout << "Testing error handling infrastructure..." << std::endl;
    
    // Test 1: Error code to string
    assert(errorToString(DistributedSystemError::OK) == "OK");
    assert(errorToString(DistributedSystemError::TRANSACTION_NOT_FOUND) == "Transaction not found");
    std::cout << "✓ Error to string conversion works" << std::endl;
    
    // Test 2: Retriable error detection
    assert(isRetriableError(DistributedSystemError::TRANSACTION_TIMEOUT));
    assert(!isRetriableError(DistributedSystemError::INVALID_ARGUMENT));
    std::cout << "✓ Retriable error detection works" << std::endl;
    
    // Test 3: Result with success
    Result<int> success_result = Ok(42);
    assert(success_result.success);
    assert(*success_result == 42);
    std::cout << "✓ Result type with success works" << std::endl;
    
    // Test 4: Result with error
    Result<int> error_result = Err<int>(
        DistributedSystemError::TRANSACTION_NOT_FOUND,
        "Transaction not found"
    );
    assert(!error_result.success);
    assert(error_result.error == DistributedSystemError::TRANSACTION_NOT_FOUND);
    std::cout << "✓ Result type with error works" << std::endl;
    
    // Test 5: Result<void>
    Result<void> void_success = Ok();
    assert(void_success.success);
    std::cout << "✓ Result<void> works" << std::endl;
    
    // Test 6: Exception creation
    try {
        throw ThemisDBException(
            DistributedSystemError::INTERNAL_ERROR,
            "Test error",
            "TestComponent"
        );
    } catch (const ThemisDBException& e) {
        assert(e.error() == DistributedSystemError::INTERNAL_ERROR);
        assert(e.component() == "TestComponent");
        std::cout << "✓ ThemisDBException works" << std::endl;
    }
    
    // Test 7: Retry delay calculation
    RetryConfig config;
    config.strategy = RetryStrategy::EXPONENTIAL_BACKOFF;
    config.initial_delay = std::chrono::milliseconds(100);
    config.backoff_multiplier = 2.0;
    config.jitter = false;
    
    auto delay1 = calculateRetryDelay(config, 1);
    auto delay2 = calculateRetryDelay(config, 2);
    assert(delay1.count() == 100);
    assert(delay2.count() == 200);
    std::cout << "✓ Retry delay calculation works" << std::endl;
    
    // Test 8: Execute with retry
    int attempt_count = 0;
    auto result = executeWithRetry([&]() -> Result<int> {
        attempt_count++;
        if (attempt_count < 3) {
            return Err<int>(DistributedSystemError::PARTICIPANT_TIMEOUT, "Timeout");
        }
        return Ok(100);
    }, config);
    
    assert(result.success);
    assert(*result == 100);
    assert(attempt_count == 3);
    std::cout << "✓ Execute with retry works" << std::endl;
    
    std::cout << "\n✅ All tests passed!" << std::endl;
    return 0;
}
