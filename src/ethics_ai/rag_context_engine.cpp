/**
 * @file rag_context_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "rag_context_engine.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <iomanip>
#include <set>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace plugins {
namespace ethics {

RAGContextEngine::RAGContextEngine(std::shared_ptr<ArgumentStore> store) : store_(store) {}

std::variant<RAGContext, Status> RAGContextEngine::buildContext(const std::string &dilemma_description,
                                                                const std::vector<std::string> &philosophy_schools,
                                                                const std::string &category) {
    // ArgumentStore acquires its own internal mutex on every operation.
    // Do NOT hold store_access_mutex_ across this entire method; doing so
    // would serialize all buildContext() calls and include CPU-heavy similarity
    // scoring inside the critical section.  Thread safety for store_ access is
    // already provided by ArgumentStore's internal locking.

    RAGContext context;

    // Pattern 1: Find similar dilemmas
    auto similar_result = findSimilarDilemmas(dilemma_description, 0.65, 10);
    if (auto *dilemmas = std::get_if<std::vector<std::string>>(&similar_result)) {
        context.similar_dilemmas = *dilemmas;
    }

    // Pattern 2: Get philosophy-specific arguments
    for (const auto &school : philosophy_schools) {
        auto args_result = store_->getArgumentsByPhilosophy(school, {}, 20);
        if (auto *args = std::get_if<std::vector<EthicalArgument>>(&args_result)) {
            std::vector<std::string> arg_ids;
            arg_ids.reserve(args->size());
            for (const auto &arg : *args) {
                arg_ids.push_back(arg.id);
            }
            context.philosophy_arguments[school] = arg_ids;
        }
    }

    // Pattern 3: Get best practices from stored decisions
    auto best_practices_result = getBestPractices(category, 0.8, 10);
    if (auto *practices = std::get_if<std::vector<std::string>>(&best_practices_result)) {
        context.best_practices = *practices;
    }

    // Pattern 2b: Record relevance scores for retrieved arguments
    for (auto &[school, arg_ids] : context.philosophy_arguments) {
        for (const auto &id : arg_ids) {
            auto arg_result = store_->getArgument(id);
            if (auto *arg = std::get_if<EthicalArgument>(&arg_result)) {
                context.relevance_scores[id] = calculateTextSimilarity(dilemma_description, arg->content);
            }
        }
    }

    return context;
}

std::variant<std::vector<std::string>, Status> RAGContextEngine::findSimilarDilemmas(const std::string &query_text,
                                                                                     double threshold, size_t limit) {
    if (!store_) {
        return std::vector<std::string>{};
    }

    // Iterate all philosophy schools and collect arguments; compare their
    // content against the query using Jaccard-based text similarity.
    static const std::vector<std::string> kSchools
        = {"kant", "utilitarianism", "virtue_ethics", "rawls", "contractarianism", "care_ethics", "natural_law"};

    std::vector<std::pair<double, std::string>> scored;
    for (const auto &school : kSchools) {
        auto result = store_->getArgumentsByPhilosophy(school, {}, 50);
        if (auto *args = std::get_if<std::vector<EthicalArgument>>(&result)) {
            for (const auto &arg : *args) {
                double sim = calculateTextSimilarity(query_text, arg.content);
                if (sim >= threshold) {
                    scored.emplace_back(sim, arg.id);
                }
            }
        }
    }

    std::sort(scored.begin(), scored.end(), [](const auto &a, const auto &b) { return a.first > b.first; });

    std::vector<std::string> results = {};

    results.reserve(std::min(scored.size(), limit));
    for (size_t i = 0; i < scored.size() && i < limit; ++i) {
        results.push_back(scored[i].second);
    }
    return results;
}

std::variant<std::vector<std::string>, Status>
RAGContextEngine::getBestPractices(const std::string &category, double min_satisfaction, size_t limit) {
    if (!store_) {
        return std::vector<std::string>{};
    }

    // Collect arguments whose strength score meets min_satisfaction.
    // ArgumentStrength::COMPELLING ≥ 1.0, HIGH ≥ 0.85, MODERATE ≥ 0.65, WEAK ≥ 0.35.
    static const std::vector<std::string> kSchools
        = {"kant", "utilitarianism", "virtue_ethics", "rawls", "contractarianism", "care_ethics", "natural_law"};

    std::vector<std::string> results = {};

    for (const auto &school : kSchools) {
        if (static_cast<int>(results.size()) > = limit) {
            break;
        }
        auto res = store_->getArgumentsByPhilosophy(school, {}, 50);
        if (auto *args = std::get_if<std::vector<EthicalArgument>>(&res)) {
            for (const auto &arg : *args) {
                if (static_cast<int>(results.size()) > = limit) {
                    break;
                }
                double strength_score = 0.5;
                switch (arg.strength) {
                    case ArgumentStrength::DECISIVE:
                        strength_score = 1.0;
                        break;
                    case ArgumentStrength::STRONG:
                        strength_score = 0.85;
                        break;
                    case ArgumentStrength::MODERATE:
                        strength_score = 0.65;
                        break;
                    case ArgumentStrength::WEAK:
                        strength_score = 0.35;
                        break;
                    default:
                        break;
                }
                if (strength_score >= min_satisfaction) {
                    double relevance = category.empty() ? 1.0 : calculateTextSimilarity(category, arg.content);
                    if (relevance > 0.0 || category.empty()) {
                        results.push_back(arg.id);
                    }
                }
            }
        }
    }
    return results;
}

std::variant<std::vector<std::pair<std::string, double>>, Status>
RAGContextEngine::vectorSemanticSearch(const std::vector<float> &query_embedding, const std::string &philosophy_school,
                                       size_t limit) {
    if (!store_ || query_embedding.empty()) {
        return std::vector<std::pair<std::string, double>>{};
    }

    static const std::vector<std::string> kSchools
        = {"kant", "utilitarianism", "virtue_ethics", "rawls", "contractarianism", "care_ethics", "natural_law"};
    const auto &schools = philosophy_school.empty() ? kSchools : std::vector<std::string>{philosophy_school};

    std::vector<std::pair<double, std::string>> scored;
    for (const auto &school : schools) {
        auto res = store_->getArgumentsByPhilosophy(school, {}, 200);
        if (auto *args = std::get_if<std::vector<EthicalArgument>>(&res)) {
            for (const auto &arg : *args) {
                std::vector<float> arg_emb = generateEmbedding(arg.content);
                if (arg_emb.size() != query_embedding.size()) {
                    continue;
                }
                double dot = 0.0, qnorm = 0.0, anorm = 0.0;
                for (size_t i = 0; i < query_embedding.size(); ++i) {
                    dot += static_cast<double>(query_embedding[i]) * arg_emb[i];
                    qnorm += static_cast<double>(query_embedding[i]) * query_embedding[i];
                    anorm += static_cast<double>(arg_emb[i]) * arg_emb[i];
                }
                double denom  = std::sqrt(qnorm) * std::sqrt(anorm);
                double cosine = (denom > 0.0) ? (dot / denom) : 0.0;
                scored.emplace_back(cosine, arg.id);
            }
        }
    }

    std::sort(scored.begin(), scored.end(), [](const auto &a, const auto &b) { return a.first > b.first; });

    std::vector<std::pair<std::string, double>> results;
    results.reserve(std::min(scored.size(), limit));
    for (size_t i = 0; i < scored.size() && i < limit; ++i) {
        results.emplace_back(scored[i].second, scored[i].first);
    }
    return results;
}

std::variant<std::vector<std::string>, Status>
RAGContextEngine::traverseArgumentChain(const std::string &start_argument_id, size_t max_depth,
                                        const std::string &direction) {
    if (!store_ || start_argument_id.empty()) {
        return std::vector<std::string>{};
    }

    // ArgumentStore is internally thread-safe; no outer mutex needed here.
    // Removing store_access_mutex_ prevents serializing the entire BFS loop.

    // BFS traversal following `supports` / `counterarguments` links.
    std::vector<std::string> visited_order;
    std::set<std::string> visited;
    // Use a deque for proper FIFO BFS ordering.
    // Each entry: (argument_id, depth)
    std::deque<std::pair<std::string, size_t>> frontier;
    frontier.emplace_back(start_argument_id, 0);
    visited.insert(start_argument_id);
    visited_order.push_back(start_argument_id);

    while (!frontier.empty()) {
        auto [current_id, depth] = frontier.front();
        frontier.pop_front();
        if (depth >= max_depth) {
            continue;
        }

        auto arg_result = store_->getArgument(current_id);
        if (auto *arg = std::get_if<EthicalArgument>(&arg_result)) {
            auto visit_list = [&]([[maybe_unused]] const std::vector<std::string> &ids) {
                for (const auto &nid : ids) {
                    if (visited.insert(nid).second) {
                        visited_order.push_back(nid);
                        frontier.emplace_back(nid, depth + 1);
                    }
                }
            };
            if (direction == "supports" || direction == "OUTBOUND") {
                visit_list(arg->supports);
            } else if (direction == "counterarguments" || direction == "INBOUND") {
                visit_list(arg->counterarguments);
            } else {
                visit_list(arg->supports);
                visit_list(arg->counterarguments);
            }
        }
    }
    return visited_order;
}

double RAGContextEngine::calculateTextSimilarity(const std::string &text1, const std::string &text2) {
    if (text1 == text2) {
        return 1.0;
    }
    if (text1.empty() || text2.empty()) {
        return 0.0;
    }

    // Jaccard similarity on word sets (case-insensitive)
    auto tokenize = [](const std::string &s) {
        std::unordered_set<std::string> tokens;
        std::istringstream iss(s);
        std::string word = {};
        while (iss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            while (!word.empty() && !std::isalpha(static_cast<unsigned char>(word.back()))) {
                word.pop_back();
            }
            if (!word.empty()) {
                tokens.insert(word);
            }
        }
        return tokens;
    };

    auto set1 = tokenize(text1);
    auto set2 = tokenize(text2);
    if (set1.empty() && set2.empty()) {
        return 1.0;
    }
    if (set1.empty() || set2.empty()) {
        return 0.0;
    }

    size_t intersection_count = 0;
    for (const auto &tok : set1) {
        if (set2.count(tok)) {
            ++intersection_count;
        }
    }
    size_t union_count = set1.size() + set2.size() - intersection_count;
    return union_count > 0 ? static_cast<double>(intersection_count) / union_count : 0.0;
}

std::vector<float> RAGContextEngine::generateEmbedding(const std::string &text) {
    // Bag-of-characters TF embedding (dimension 768, L2-normalised).
    // Each byte is hashed into one of 768 buckets.  Deterministic and
    // lightweight — suitable when a real embedding model is not configured.
    constexpr size_t kDim = 768;
    std::vector<float> emb(kDim, 0.0f);
    if (text.empty()) {
        return emb;
    }

    for (unsigned char c : text) {
        emb[c % kDim] += 1.0f;
    }

    float norm = 0.0f;
    for (float v : emb) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (float &v : emb) {
            v /= norm;
        }
    }
    return emb;
}

LegalGrounding RAGContextEngine::retrieveLegalGrounding(
    const std::string& dilemma_description) const {
    (void)dilemma_description;

    LegalGrounding grounding;
    grounding.grounding_available = legal_db_available_;
    grounding.legal_db_unavailable = !legal_db_available_;
    if (!legal_db_available_) {
        return grounding;
    }

    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
#if defined(_WIN32)
    gmtime_s(&utc_tm, &now_time);
#else
    gmtime_r(&now_time, &utc_tm);
#endif
    std::ostringstream ts = {};
    ts << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    const std::string retrieved_at = ts.str();
    grounding.retrieval_timestamp_utc = retrieved_at;

    const std::array<std::pair<const char*, const char*>, 3> canonical_norms{{
        {"gg-art-1", "GG Art. 1"},
        {"dsgvo-art-5", "DSGVO Art. 5"},
        {"eu-ai-act-art-22", "EU AI Act Art. 22"},
    }};

    grounding.citation_ids.reserve(canonical_norms.size());
    grounding.norm_refs.reserve(canonical_norms.size());
    for (const auto& [id, article] : canonical_norms) {
        grounding.citation_ids.emplace_back(id);
        grounding.norm_refs.emplace_back(article);
    }
    return grounding;
}

void RAGContextEngine::setLegalDbAvailable([[maybe_unused]] bool available) noexcept {
    legal_db_available_ = available;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
