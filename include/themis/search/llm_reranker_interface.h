#pragma once

#include "themis/search/llm_reranker_types.h"
#include <memory>
#include <functional>

namespace themis {

struct LlmRerankCandidate;
struct LlmRerankResult;

class ILlmReranker {
public:
    using LlmBackend = std::function<std::string(const std::string&)>;
    struct Config {
        size_t batch_size = 5;
        double llm_weight = 0.7;
        size_t max_snippet_length = 200;
        bool fallback_to_original = true;
        int max_tokens = 256;
        float temperature = 0.0f;
        double min_score_threshold = 0.0;
        /**
         * @brief TBD: Describe defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };

    /**
     * @brief TBD: Describe ~ILlmReranker.
     * @return Return value.
     */
    virtual ~ILlmReranker() = default;
    /**
     * @brief TBD: Describe setBackend.
     * @param[in] backend Input parameter.
     */
    virtual void setBackend(LlmBackend backend) = 0;
    /**
     * @brief TBD: Describe rerank.
     * @param[in] query Input parameter.
     * @param[in] candidates Input parameter.
     * @return Return value.
     */
    virtual std::vector<LlmRerankResult> rerank(const std::string& query,
                                                const std::vector<LlmRerankCandidate>& candidates) const = 0;
    /**
     * @brief TBD: Describe getConfig.
     * @return Return value.
     */
    virtual const Config& getConfig() const = 0;
};

} // namespace themis
