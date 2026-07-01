/**
 * @file rotate_completion.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "graph/rotate_completion.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <numbers>
#include <numeric>
#include <random>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace graph {

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
}

// ============================================================================
// RotatEModel::Impl — private implementation
// ============================================================================

class RotatEModel::Impl {
public:
    explicit Impl(RotatEConfig cfg) : cfg_(std::move(cfg)) {}

    // ──────────────────────────────────────────────────────────────────
    // Registry
    // ──────────────────────────────────────────────────────────────────

    size_t addEntity(const std::string& id) {
        std::unique_lock lk(mu_);
        auto [it, inserted] = entity_index_.emplace(id, entity_names_.size());
        if (inserted) {
            entity_names_.push_back(id);
        }
        return it->second;
    }

    size_t addRelation(const std::string& id) {
        std::unique_lock lk(mu_);
        auto [it, inserted] = relation_index_.emplace(id, relation_names_.size());
        if (inserted) {
            relation_names_.push_back(id);
        }
        return it->second;
    }

    size_t entityCount() const {
        std::shared_lock lk(mu_);
        return entity_names_.size();
    }

    size_t relationCount() const {
        std::shared_lock lk(mu_);
        return relation_names_.size();
    }

    // ──────────────────────────────────────────────────────────────────
    // Index helpers (lock must be held by caller)
    // ──────────────────────────────────────────────────────────────────

    size_t entityIdx(const std::string& id) const {
        auto it = entity_index_.find(id);
        if (it == entity_index_.end())
            throw std::out_of_range("RotatEModel: unknown entity '" + id + "'");
        return it->second;
    }

    size_t relationIdx(const std::string& id) const {
        auto it = relation_index_.find(id);
        if (it == relation_index_.end())
            throw std::out_of_range("RotatEModel: unknown relation '" + id + "'");
        return it->second;
    }

    // ──────────────────────────────────────────────────────────────────
    // Embedding access
    // ──────────────────────────────────────────────────────────────────

    std::vector<float> entityEmbedding(const std::string& id) const {
        std::shared_lock lk(mu_);
        if (!trained_) return {};
        
        size_t idx = entityIdx(id);  // May throw if id not registered
        size_t d   = cfg_.embedding_dim;
        
        // Pre-allocate full output vector and populate it
        // real + imag interleaved: [re_0, im_0, re_1, im_1, ...]
        std::vector<float> out(2 * d);
        
        for (size_t k = 0; k < d; ++k) {
            // Each complex number component gets two float slots: real and imaginary
            out[2 * k]     = entity_re_[idx * d + k];      // real component
            out[2 * k + 1] = entity_im_[idx * d + k];      // imaginary component
        }
        
        return out;  // Move semantics: ownership transferred to caller
    }

    std::vector<float> relationPhase(const std::string& id) const {
        std::shared_lock lk(mu_);
        if (!trained_) return {};
        size_t idx = relationIdx(id);
        size_t d   = cfg_.embedding_dim;
        // Use vector iterator-range constructor to properly copy the range [idx*d, (idx+1)*d)
        // (not initializer list which would create a vector containing two iterator objects)
        return std::vector<float>(relation_phase_.begin() + idx * d,
                                  relation_phase_.begin() + (idx + 1) * d);
    }

    bool isTrained() const {
        std::shared_lock lk(mu_);
        return trained_;
    }

    // ──────────────────────────────────────────────────────────────────
    // Scoring — L1 RotatE distance (no lock: caller locks or scores are immutable)
    // ──────────────────────────────────────────────────────────────────

    double scoreImpl(size_t h_idx, size_t r_idx, size_t t_idx) const {
        size_t d   = cfg_.embedding_dim;
        double sum = 0.0;
        for (size_t k = 0; k < d; ++k) {
            float h_re = entity_re_[h_idx * d + k];
            float h_im = entity_im_[h_idx * d + k];
            float phi  = relation_phase_[r_idx * d + k];
            float t_re = entity_re_[t_idx * d + k];
            float t_im = entity_im_[t_idx * d + k];

            // h ∘ r (complex mul with unit-modulus r = e^{iφ})
            float hr_re = h_re * std::cos(phi) - h_im * std::sin(phi);
            float hr_im = h_re * std::sin(phi) + h_im * std::cos(phi);

            // ‖h ∘ r − t‖₁
            sum += std::abs(hr_re - t_re) + std::abs(hr_im - t_im);
        }
        return sum;
    }

    double score(const std::string& h, const std::string& r,
                 const std::string& t) const
    {
        std::shared_lock lk(mu_);
        if (!trained_)
            throw std::runtime_error("RotatEModel: model not trained yet");
        return scoreImpl(entityIdx(h), relationIdx(r), entityIdx(t));
    }

    // ──────────────────────────────────────────────────────────────────
    // Training — Stub RTE-S01
    // ──────────────────────────────────────────────────────────────────

    RotatETrainResult train(const std::vector<KGTriple>& triples) {
        std::unique_lock lk(mu_);

        const size_t n_ent  = entity_names_.size();
        const size_t n_rel  = relation_names_.size();
        const size_t d      = cfg_.embedding_dim;

        if (n_ent == 0 || n_rel == 0 || triples.empty())
            return {};

        // Validate triples.
        for (const auto& t : triples) {
            entityIdx(t.head);
            relationIdx(t.relation);
            entityIdx(t.tail);
        }

        // Initialise / resize embedding tables.
        const size_t ent_params = n_ent * d;
        const size_t rel_params = n_rel * d;
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> init_dist(-0.5f / d, 0.5f / d);
        std::uniform_real_distribution<float> phase_dist(
            -kPi, kPi);

        auto reinit = [&](std::vector<float>& v, size_t sz, auto& dist) {
            v.resize(sz);
            for (auto& x : v) x = dist(rng);
        };

        reinit(entity_re_,      ent_params, init_dist);
        reinit(entity_im_,      ent_params, init_dist);
        reinit(relation_phase_, rel_params, phase_dist);

        // Normalise entity embeddings (modulus ≈ 1).
        for (size_t i = 0; i < n_ent; ++i) {
            float norm2 = 0.0f;
            for (size_t k = 0; k < d; ++k) {
                float re = entity_re_[i * d + k];
                float im = entity_im_[i * d + k];
                norm2 += re * re + im * im;
            }
            float inv = (norm2 > 0.f) ? (1.0f / std::sqrt(norm2 / d)) : 1.0f;
            for (size_t k = 0; k < d; ++k) {
                entity_re_[i * d + k] *= inv;
                entity_im_[i * d + k] *= inv;
            }
        }

        const float lr     = cfg_.learning_rate;
        const float margin = cfg_.margin;
        const size_t neg_k = cfg_.neg_samples;
        std::uniform_int_distribution<size_t> ent_dist(0, n_ent - 1);

        double final_loss = 0.0;
        std::vector<size_t> order(triples.size());
        std::iota(order.begin(), order.end(), 0);

        for (size_t ep = 0; ep < cfg_.epochs; ++ep) {
            std::shuffle(order.begin(), order.end(), rng);
            double ep_loss = 0.0;

            for (size_t ti : order) {
                const auto& tr = triples[ti];
                size_t h_idx = entityIdx(tr.head);
                size_t r_idx = relationIdx(tr.relation);
                size_t t_idx = entityIdx(tr.tail);

                double pos_score = scoreImpl(h_idx, r_idx, t_idx);

                for (size_t ns = 0; ns < neg_k; ++ns) {
                    size_t neg_ent = ent_dist(rng);
                    bool corrupt_tail = (ns % 2 == 0);
                    size_t h_neg = corrupt_tail ? h_idx : neg_ent;
                    size_t t_neg = corrupt_tail ? neg_ent : t_idx;

                    double neg_score = scoreImpl(h_neg, r_idx, t_neg);

                    // Margin loss: max(0, pos - neg + γ)
                    double loss_val = pos_score - neg_score + margin;
                    if (loss_val <= 0.0) continue;

                    ep_loss += loss_val;

                    // Gradient update: push pos_score down, neg_score up.
                    // Approximate SGD using sub-gradients of L1 distance.
                    float step = lr * static_cast<float>(
                        std::min(1.0, std::max(-1.0, (pos_score - neg_score + margin) / margin)));

                    auto apply_distance_update = [&](size_t h_u, size_t r_u, size_t t_u,
                                                     float direction) {
                        // direction = -1 => minimise distance, +1 => maximise distance
                        for (size_t k = 0; k < d; ++k) {
                            const size_t h_off = h_u * d + k;
                            const size_t t_off = t_u * d + k;
                            const size_t r_off = r_u * d + k;

                            float h_re = entity_re_[h_off];
                            float h_im = entity_im_[h_off];
                            float t_re = entity_re_[t_off];
                            float t_im = entity_im_[t_off];
                            float phi  = relation_phase_[r_off];

                            float c = std::cos(phi);
                            float s = std::sin(phi);

                            float hr_re = h_re * c - h_im * s;
                            float hr_im = h_re * s + h_im * c;

                            float diff_re = hr_re - t_re;
                            float diff_im = hr_im - t_im;
                            float sign_re = (diff_re >= 0.0f) ? 1.0f : -1.0f;
                            float sign_im = (diff_im >= 0.0f) ? 1.0f : -1.0f;

                            float grad_h_re = sign_re * c + sign_im * s;
                            float grad_h_im = -sign_re * s + sign_im * c;
                            float grad_t_re = -sign_re;
                            float grad_t_im = -sign_im;

                            float dhr_re_dphi = -h_re * s - h_im * c;
                            float dhr_im_dphi =  h_re * c - h_im * s;
                            float grad_phi = sign_re * dhr_re_dphi + sign_im * dhr_im_dphi;

                            entity_re_[h_off] += direction * step * grad_h_re;
                            entity_im_[h_off] += direction * step * grad_h_im;
                            entity_re_[t_off] += direction * step * grad_t_re;
                            entity_im_[t_off] += direction * step * grad_t_im;
                            relation_phase_[r_off] += direction * step * grad_phi;

                            if (relation_phase_[r_off] > kPi) {
                                relation_phase_[r_off] -= 2.0f * kPi;
                            } else if (relation_phase_[r_off] < -kPi) {
                                relation_phase_[r_off] += 2.0f * kPi;
                            }
                        }

                        auto renormalize_entity = [&](size_t e_idx) {
                            float norm2 = 0.0f;
                            for (size_t kk = 0; kk < d; ++kk) {
                                float re = entity_re_[e_idx * d + kk];
                                float im = entity_im_[e_idx * d + kk];
                                norm2 += re * re + im * im;
                            }
                            if (norm2 <= 0.0f) return;
                            float inv = 1.0f / std::sqrt(norm2 / static_cast<float>(d));
                            for (size_t kk = 0; kk < d; ++kk) {
                                entity_re_[e_idx * d + kk] *= inv;
                                entity_im_[e_idx * d + kk] *= inv;
                            }
                        };
                        renormalize_entity(h_u);
                        renormalize_entity(t_u);
                    };

                    apply_distance_update(h_idx, r_idx, t_idx, -1.0f);
                    apply_distance_update(h_neg, r_idx, t_neg, +1.0f);
                }
            }

            final_loss = ep_loss / static_cast<double>(triples.size());
        }

        trained_ = true;

        RotatETrainResult res;
        res.success    = true;
        res.final_loss = final_loss;
        res.epochs_run = cfg_.epochs;
        res.entities   = n_ent;
        res.relations  = n_rel;
        res.triples    = triples.size();
        return res;
    }

    // ──────────────────────────────────────────────────────────────────
    // Link prediction helpers (called by LinkPredictionHead)
    // ──────────────────────────────────────────────────────────────────

    std::vector<LinkPrediction> rankAll(size_t h_idx, size_t r_idx,
                                         bool predict_tail, size_t top_k) const
    {
        // Caller must hold at least a shared lock on mu_.
        // Note: Results are independent vectors; safe for concurrent reads and external caching.
        if (!trained_)
            throw std::runtime_error("RotatEModel: model not trained yet");

        const size_t n = entity_names_.size();
        
        // Score all entities; pre-allocate to avoid reallocation overhead
        std::vector<std::pair<double, size_t>> scored;
        scored.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            double s = predict_tail
                ? scoreImpl(h_idx, r_idx, i)
                : scoreImpl(i, r_idx, h_idx);
            scored.emplace_back(s, i);
        }

        // Sort by ascending score (lower distance = higher confidence)
        std::sort(scored.begin(), scored.end());

        // Extract top-k predictions with ranks
        const size_t k = std::min(top_k, n);
        std::vector<LinkPrediction> out;
        out.reserve(k);
        
        for (size_t i = 0; i < k; ++i) {
            // Access entity_names_ by index; safe because it's not modified during ranking
            out.push_back({entity_names_[scored[i].second],
                           scored[i].first,
                           static_cast<double>(i + 1)});
        }
        
        return out;  // Move semantics; ownership transferred to caller
    }

    // Expose entity name for a given index (for injection into KGReasoner).
    std::string entityName(size_t idx) const {
        std::shared_lock lk(mu_);
        return entity_names_.at(idx);
    }

    // Public rank-all: acquires lock then calls the internal helper.
    std::vector<LinkPrediction> rankTailPublic(const std::string& head,
                                                const std::string& relation,
                                                size_t             top_k) const
    {
        std::shared_lock lk(mu_);
        size_t h_idx = entityIdx(head);
        size_t r_idx = relationIdx(relation);
        return rankAll(h_idx, r_idx, /*predict_tail=*/true, top_k);
    }

    std::vector<LinkPrediction> rankHeadPublic(const std::string& relation,
                                                const std::string& tail,
                                                size_t             top_k) const
    {
        std::shared_lock lk(mu_);
        size_t t_idx = entityIdx(tail);
        size_t r_idx = relationIdx(relation);
        return rankAll(t_idx, r_idx, /*predict_tail=*/false, top_k);
    }

private:
    mutable std::shared_mutex mu_;
    RotatEConfig              cfg_;

    std::unordered_map<std::string, size_t> entity_index_;
    std::unordered_map<std::string, size_t> relation_index_;
    std::vector<std::string>                entity_names_;
    std::vector<std::string>                relation_names_;

    // Embedding tables (Stub RTE-S02: CPU float32 vectors)
    std::vector<float> entity_re_;       ///< Real parts  (n_ent × d)
    std::vector<float> entity_im_;       ///< Imag parts  (n_ent × d)
    std::vector<float> relation_phase_;  ///< Phases φ    (n_rel × d)

    bool trained_ = false;
};

// ============================================================================
// RotatEModel — public delegation
// ============================================================================

RotatEModel::RotatEModel(RotatEConfig cfg)
    : impl_(std::make_unique<Impl>(std::move(cfg))) {}

RotatEModel::~RotatEModel() = default;

size_t RotatEModel::addEntity(const std::string& id)   { return impl_->addEntity(id); }
size_t RotatEModel::addRelation(const std::string& id) { return impl_->addRelation(id); }
size_t RotatEModel::entityCount()   const { return impl_->entityCount(); }
size_t RotatEModel::relationCount() const { return impl_->relationCount(); }
bool   RotatEModel::isTrained()     const { return impl_->isTrained(); }

RotatETrainResult RotatEModel::train(const std::vector<KGTriple>& triples) {
    return impl_->train(triples);
}

double RotatEModel::score(const std::string& h,
                           const std::string& r,
                           const std::string& t) const {
    return impl_->score(h, r, t);
}

std::vector<float> RotatEModel::entityEmbedding(const std::string& id) const {
    return impl_->entityEmbedding(id);
}

std::vector<float> RotatEModel::relationPhase(const std::string& id) const {
    return impl_->relationPhase(id);
}

std::vector<LinkPrediction> RotatEModel::rankTail(const std::string& head,
                                                    const std::string& relation,
                                                    size_t             top_k) const
{
    return impl_->rankTailPublic(head, relation, top_k);
}

std::vector<LinkPrediction> RotatEModel::rankHead(const std::string& relation,
                                                    const std::string& tail,
                                                    size_t             top_k) const
{
    return impl_->rankHeadPublic(relation, tail, top_k);
}

// ============================================================================
// LinkPredictionHead
// ============================================================================

LinkPredictionHead::LinkPredictionHead(RotatEModel& model)
    : model_(model) {}

std::vector<LinkPrediction> LinkPredictionHead::predictTail(
        const std::string& head,
        const std::string& relation,
        size_t             top_k) const
{
    return model_.rankTail(head, relation, top_k);
}

std::vector<LinkPrediction> LinkPredictionHead::predictHead(
        const std::string& relation,
        const std::string& tail,
        size_t             top_k) const
{
    return model_.rankHead(relation, tail, top_k);
}

// ============================================================================
// KGCompletionEngine
// ============================================================================

KGCompletionEngine::KGCompletionEngine(RotatEConfig cfg)
    : cfg_(cfg), model_(cfg), link_head_(model_) {}

void KGCompletionEngine::setReasoner(KnowledgeGraphReasoner* reasoner,
                                      double                  inject_threshold)
{
    reasoner_         = reasoner;
    inject_threshold_ = inject_threshold;
}

size_t KGCompletionEngine::addEntity(const std::string& id)   { return model_.addEntity(id); }
size_t KGCompletionEngine::addRelation(const std::string& id) { return model_.addRelation(id); }

RotatETrainResult KGCompletionEngine::train(const std::vector<KGTriple>& triples) {
    return model_.train(triples);
}

std::vector<LinkPrediction> KGCompletionEngine::completeTail(
        const std::string& head,
        const std::string& relation,
        size_t             top_k)
{
    auto preds = link_head_.predictTail(head, relation, top_k);

    if (reasoner_) {
        for (const auto& p : preds) {
            if (p.score < inject_threshold_) {
                reasoner_->addFact({head, relation, p.entity});
            }
        }
    }

    return preds;
}

std::vector<LinkPrediction> KGCompletionEngine::completeHead(
        const std::string& relation,
        const std::string& tail,
        size_t             top_k)
{
    return link_head_.predictHead(relation, tail, top_k);
}

} // namespace graph
} // namespace themis
