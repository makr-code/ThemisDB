/**
 * @file ann_frontdoor.cc
 * @brief ANN Frontdoor implementation stub.
 *
 * Skeleton: factory and minimal concrete class.  Replace with production
 * HNSW / DiskANN backend integration in sub-issue #5424.
 */

#include "retrieval/include/ann_frontdoor.h"

#include <stdexcept>

namespace themis::retrieval {

namespace {

class AnnFrontdoorImpl final : public IAnnFrontdoor {
public:
    explicit AnnFrontdoorImpl(AnnFrontdoorConfig cfg)
        : cfg_(std::move(cfg)), route_(AnnRoute::Hot) {}

    AnnResult search(const AnnQuery& /*query*/) override {
        // TODO(#5424): Implement HNSW / DiskANN search dispatch.
        return AnnResult{.route = route_};
    }

    AnnRoute getRoute() const override { return route_; }

    void setBackend(AnnBackend backend) override {
        cfg_.preferred_backend = backend;
    }

    void onSearch(SearchCallback cb) override {
        callback_ = std::move(cb);
    }

private:
    AnnFrontdoorConfig cfg_;
    AnnRoute           route_;
    SearchCallback     callback_;
};

} // namespace

std::unique_ptr<IAnnFrontdoor> makeAnnFrontdoor(const AnnFrontdoorConfig& cfg) {
    return std::make_unique<AnnFrontdoorImpl>(cfg);
}

} // namespace themis::retrieval
