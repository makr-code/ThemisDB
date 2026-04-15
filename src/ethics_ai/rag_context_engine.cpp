/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rag_context_engine.cpp                             ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:16:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     329                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 11ddb98b9f  2026-04-09  Add comprehensive documentation and security measures for... ║
    • 172e0dd5e1  2026-03-26  fix: address code review - safe filesystem copy, RFC 4180... ║
    • 490de27f06  2026-03-26  fix: implement all P0/P1 blockers - QueryEngine, RAG, eth... ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "rag_context_engine.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace plugins {
namespace ethics {

RAGContextEngine::RAGContextEngine(std::shared_ptr<ArgumentStore> store)
    : store_(store) {
}

std::variant<RAGContext, Status> RAGContextEngine::buildContext(
    const std::string& dilemma_description,
    const std::vector<std::string>& philosophy_schools,
    const std::string& category) {
    
    RAGContext context;
    
    // Pattern 1: Find similar dilemmas
    auto similar_result = findSimilarDilemmas(dilemma_description, 0.65, 10);
    if (auto* dilemmas = std::get_if<std::vector<std::string>>(&similar_result)) {
        context.similar_dilemmas = *dilemmas;
    }
    
    // Pattern 2: Get philosophy-specific arguments
    for (const auto& school : philosophy_schools) {
        auto args_result = store_->getArgumentsByPhilosophy(school, {}, 20);
        if (auto* args = std::get_if<std::vector<EthicalArgument>>(&args_result)) {
            std::vector<std::string> arg_ids;
            for (const auto& arg : *args) {
                arg_ids.push_back(arg.id);
            }
            context.philosophy_arguments[school] = arg_ids;
        }
    }
    
    // Pattern 3: Get best practices from stored decisions
    auto best_practices_result = getBestPractices(category, 0.8, 10);
    if (auto* practices = std::get_if<std::vector<std::string>>(&best_practices_result)) {
        context.best_practices = *practices;
    }
    
    // Pattern 2b: Record relevance scores for retrieved arguments
    for (auto& [school, arg_ids] : context.philosophy_arguments) {
        for (const auto& id : arg_ids) {
            auto arg_result = store_->getArgument(id);
            if (auto* arg = std::get_if<EthicalArgument>(&arg_result)) {
                context.relevance_scores[id] =
                    calculateTextSimilarity(dilemma_description, arg->content);
            }
        }
    }

    return context;
}

std::variant<std::vector<std::string>, Status> RAGContextEngine::findSimilarDilemmas(
    const std::string& query_text,
    double threshold,
    size_t limit) {
    
    if (!store_) {
        return std::vector<std::string>{};
    }

    // Iterate all philosophy schools and collect arguments; compare their
    // content against the query using Jaccard-based text similarity.
    static const std::vector<std::string> kSchools = {
        "kant", "utilitarianism", "virtue_ethics", "rawls",
        "contractarianism", "care_ethics", "natural_law"
    };

    std::vector<std::pair<double, std::string>> scored;
    for (const auto& school : kSchools) {
        auto result = store_->getArgumentsByPhilosophy(school, {}, 50);
        if (auto* args = std::get_if<std::vector<EthicalArgument>>(&result)) {
            for (const auto& arg : *args) {
                double sim = calculateTextSimilarity(query_text, arg.content);
                if (sim >= threshold) {
                    scored.emplace_back(sim, arg.id);
                }
            }
        }
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<std::string> results;
    results.reserve(std::min(scored.size(), limit));
    for (size_t i = 0; i < scored.size() && i < limit; ++i) {
        results.push_back(scored[i].second);
    }
    return results;
}

std::variant<std::vector<std::string>, Status> RAGContextEngine::getBestPractices(
    const std::string& category,
    double min_satisfaction,
    size_t limit) {
    
    if (!store_) {
        return std::vector<std::string>{};
    }

    // Collect arguments whose strength score meets min_satisfaction.
    // ArgumentStrength::COMPELLING ≥ 1.0, HIGH ≥ 0.85, MODERATE ≥ 0.65, WEAK ≥ 0.35.
    static const std::vector<std::string> kSchools = {
        "kant", "utilitarianism", "virtue_ethics", "rawls",
        "contractarianism", "care_ethics", "natural_law"
    };

    std::vector<std::string> results;
    for (const auto& school : kSchools) {
        if (results.size() >= limit) break;
        auto res = store_->getArgumentsByPhilosophy(school, {}, 50);
        if (auto* args = std::get_if<std::vector<EthicalArgument>>(&res)) {
            for (const auto& arg : *args) {
                if (results.size() >= limit) break;
                double strength_score = 0.5;
                switch (arg.strength) {
                    case ArgumentStrength::DECISIVE: strength_score = 1.0;  break;
                    case ArgumentStrength::STRONG:   strength_score = 0.85; break;
                    case ArgumentStrength::MODERATE: strength_score = 0.65; break;
                    case ArgumentStrength::WEAK:     strength_score = 0.35; break;
                    default: break;
                }
                if (strength_score >= min_satisfaction) {
                    double relevance = category.empty()
                        ? 1.0
                        : calculateTextSimilarity(category, arg.content);
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
RAGContextEngine::vectorSemanticSearch(
    const std::vector<float>& query_embedding,
    const std::string& philosophy_school,
    size_t limit) {
    
    if (!store_ || query_embedding.empty()) {
        return std::vector<std::pair<std::string, double>>{};
    }

    static const std::vector<std::string> kSchools = {
        "kant", "utilitarianism", "virtue_ethics", "rawls",
        "contractarianism", "care_ethics", "natural_law"
    };
    const auto& schools = philosophy_school.empty()
        ? kSchools
        : std::vector<std::string>{philosophy_school};

    std::vector<std::pair<double, std::string>> scored;
    for (const auto& school : schools) {
        auto res = store_->getArgumentsByPhilosophy(school, {}, 200);
        if (auto* args = std::get_if<std::vector<EthicalArgument>>(&res)) {
            for (const auto& arg : *args) {
                std::vector<float> arg_emb = generateEmbedding(arg.content);
                if (arg_emb.size() != query_embedding.size()) continue;
                double dot = 0.0, qnorm = 0.0, anorm = 0.0;
                for (size_t i = 0; i < query_embedding.size(); ++i) {
                    dot   += static_cast<double>(query_embedding[i]) * arg_emb[i];
                    qnorm += static_cast<double>(query_embedding[i]) * query_embedding[i];
                    anorm += static_cast<double>(arg_emb[i]) * arg_emb[i];
                }
                double denom = std::sqrt(qnorm) * std::sqrt(anorm);
                double cosine = (denom > 0.0) ? (dot / denom) : 0.0;
                scored.emplace_back(cosine, arg.id);
            }
        }
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<std::pair<std::string, double>> results;
    results.reserve(std::min(scored.size(), limit));
    for (size_t i = 0; i < scored.size() && i < limit; ++i) {
        results.emplace_back(scored[i].second, scored[i].first);
    }
    return results;
}

std::variant<std::vector<std::string>, Status> RAGContextEngine::traverseArgumentChain(
    const std::string& start_argument_id,
    size_t max_depth,
    const std::string& direction) {
    
    if (!store_ || start_argument_id.empty()) {
        return std::vector<std::string>{};
    }

    // BFS traversal following `supports` / `counterarguments` links.
    std::vector<std::string> visited_order;
    std::unordered_set<std::string> visited;
    // Use a deque for proper FIFO BFS ordering.
    // Each entry: (argument_id, depth)
    std::deque<std::pair<std::string, size_t>> frontier;
    frontier.emplace_back(start_argument_id, 0);
    visited.insert(start_argument_id);
    visited_order.push_back(start_argument_id);

    while (!frontier.empty()) {
        auto [current_id, depth] = frontier.front();
        frontier.pop_front();
        if (depth >= max_depth) continue;

        auto arg_result = store_->getArgument(current_id);
        if (auto* arg = std::get_if<EthicalArgument>(&arg_result)) {
            auto visit_list = [&](const std::vector<std::string>& ids) {
                for (const auto& nid : ids) {
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

double RAGContextEngine::calculateTextSimilarity(
    const std::string& text1, 
    const std::string& text2) {
    
    if (text1 == text2) return 1.0;
    if (text1.empty() || text2.empty()) return 0.0;

    // Jaccard similarity on word sets (case-insensitive)
    auto tokenize = [](const std::string& s) {
        std::unordered_set<std::string> tokens;
        std::istringstream iss(s);
        std::string word;
        while (iss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            while (!word.empty() &&
                   !std::isalpha(static_cast<unsigned char>(word.back()))) {
                word.pop_back();
            }
            if (!word.empty()) tokens.insert(word);
        }
        return tokens;
    };

    auto set1 = tokenize(text1);
    auto set2 = tokenize(text2);
    if (set1.empty() && set2.empty()) return 1.0;
    if (set1.empty() || set2.empty()) return 0.0;

    size_t intersection_count = 0;
    for (const auto& tok : set1) {
        if (set2.count(tok)) ++intersection_count;
    }
    size_t union_count = set1.size() + set2.size() - intersection_count;
    return union_count > 0
        ? static_cast<double>(intersection_count) / union_count
        : 0.0;
}

std::vector<float> RAGContextEngine::generateEmbedding(const std::string& text) {
    // Bag-of-characters TF embedding (dimension 768, L2-normalised).
    // Each byte is hashed into one of 768 buckets.  Deterministic and
    // lightweight — suitable when a real embedding model is not configured.
    constexpr size_t kDim = 768;
    std::vector<float> emb(kDim, 0.0f);
    if (text.empty()) return emb;

    for (unsigned char c : text) {
        emb[c % kDim] += 1.0f;
    }

    float norm = 0.0f;
    for (float v : emb) norm += v * v;
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (float& v : emb) v /= norm;
    }
    return emb;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
