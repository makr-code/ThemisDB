/**
 * @file tensor_midlayer.cc
 * @brief Tensor Mid-Layer implementation stub.
 *
 * Skeleton: factory and minimal concrete class.  Replace with production
 * compression-aware reranking logic in sub-issue #5425.
 */

#include "retrieval/include/tensor_midlayer.h"

namespace themis::retrieval {

namespace {

class TensorMidlayerImpl final : public ITensorMidlayer {
public:
    explicit TensorMidlayerImpl(TensorMidlayerConfig cfg)
        : cfg_(std::move(cfg)) {}

    TensorResult rerank(const TensorQuery& query) override {
        // TODO(#5425): Implement compression-aware reranking.
        TensorResult result;
        result.reranked = query.ann_candidates;
        return result;
    }

    void warmCache(const std::string& /*shard_key*/) override {
        // TODO(#5425): Load summaries into local cache.
    }

    void evictCache(const std::string& /*shard_key*/) override {
        // TODO(#5425): Evict shard summaries from local cache.
    }

    TensorCompression activeCompression() const override {
        return cfg_.compression;
    }

private:
    TensorMidlayerConfig cfg_;
};

} // namespace

std::unique_ptr<ITensorMidlayer> makeTensorMidlayer(const TensorMidlayerConfig& cfg) {
    return std::make_unique<TensorMidlayerImpl>(cfg);
}

} // namespace themis::retrieval
