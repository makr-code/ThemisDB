#pragma once

#include "themis/search/llm_reranker_interface.h"
#include <memory>

namespace themis {
namespace search {

using LlmRerankerFactory = std::function<std::unique_ptr<ILlmReranker>(const ILlmReranker::Config&)>;

void registerLlmRerankerFactory(LlmRerankerFactory f);
std::unique_ptr<ILlmReranker> createLlmReranker(const ILlmReranker::Config& cfg = ILlmReranker::Config::defaults());

} // namespace search
} // namespace themis
