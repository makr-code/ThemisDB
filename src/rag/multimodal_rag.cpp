/**
 * @file multimodal_rag.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/multimodal_rag.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace themis::rag::multimodal {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/**
 * Apply Reciprocal Rank Fusion (RRF) across multiple ranked lists.
 *
 * Each list entry is (document_id, raw_score).  The RRF score for a
 * document across all lists is:  Σ_i ( weight_i / (k + rank_i) )
 *
 * @param ranked_lists   Per-modality ranked lists (sorted by raw score DESC).
 * @param weights        Per-list weight (must be same size as ranked_lists).
 * @param modality_names Per-list modality label for populating matched_modality.
 * @param rrf_k          RRF smoothing constant.
 * @param max_results    Maximum number of results to return.
 * @return               Pairs of (document_id, rrf_score), sorted DESC.
 */
std::vector<std::pair<std::string, double>> fuseRRF(
    const std::vector<std::vector<std::pair<std::string, double>>>& ranked_lists,
    const std::vector<double>&                                       weights,
    double                                                           rrf_k,
    size_t                                                           max_results)
{
    std::unordered_map<std::string, double> scores;
    std::unordered_map<std::string, std::string> best_modality;

    for (size_t li = 0; li < ranked_lists.size(); ++li) {
        const auto& list   = ranked_lists[li];
        const double w     = (li < weights.size()) ? weights[li] : 1.0;

        for (size_t rank = 0; rank < list.size(); ++rank) {
            const auto& [doc_id, raw_score] = list[rank];
            scores[doc_id] += w / (rrf_k + static_cast<double>(rank + 1));
        }
    }

    std::vector<std::pair<std::string, double>> result(scores.begin(), scores.end());
    std::stable_sort(result.begin(), result.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });

    if (result.size() > max_results) {
        result.resize(max_results);
    }
    return result;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// MultiModalRAG::Impl
// ─────────────────────────────────────────────────────────────────────────────

struct MultiModalRAG::Impl {
    MultiModalRAGConfig config;
    TextRetrievalFn     text_retriever;
    ImageRetrievalFn    image_retriever;
    ImageCaptionFn      image_captioner;
    mutable std::mutex state_mutex;
};

// ─────────────────────────────────────────────────────────────────────────────
// MultiModalRAG
// ─────────────────────────────────────────────────────────────────────────────

MultiModalRAG::MultiModalRAG()
    : impl_(std::make_unique<Impl>())
{
    THEMIS_DEBUG("MultiModalRAG created with default config");
}

MultiModalRAG::MultiModalRAG(const MultiModalRAGConfig& config)
    : impl_(std::make_unique<Impl>())
{
    impl_->config = config;
    THEMIS_DEBUG("MultiModalRAG created: image={}, table_qa={}, top_k={}",
                 config.enable_image_retrieval,
                 config.enable_table_qa,
                 config.top_k);
}

MultiModalRAG::~MultiModalRAG() = default;

// ── Retrieval backend configuration ──────────────────────────────────────────

void MultiModalRAG::setTextRetriever(TextRetrievalFn fn) {
    impl_->text_retriever = std::move(fn);
}

void MultiModalRAG::setImageRetriever(ImageRetrievalFn fn) {
    impl_->image_retriever = std::move(fn);
}

void MultiModalRAG::setImageCaptioner(ImageCaptionFn fn) {
    impl_->image_captioner = std::move(fn);
}

// ── Configuration ─────────────────────────────────────────────────────────────

MultiModalRAGConfig MultiModalRAG::getConfig() const {
    return impl_->config;
}

void MultiModalRAG::setConfig(const MultiModalRAGConfig& config) {
    impl_->config = config;
}

// ── query() ──────────────────────────────────────────────────────────────────

MultiModalRAGResult MultiModalRAG::query(const MultiModalQuery& mq) const {
    const auto t_start = std::chrono::steady_clock::now();

    const MultiModalRAGConfig& cfg = impl_->config;
    const size_t top_k = (mq.top_k > 0) ? mq.top_k : cfg.top_k;

    THEMIS_INFO("MultiModalRAG::query text='{}', modalities={}, top_k={}",
                mq.text, mq.modalities.size(), top_k);

    // Determine which modalities are active for this query.
    bool want_text  = false;
    bool want_image = false;

    for (const Modality m : mq.modalities) {
        if (m == Modality::TEXT)  want_text  = true;
        if (m == Modality::IMAGE) want_image = true;
    }

    // ── Step 1: Text retrieval ────────────────────────────────────────────────

    // (document_id → text document) for fast lookup when building sources
    std::unordered_map<std::string, judge::RetrievedDocument> text_doc_map;
    std::vector<std::pair<std::string, double>> text_ranked;

    if (want_text && impl_->text_retriever && !mq.text.empty()) {
        auto text_docs = impl_->text_retriever(mq.text, top_k);

        THEMIS_DEBUG("MultiModalRAG text retrieval: {} docs", text_docs.size());

        for (size_t i = 0; i < text_docs.size(); ++i) {
            const auto& doc = text_docs[i];
            text_ranked.emplace_back(doc.id, doc.similarity_score);
            text_doc_map[doc.id] = doc;
        }
    }

    // ── Step 2: Image retrieval ───────────────────────────────────────────────

    std::unordered_map<std::string, ImageDocument> image_doc_map;
    std::vector<std::pair<std::string, double>>    image_ranked;

    if (want_image && cfg.enable_image_retrieval &&
        impl_->image_retriever && !mq.image_embedding.empty())
    {
        auto image_docs = impl_->image_retriever(mq.image_embedding, top_k);

        THEMIS_DEBUG("MultiModalRAG image retrieval: {} docs", image_docs.size());

        for (const auto& img : image_docs) {
            image_ranked.emplace_back(img.id, img.relevance_score);
            image_doc_map[img.id] = img;
        }
    }

    // ── Step 3: RRF fusion ────────────────────────────────────────────────────

    std::vector<std::vector<std::pair<std::string, double>>> all_lists;
    std::vector<double> weights;

    if (!text_ranked.empty()) {
        all_lists.push_back(text_ranked);
        weights.push_back(cfg.text_weight);
    }
    if (!image_ranked.empty()) {
        all_lists.push_back(image_ranked);
        weights.push_back(cfg.image_weight);
    }

    std::vector<std::pair<std::string, double>> fused;

    if (all_lists.empty()) {
        THEMIS_WARN("MultiModalRAG::query: no retrieval results (no backends set or empty query)");
    } else if (all_lists.size() == 1) {
        // Single-modality: no fusion needed, use raw ranked list directly.
        fused = all_lists[0];
        if (fused.size() > cfg.max_sources) {
            fused.resize(cfg.max_sources);
        }
    } else {
        fused = fuseRRF(all_lists, weights, cfg.rrf_k, cfg.max_sources);
    }

    // ── Step 4: Build MultiModalSource list ──────────────────────────────────

    MultiModalRAGResult result;
    result.sources.reserve(fused.size());

    // Track used IDs to avoid duplicates when both modalities produce same doc ID.
    std::unordered_set<std::string> used_ids;

    for (const auto& [doc_id, rrf_score] : fused) {
        if (used_ids.count(doc_id)) continue;
        used_ids.insert(doc_id);

        // Prefer image source if available (image modality was requested and
        // this ID exists in the image map).
        auto img_it = image_doc_map.find(doc_id);
        if (img_it != image_doc_map.end()) {
            MultiModalSource src;
            src.modality        = Modality::IMAGE;
            src.document_id     = doc_id;
            src.relevance_score = rrf_score;
            src.image_path      = img_it->second.image_path;
            src.metadata        = img_it->second.metadata;

            // Use pre-computed caption or generate one via captioner.
            if (!img_it->second.caption.empty()) {
                src.caption = img_it->second.caption;
            } else {
                {
                    std::lock_guard<std::mutex> lock(impl_->state_mutex);
                    if (impl_->image_captioner) {
                        src.caption = impl_->image_captioner(img_it->second);
                    }
                }
                if (!src.caption.empty()) {
                    THEMIS_DEBUG("MultiModalRAG: generated caption for '{}': '{}'",
                                 doc_id, src.caption);
                }
            }

            result.sources.push_back(std::move(src));
            continue;
        }

        // Otherwise use text source.
        auto txt_it = text_doc_map.find(doc_id);
        if (txt_it != text_doc_map.end()) {
            MultiModalSource src;
            src.modality        = Modality::TEXT;
            src.document_id     = doc_id;
            src.relevance_score = rrf_score;
            src.content         = txt_it->second.content;
            src.metadata        = txt_it->second.metadata;
            result.sources.push_back(std::move(src));
        }
    }

    // ── Step 5: Build LLM context ─────────────────────────────────────────────

    result.context = buildContext(result.sources, mq.text);

    // ── Finalise ──────────────────────────────────────────────────────────────

    const auto t_end = std::chrono::steady_clock::now();
    result.elapsed_ms =
        std::chrono::duration<double, std::milli>(t_end - t_start).count();

    THEMIS_INFO("MultiModalRAG::query complete: sources={}, elapsed={:.2f}ms",
                result.sources.size(), result.elapsed_ms);

    return result;
}

// ── buildContext() ────────────────────────────────────────────────────────────

std::string MultiModalRAG::buildContext(
    const std::vector<MultiModalSource>& sources,
    const std::string&                   question) const
{
    std::ostringstream oss;

    // Partition sources by modality.
    std::vector<const MultiModalSource*> text_sources;
    std::vector<const MultiModalSource*> image_sources;
    std::vector<const MultiModalSource*> table_sources;

    for (const auto& src : sources) {
        if      (src.modality == Modality::TEXT)  text_sources.push_back(&src);
        else if (src.modality == Modality::IMAGE) image_sources.push_back(&src);
        else if (src.modality == Modality::TABLE) table_sources.push_back(&src);
    }

    // Text passages
    if (!text_sources.empty()) {
        oss << "Text passages:\n";
        for (size_t i = 0; i < text_sources.size(); ++i) {
            oss << "[" << (i + 1) << "] (score="
                << std::fixed << std::setprecision(2)
                << text_sources[i]->relevance_score << ") "
                << text_sources[i]->content << "\n";
        }
        oss << "\n";
    }

    // Table data
    if (!table_sources.empty()) {
        oss << "Tables:\n";
        for (size_t i = 0; i < table_sources.size(); ++i) {
            oss << "[" << (i + 1) << "] (score="
                << std::fixed << std::setprecision(2)
                << table_sources[i]->relevance_score << ")\n"
                << table_sources[i]->table_data << "\n";
        }
        oss << "\n";
    }

    // Image captions
    if (!image_sources.empty()) {
        oss << "Image captions:\n";
        for (size_t i = 0; i < image_sources.size(); ++i) {
            oss << "[" << (i + 1) << "] (score="
                << std::fixed << std::setprecision(2)
                << image_sources[i]->relevance_score << ") ";
            if (!image_sources[i]->caption.empty()) {
                oss << image_sources[i]->caption;
            } else {
                oss << "[image: " << image_sources[i]->image_path << "]";
            }
            oss << "\n";
        }
        oss << "\n";
    }

    // Question / answer prompt
    if (!question.empty()) {
        oss << "Question: " << question << "\n";
        oss << "Answer:";
    }

    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// MultiModalRAGFactory
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<MultiModalRAG> MultiModalRAGFactory::createTextOnly() {
    MultiModalRAGConfig cfg;
    cfg.enable_image_retrieval = false;
    cfg.enable_table_qa        = false;
    cfg.enable_ocr             = false;
    cfg.text_weight            = 1.0;
    cfg.image_weight           = 0.0;
    return std::make_unique<MultiModalRAG>(cfg);
}

std::unique_ptr<MultiModalRAG> MultiModalRAGFactory::createTextAndImage() {
    MultiModalRAGConfig cfg;
    cfg.enable_image_retrieval = true;
    cfg.enable_table_qa        = false;
    cfg.text_weight            = 0.5;
    cfg.image_weight           = 1.0;
    return std::make_unique<MultiModalRAG>(cfg);
}

std::unique_ptr<MultiModalRAG> MultiModalRAGFactory::createFull() {
    MultiModalRAGConfig cfg;
    cfg.enable_image_retrieval = true;
    cfg.enable_table_qa        = true;
    cfg.enable_ocr             = true;
    cfg.text_weight            = 0.5;
    cfg.image_weight           = 1.0;
    return std::make_unique<MultiModalRAG>(cfg);
}

} // namespace themis::rag::multimodal
