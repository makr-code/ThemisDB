/**
 * @file test_updates_diagnostics_focused.cpp
 * @brief Focused tests for updates diagnostics and formatting.
 */

#include <gtest/gtest.h>

#include "updates/updates_diagnostic_emitter.h"
#include "updates/updates_diagnostics.h"

#include <memory>
#include <mutex>
#include <vector>

namespace themis {
namespace updates {

namespace {

class RecordingListener : public DiagnosticListener {
public:
    struct Event {
        ErrorContext context;
        bool is_error;
    };

    void onDiagnosticEvent(const ErrorContext& context, bool is_error) override {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back({context, is_error});
    }

    size_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.size();
    }

    Event last() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_.back();
    }

private:
    mutable std::mutex mutex_;
    std::vector<Event> events_;
};

} // namespace

class DiagnosticEmitterTest : public ::testing::Test {
protected:
    void SetUp() override {
        listener_ = std::make_shared<RecordingListener>();
        emitter_.addListener(listener_);
    }

    DiagnosticEmitter emitter_;
    std::shared_ptr<RecordingListener> listener_;
};

TEST_F(DiagnosticEmitterTest, FormatErrorMessageContainsKeyFields) {
    ErrorContext ctx;
    ctx.error_code = DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH;
    ctx.severity = DiagnosticSeverity::ERROR;
    ctx.root_cause = RootCauseClass::CHECKSUM;
    ctx.message = "SHA256 mismatch";
    ctx.operation = "verify_patch";
    ctx.phase = "verifying";
    ctx.node_id = "node-a";
    ctx.version = "1.7.0";

    const auto formatted = DiagnosticEmitter::formatErrorMessage(ctx);

    EXPECT_NE(formatted.find("PATCH_CHECKSUM_MISMATCH"), std::string::npos);
    EXPECT_NE(formatted.find("verify_patch"), std::string::npos);
    EXPECT_NE(formatted.find("verifying"), std::string::npos);
    EXPECT_NE(formatted.find("node-a"), std::string::npos);
}

TEST_F(DiagnosticEmitterTest, EmitErrorNotifiesListener) {
    ErrorContext ctx;
    ctx.error_code = DiagnosticErrorCode::PATCH_APPLY_FAILED;
    ctx.severity = DiagnosticSeverity::ERROR;
    ctx.root_cause = RootCauseClass::ARTIFACT;
    ctx.message = "Failed to apply patch";
    ctx.operation = "apply_patch";
    ctx.phase = "applying";

    emitter_.emitError(ctx);

    ASSERT_EQ(listener_->count(), 1u);
    const auto event = listener_->last();
    EXPECT_TRUE(event.is_error);
    EXPECT_EQ(event.context.error_code, DiagnosticErrorCode::PATCH_APPLY_FAILED);
}

TEST_F(DiagnosticEmitterTest, EmitInfoNotifiesListener) {
    emitter_.emitInfo("state_transition", "applying", "Transitioned", "", "1.7.0");

    ASSERT_EQ(listener_->count(), 1u);
    const auto event = listener_->last();
    EXPECT_FALSE(event.is_error);
    EXPECT_EQ(event.context.operation, "state_transition");
}

} // namespace updates
} // namespace themis
