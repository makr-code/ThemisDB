/**
 * @file lora_enhanced_retriever.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "rag/lora_enhanced_retriever.h"
#include <stdexcept>
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace themis::rag {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Tokenise @p text into a set of lowercase alpha-numeric tokens.
std::unordered_set<std::string> tokeniseSet(const std::string& text) {
    std::unordered_set<std::string> tokens;
    std::string tok;
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            tok += static_cast<char>(std::tolower(ch));
        } else {
            if (!tok.empty()) {
                tokens.insert(tok);
                tok.clear();
            }
        }
    }
    if (!tok.empty()) {
      tokens.insert(tok);
    }
    return tokens;
}

/// Jaccard similarity between two token sets.
double jaccardTokenSets(const std::unordered_set<std::string>& A,
                        const std::unordered_set<std::string>& B)
{
    if (A.empty() && B.empty()) {
      return 1.0;
    }
    if (A.empty() || B.empty()) {
      return 0.0;
    }

    std::size_t inter = 0;
    for (const auto& t : A) {
        if (B.count(t)) {
          ++inter;
        }
    }
    const std::size_t uni = A.size() + B.size() - inter;
    return uni == 0 ? 0.0 : static_cast<double>(inter) / static_cast<double>(uni);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// HeuristicLoRAScorer
// ─────────────────────────────────────────────────────────────────────────────

HeuristicLoRAScorer::HeuristicLoRAScorer(std::string domain_hint)
    : domain_(std::move(domain_hint))
{}

double HeuristicLoRAScorer::score(const std::string& query,
                                   const std::string& content,
                                   const std::string& /*domain*/)
{
    // Token-overlap Jaccard as a lightweight proxy for domain relevance.
    return jaccardTokenSets(tokeniseSet(query), tokeniseSet(content));
}

std::string HeuristicLoRAScorer::domain() const {
    return domain_;
}

// ─────────────────────────────────────────────────────────────────────────────
// LoRAEnhancedRetriever
// ─────────────────────────────────────────────────────────────────────────────

LoRAEnhancedRetriever::LoRAEnhancedRetriever(std::shared_ptr<ILoRAScorer> scorer,
                                             LoRARetrieverConfig          config)
    : scorer_(std::move(scorer))
    , config_(std::move(config))
{}

LoRAEnhancedRetriever::~LoRAEnhancedRetriever() = default;

const LoRARetrieverConfig& LoRAEnhancedRetriever::config() const noexcept {
    return config_;
}

void LoRAEnhancedRetriever::setConfig(const LoRARetrieverConfig& config) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    config_ = config;
}

void LoRAEnhancedRetriever::setScorer(std::shared_ptr<ILoRAScorer> scorer) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    scorer_ = std::move(scorer);
}

std::vector<judge::RetrievedDocument>
LoRAEnhancedRetriever::rerank(
    const std::string&                           query,
    const std::vector<judge::RetrievedDocument>& candidates) const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!scorer_ || candidates.empty()) {
        return candidates;
    }

    const std::size_t rerank_k =
        std::min(config_.top_k_rerank, candidates.size());

    // Split into top-K (to re-rank) and the rest (append unchanged).
    std::vector<judge::RetrievedDocument> to_rerank(
        candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(rerank_k));
    std::vector<judge::RetrievedDocument> tail(
        candidates.begin() + static_cast<std::ptrdiff_t>(rerank_k), candidates.end());

    const double w = config_.lora_weight;

    // Score each document in the top-K slice.
    for (auto& doc : to_rerank) {
        const double lora_s = scorer_->score(query, doc.content, config_.domain);
        const double fused  = doc.similarity_score * (1.0 - w) + lora_s * w;

        THEMIS_DEBUG("LoRAEnhancedRetriever: doc='{}' orig={:.3f} lora={:.3f} fused={:.3f}",
                     doc.id, doc.similarity_score, lora_s, fused);

        doc.metadata["lora_score"]  = std::to_string(lora_s);
        doc.similarity_score        = fused;
    }

    // Sort re-ranked slice by fused score descending.
    std::stable_sort(to_rerank.begin(), to_rerank.end(),
        [](const judge::RetrievedDocument& a, const judge::RetrievedDocument& b) {
            return a.similarity_score > b.similarity_score;
        });

    // Apply min_lora_score filter: move below-threshold docs to tail.
    if (config_.min_lora_score > 0.0) {
        auto partition_it = std::stable_partition(
            to_rerank.begin(), to_rerank.end(),
            [&]([[maybe_unused]] const judge::RetrievedDocument& d) {
                auto it = d.metadata.find("lora_score");
                if (it == d.metadata.end()) {
                  return true;
                }
                try {
                    return std::stod(it->second) >= config_.min_lora_score;
                } catch (...) {
                    return true;
                }
            });
        tail.insert(tail.begin(), partition_it, to_rerank.end());
        to_rerank.erase(partition_it, to_rerank.end());
    }

    // Combine: re-ranked set first, then tail.
    std::vector<judge::RetrievedDocument> result;
    result.reserve(candidates.size());
    result.insert(result.end(),
                  std::make_move_iterator(to_rerank.begin()),
                  std::make_move_iterator(to_rerank.end()));
    result.insert(result.end(),
                  std::make_move_iterator(tail.begin()),
                  std::make_move_iterator(tail.end()));

    THEMIS_INFO("LoRAEnhancedRetriever::rerank complete: {} docs (re-ranked {})",
                result.size(), rerank_k);

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

/*static*/
std::unique_ptr<LoRAEnhancedRetriever>
LoRAEnhancedRetrieverFactory::createLightweight() {
    LoRARetrieverConfig cfg;
    cfg.top_k_rerank = 20;
    cfg.lora_weight  = 0.2;
    auto scorer = std::make_shared<HeuristicLoRAScorer>();
    return std::make_unique<LoRAEnhancedRetriever>(std::move(scorer), cfg);
}

/*static*/
std::unique_ptr<LoRAEnhancedRetriever>
LoRAEnhancedRetrieverFactory::createBalanced(const std::string& domain) {
    LoRARetrieverConfig cfg;
    cfg.top_k_rerank = 50;
    cfg.lora_weight  = 0.35;
    cfg.domain       = domain;
    auto scorer = std::make_shared<HeuristicLoRAScorer>(domain);
    return std::make_unique<LoRAEnhancedRetriever>(std::move(scorer), cfg);
}

/*static*/
std::unique_ptr<LoRAEnhancedRetriever>
LoRAEnhancedRetrieverFactory::createDomainSpecific(const std::string& domain) {
    LoRARetrieverConfig cfg;
    cfg.top_k_rerank = 50;
    cfg.lora_weight  = 0.5;
    cfg.domain       = domain;
    auto scorer = std::make_shared<HeuristicLoRAScorer>(domain);
    return std::make_unique<LoRAEnhancedRetriever>(std::move(scorer), cfg);
}

} // namespace themis::rag

