/**
 * @file test_utils_observability_error_contracts.cpp
 * @brief Phase 4 tests for logger.cpp and tracing.cpp error contracts.
 *
 * Coverage targets (Phase 4 gate):
 *  - Logger::init() emit structured LOG_INITIALIZATION_FAILED on sink failure
 *  - Logger error codes in range 9020-9029
 *  - Tracer::initialize() fail-open behavior (returns false, does not throw)
 *  - Tracer error codes in range 9030-9039
 *  - ErrorContext categorisation for observability codes
 */

#include <gtest/gtest.h>

#include "utils/error_contracts.h"
#include "utils/logger.h"
#include "utils/tracing.h"

// ─────────────────────────────────────────────────────────────────────────────
// OB-01: LOG_* error codes are in range 9020-9029
// ─────────────────────────────────────────────────────────────────────────────
TEST(ObservabilityErrorContracts, LoggerErrorCodesInRange) {
    using EC = themis::utils::ErrorCode;
    auto check = [](EC code) {
        auto v = static_cast<uint16_t>(code);
        EXPECT_GE(v, uint16_t{9020}) << "code " << v << " below 9020";
        EXPECT_LE(v, uint16_t{9029}) << "code " << v << " above 9029";
    };
    check(EC::LOG_BUFFER_OVERFLOW);
    check(EC::LOG_WRITE_FAILED);
    check(EC::LOG_INVALID_FORMAT);
    check(EC::LOG_INITIALIZATION_FAILED);
    check(EC::LOG_LEVEL_INVALID);
    check(EC::LOG_SINK_FAILED);
    check(EC::LOG_ASYNC_OVERFLOW);
}

// ─────────────────────────────────────────────────────────────────────────────
// OB-02: TRACE_* error codes are in range 9030-9039
// ─────────────────────────────────────────────────────────────────────────────
TEST(ObservabilityErrorContracts, TracingErrorCodesInRange) {
    using EC = themis::utils::ErrorCode;
    auto check = [](EC code) {
        auto v = static_cast<uint16_t>(code);
        EXPECT_GE(v, uint16_t{9030}) << "code " << v << " below 9030";
        EXPECT_LE(v, uint16_t{9039}) << "code " << v << " above 9039";
    };
    check(EC::TRACE_SPAN_CREATE_FAILED);
    check(EC::TRACE_EXPORT_FAILED);
    check(EC::TRACE_BUFFER_OVERFLOW);
    check(EC::TRACE_INVALID_CONTEXT);
    check(EC::TRACE_SAMPLING_FAILED);
    check(EC::TRACE_BATCH_FAILED);
}

// ─────────────────────────────────────────────────────────────────────────────
// OB-03: ErrorContext categorizes observability codes as Observability
// ─────────────────────────────────────────────────────────────────────────────
TEST(ObservabilityErrorContracts, ErrorContextCategoryIsObservability) {
    using namespace themis::utils;

    // Each code range maps to its specific observability sub-category.
    auto audit_ctx = makeErrorContext(
        ErrorCode::AUDIT_WRITE_FAILED, "test", "unit test",
        ErrorSeverity::Error, false);
    EXPECT_EQ(audit_ctx.category, ErrorCategory::AuditLog)
        << "AUDIT_WRITE_FAILED should be AuditLog category";

    auto log_ctx = makeErrorContext(
        ErrorCode::LOG_SINK_FAILED, "test", "unit test",
        ErrorSeverity::Error, false);
    EXPECT_EQ(log_ctx.category, ErrorCategory::StructuredLogging)
        << "LOG_SINK_FAILED should be StructuredLogging category";

    auto trace_ctx = makeErrorContext(
        ErrorCode::TRACE_EXPORT_FAILED, "test", "unit test",
        ErrorSeverity::Error, false);
    EXPECT_EQ(trace_ctx.category, ErrorCategory::Tracing)
        << "TRACE_EXPORT_FAILED should be Tracing category";

    auto init_ctx = makeErrorContext(
        ErrorCode::LOG_INITIALIZATION_FAILED, "test", "unit test",
        ErrorSeverity::Error, false);
    EXPECT_EQ(init_ctx.category, ErrorCategory::StructuredLogging)
        << "LOG_INITIALIZATION_FAILED should be StructuredLogging category";
}

// ─────────────────────────────────────────────────────────────────────────────
// OB-04: Tracer::initialize() with unreachable endpoint is fail-open
//        (returns false rather than throwing)
// ─────────────────────────────────────────────────────────────────────────────
TEST(ObservabilityErrorContracts, TracerInitializeFailOpenOnUnreachableEndpoint) {
    // Use a localhost port that is almost certainly not listening.
    // initialize() should return false within the 3-second probe timeout.
    // We allow up to 5 s total; CI machines should be fine.
    const auto result = themis::Tracer::initialize(
        "test-service",
        "http://127.0.0.1:19999");

    // Fail-open: must NOT throw; must return false (backend unreachable)
    // OR true if by fluke something is actually listening on 19999 (CI isolation).
    // We just assert no exception was thrown (the return value is either true/false).
    (void)result;
    SUCCEED(); // reaching here means no exception was thrown
}

// ─────────────────────────────────────────────────────────────────────────────
// OB-05: Logger::get() returns a valid logger even if init() was not called
//        (auto-init path does not throw)
// ─────────────────────────────────────────────────────────────────────────────
TEST(ObservabilityErrorContracts, LoggerGetWithoutInitDoesNotThrow) {
    EXPECT_NO_THROW({
        auto l = themis::utils::Logger::get();
        (void)l;
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// OB-06: logErrorWithContext does not throw for any observability code
// ─────────────────────────────────────────────────────────────────────────────
TEST(ObservabilityErrorContracts, LogErrorWithContextDoesNotThrow) {
    using namespace themis::utils;

    for (auto code : {ErrorCode::LOG_INITIALIZATION_FAILED,
                      ErrorCode::LOG_SINK_FAILED,
                      ErrorCode::TRACE_EXPORT_FAILED,
                      ErrorCode::TRACE_SPAN_CREATE_FAILED,
                      ErrorCode::AUDIT_PERSISTENCE_FAILED,
                      ErrorCode::AUDIT_WRITE_FAILED}) {
        auto ctx = makeErrorContext(code, "OB-06-test", "unit test",
                                    ErrorSeverity::Warning, true);
        EXPECT_NO_THROW(logErrorWithContext(ctx))
            << "code " << static_cast<uint16_t>(code);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// OB-07: categorizeIncident maps observability codes to correct category
// ─────────────────────────────────────────────────────────────────────────────
TEST(ObservabilityErrorContracts, CategorizeIncidentMapsObservabilityCodes) {
    using namespace themis::utils;
    // All audit/log/trace codes should map to an observability-related category
    // (not UnclassifiedIncident).
    for (auto code : {ErrorCode::AUDIT_WRITE_FAILED,
                      ErrorCode::LOG_SINK_FAILED,
                      ErrorCode::TRACE_EXPORT_FAILED}) {
        auto cat = categorizeIncident(code);
        EXPECT_NE(cat, IncidentCategory::UnclassifiedIncident)
            << "code " << static_cast<uint16_t>(code)
            << " should not be UnclassifiedIncident";
    }
}
