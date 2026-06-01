/**
 * @file graph_validator.cc
 * @brief Graph Truth Validation Layer implementation stub.
 *
 * Skeleton: factory and minimal concrete class.  Replace with production
 * knowledge-graph traversal in sub-issue #5426.
 */

#include "retrieval/include/graph_validator.h"

namespace themis::retrieval {

namespace {

class GraphValidatorImpl final : public IGraphValidator {
public:
    explicit GraphValidatorImpl(GraphValidatorConfig cfg)
        : cfg_(std::move(cfg)) {}

    GraphResult validate(const GraphQuery& query) override {
        // TODO(#5426): Implement knowledge-graph traversal and evidence assembly.
        GraphResult result;
        result.validated_candidates = query.candidates;
        result.graph_available = false; // Stub: no graph connected yet
        return result;
    }

    bool isAvailable() const override {
        return false; // TODO(#5426): Check graph backend connectivity.
    }

    std::optional<GraphEvidence> lookupProvenance(
        const std::string& /*provenance_id*/) const override {
        // TODO(#5426): Implement provenance lookup.
        return std::nullopt;
    }

private:
    GraphValidatorConfig cfg_;
};

} // namespace

std::unique_ptr<IGraphValidator> makeGraphValidator(const GraphValidatorConfig& cfg) {
    return std::make_unique<GraphValidatorImpl>(cfg);
}

} // namespace themis::retrieval
