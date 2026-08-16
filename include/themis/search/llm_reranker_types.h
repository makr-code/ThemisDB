#pragma once

#include <string>
#include <vector>

namespace themis {

struct LlmRerankCandidate {
    std::string document_id;
    std::string content;
    double initial_score = 0.0;
};

struct LlmRerankResult {
    std::string document_id;
    double llm_score = 0.0;
    double initial_score = 0.0;
    double final_score = 0.0;
    bool llm_scored = false;
};

} // namespace themis
