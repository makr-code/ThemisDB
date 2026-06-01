/**
 * @file retrieval_observability.cc
 * @brief Retrieval observability provider implementation stub.
 *
 * Skeleton: no-op factory for safe default use in tests and early integration.
 * Replace with a Prometheus / OpenTelemetry backend in sub-issue 1.7.
 */

#include "retrieval/include/retrieval_observability.h"

namespace themis::retrieval {

namespace {

class NoopObservability final : public IRetrievalObservability {
public:
    void emitSpan(const RetrievalSpan& /*span*/) override {}

    void emitTrace(const RetrievalTrace& /*trace*/) override {}

    GovernanceDecision checkGovernance(
        const GovernanceContext& /*ctx*/) const override {
        return GovernanceDecision::Allow;
    }

    void incrementCounter(const std::string& /*name*/,
                           double /*delta*/) override {}

    void recordHistogram(const std::string& /*name*/,
                          double /*value*/) override {}

    void onGovernanceBlock(BlockCallback /*cb*/) override {}
};

} // namespace

std::unique_ptr<IRetrievalObservability> makeNoopObservability() {
    return std::make_unique<NoopObservability>();
}

} // namespace themis::retrieval
