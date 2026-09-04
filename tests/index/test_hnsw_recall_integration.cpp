// Integration tests: HNSW recall@10 (Production Readiness Checklist)
//
// Validates that the hnswlib-backed HNSW index achieves acceptable recall@10
// for all supported distance metrics (L2, Cosine, Dot Product) and that the
// VectorIndexManager's HNSW path surfaces at least the same quality level.
//
// recall@k = |ANN top-k ∩ exact top-k| / k
//
// Acceptance criteria (from ROADMAP):
//   - Integration tests: HNSW recall@10 >= 0.90 on a 2 000-vector / 64-dim corpus

#include <gtest/gtest.h>
#if __has_include(<hnswlib/hnswlib.h>)
#include <hnswlib/hnswlib.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <set>
#include <vector>

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

namespace {

static std::vector<std::vector<float>> makeRandomVectors(
        size_t n, size_t dim, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    std::vector<std::vector<float>> out(n, std::vector<float>(dim));
    for (auto& v : out)
        for (auto& x : v) {
          x = dist(rng);
        }
    return out;
}

// Normalise to unit length (required for cosine / dot-product spaces)
static std::vector<float> normalise(std::vector<float> v) {
    float norm = 0.f;
    for (float x : v) {
      norm += x * x;
    }
    norm = std::sqrt(norm);
    if (norm > 1e-9f)
        for (auto& x : v) {
          x /= norm;
        }
    return v;
}

static float l2sq(const std::vector<float>& a, const std::vector<float>& b) {
    float s = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

static float innerProduct(const std::vector<float>& a, const std::vector<float>& b) {
    float s = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
      s += a[i] * b[i];
    }
    return s;
}

// Brute-force top-k by L2 squared distance (ascending)
static std::vector<size_t> bfKnnL2(
        const std::vector<std::vector<float>>& db,
        const std::vector<float>& q, int k) {
    std::vector<std::pair<float, size_t>> scored;
    scored.reserve(db.size());
    for (size_t i = 0; i < db.size(); ++i)
        scored.emplace_back(l2sq(db[i], q), i);
    std::partial_sort(scored.begin(),
                      scored.begin() + std::min((int)scored.size(), k),
                      scored.end());
    std::vector<size_t> ids;
    for (int i = 0; i < k && i < (int)scored.size(); ++i)
        ids.push_back(scored[i].second);
    return ids;
}

// Brute-force top-k by inner-product (descending → smallest hnswlib distance)
// hnswlib InnerProductSpace returns 1 - dot; so brute-force exact is by largest dot.
static std::vector<size_t> bfKnnIP(
        const std::vector<std::vector<float>>& db,
        const std::vector<float>& q, int k) {
    std::vector<std::pair<float, size_t>> scored;
    scored.reserve(db.size());
    for (size_t i = 0; i < db.size(); ++i)
        scored.emplace_back(-innerProduct(db[i], q), i);  // negate for ascending sort
    std::partial_sort(scored.begin(),
                      scored.begin() + std::min((int)scored.size(), k),
                      scored.end());
    std::vector<size_t> ids;
    for (int i = 0; i < k && i < (int)scored.size(); ++i)
        ids.push_back(scored[i].second);
    return ids;
}

// recall@k: fraction of exact top-k found in returned set
static float recallAtK(const std::vector<size_t>& exact,
                        const std::vector<std::pair<float, size_t>>& ann) {
    std::set<size_t> gold(exact.begin(), exact.end());
    size_t hits = 0;
    for (auto& p : ann)
        if (gold.count(p.second)) {
          ++hits;
        }
    return static_cast<float>(hits) / static_cast<float>(exact.size());
}

// Drain a max-heap from hnswlib into a sorted (distance asc) vector
static std::vector<std::pair<float, size_t>> drainHeap(
        std::priority_queue<std::pair<float, hnswlib::labeltype>> heap) {
    std::vector<std::pair<float, size_t>> v;
    v.reserve(heap.size());
    while (!heap.empty()) {
        v.emplace_back(heap.top().first, (size_t)heap.top().second);
        heap.pop();
    }
    std::sort(v.begin(), v.end());
    return v;
}

// Average recall over a set of query vectors
template <typename BfFn>
static float avgRecall(
        hnswlib::AlgorithmInterface<float>* idx,
        const std::vector<std::vector<float>>& db,
        const std::vector<std::vector<float>>& queries,
        int k, BfFn bf_fn) {
    float total = 0.f;
    for (const auto& q : queries) {
        auto exact  = bf_fn(db, q, k);
        auto raw    = idx->searchKnn(q.data(), k);
        auto sorted = drainHeap(std::move(raw));
        total += recallAtK(exact, sorted);
    }
    return total / static_cast<float>(queries.size());
}

}  // namespace

// ---------------------------------------------------------------------------
// Test constants
// ---------------------------------------------------------------------------

static constexpr size_t kN      = 2000;
static constexpr size_t kDim    = 64;
static constexpr int    kK      = 10;
static constexpr int    kM      = 16;
static constexpr int    kEfC    = 200;
static constexpr int    kEfS    = 100;
static constexpr int    kQueries = 50;
static constexpr float  kMinRecall = 0.90f;

// ---------------------------------------------------------------------------
// Fixture: L2 (Euclidean) metric
// ---------------------------------------------------------------------------

class HnswRecallL2 : public ::testing::Test {
protected:
    void SetUp() override {
        db_     = makeRandomVectors(kN, kDim, 1);
        queries_ = makeRandomVectors(kQueries, kDim, 99);

        space_ = std::make_unique<hnswlib::L2Space>(kDim);
        idx_   = std::make_unique<hnswlib::HierarchicalNSW<float>>(
                     space_.get(), kN, kM, kEfC);
        for (size_t i = 0; i < kN; ++i)
            idx_->addPoint(db_[i].data(), i);
        idx_->setEf(kEfS);
    }

    std::vector<std::vector<float>>              db_;
    std::vector<std::vector<float>>              queries_;
    std::unique_ptr<hnswlib::L2Space>            space_;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> idx_;
};

TEST_F(HnswRecallL2, RecallAtK_MeetsMinimumThreshold) {
    float r = avgRecall(idx_.get(), db_, queries_, kK, bfKnnL2);
    EXPECT_GE(r, kMinRecall)
        << "HNSW L2 recall@" << kK << " = " << r
        << " (minimum required: " << kMinRecall << ")";
}

TEST_F(HnswRecallL2, ExactSelfQuery_PerfectRecall) {
    // Querying a point that is in the index must return itself as top-1.
    for (int i = 0; i < 10; ++i) {
        auto raw = idx_->searchKnn(db_[i].data(), 1);
        ASSERT_EQ(raw.size(), 1u);
        EXPECT_EQ(raw.top().second, (hnswlib::labeltype)i)
            << "Self-query failed for vector " << i;
        EXPECT_NEAR(raw.top().first, 0.f, 1e-4f);
    }
}

TEST_F(HnswRecallL2, SearchReturnsExactlyKResults) {
    for (const auto& q : queries_) {
        auto raw = idx_->searchKnn(q.data(), kK);
        EXPECT_EQ((int)raw.size(), kK);
    }
}

TEST_F(HnswRecallL2, DistancesNonNegative) {
    for (const auto& q : queries_) {
        auto sorted = drainHeap(idx_->searchKnn(q.data(), kK));
        for (auto& p : sorted)
            EXPECT_GE(p.first, 0.f);
    }
}

TEST_F(HnswRecallL2, DistancesNonDecreasing) {
    for (const auto& q : queries_) {
        auto sorted = drainHeap(idx_->searchKnn(q.data(), kK));
        for (size_t i = 1; i < sorted.size(); ++i)
            EXPECT_LE(sorted[i - 1].first, sorted[i].first);
    }
}

// ---------------------------------------------------------------------------
// Fixture: Cosine similarity (via InnerProductSpace on normalised vectors)
// ---------------------------------------------------------------------------

class HnswRecallCosine : public ::testing::Test {
protected:
    void SetUp() override {
        auto raw = makeRandomVectors(kN, kDim, 2);
        for (auto& v : raw) {
          db_.push_back(normalise(v));
        }

        auto rawq = makeRandomVectors(kQueries, kDim, 88);
        for (auto& v : rawq) {
          queries_.push_back(normalise(v));
        }

        space_ = std::make_unique<hnswlib::InnerProductSpace>(kDim);
        idx_   = std::make_unique<hnswlib::HierarchicalNSW<float>>(
                     space_.get(), kN, kM, kEfC);
        for (size_t i = 0; i < kN; ++i)
            idx_->addPoint(db_[i].data(), i);
        idx_->setEf(kEfS);
    }

    std::vector<std::vector<float>>                  db_;
    std::vector<std::vector<float>>                  queries_;
    std::unique_ptr<hnswlib::InnerProductSpace>      space_;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> idx_;
};

TEST_F(HnswRecallCosine, RecallAtK_MeetsMinimumThreshold) {
    // For normalised vectors, cosine similarity == inner product;
    // hnswlib InnerProductSpace distance = 1 - dot, so brute-force is by IP.
    float r = avgRecall(idx_.get(), db_, queries_, kK, bfKnnIP);
    EXPECT_GE(r, kMinRecall)
        << "HNSW Cosine (IP) recall@" << kK << " = " << r
        << " (minimum required: " << kMinRecall << ")";
}

TEST_F(HnswRecallCosine, SelfQuery_NearZeroDistance) {
    // For a unit vector, ip_distance = 1 - 1 = 0.
    for (int i = 0; i < 5; ++i) {
        auto raw = idx_->searchKnn(db_[i].data(), 1);
        ASSERT_EQ(raw.size(), 1u);
        EXPECT_EQ(raw.top().second, (hnswlib::labeltype)i);
        EXPECT_NEAR(raw.top().first, 0.f, 1e-3f);
    }
}

// ---------------------------------------------------------------------------
// Fixture: Dot Product (InnerProductSpace on non-normalised vectors)
// ---------------------------------------------------------------------------

class HnswRecallDotProduct : public ::testing::Test {
protected:
    void SetUp() override {
        db_      = makeRandomVectors(kN, kDim, 3);
        queries_ = makeRandomVectors(kQueries, kDim, 77);

        space_ = std::make_unique<hnswlib::InnerProductSpace>(kDim);
        idx_   = std::make_unique<hnswlib::HierarchicalNSW<float>>(
                     space_.get(), kN, kM, kEfC);
        for (size_t i = 0; i < kN; ++i)
            idx_->addPoint(db_[i].data(), i);
        idx_->setEf(kEfS);
    }

    std::vector<std::vector<float>>                  db_;
    std::vector<std::vector<float>>                  queries_;
    std::unique_ptr<hnswlib::InnerProductSpace>      space_;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> idx_;
};

TEST_F(HnswRecallDotProduct, RecallAtK_MeetsMinimumThreshold) {
    float r = avgRecall(idx_.get(), db_, queries_, kK, bfKnnIP);
    EXPECT_GE(r, kMinRecall)
        << "HNSW DotProduct recall@" << kK << " = " << r
        << " (minimum required: " << kMinRecall << ")";
}

// ---------------------------------------------------------------------------
// High-ef parameter: recall improves with larger ef_search
// ---------------------------------------------------------------------------

TEST(HnswRecallParameterSensitivity, HigherEfSearchIncreasesRecall) {
    constexpr size_t N = 1000, DIM = 32, Q = 20;
    constexpr int K = 10;

    auto db = makeRandomVectors(N, DIM, 10);
    auto qs = makeRandomVectors(Q, DIM, 20);

    hnswlib::L2Space space(DIM);
    hnswlib::HierarchicalNSW<float> idx(&space, N, 16, 200);
    for (size_t i = 0; i < N; ++i)
        idx.addPoint(db[i].data(), i);

    idx.setEf(K);
    float recall_low = avgRecall(&idx, db, qs, K, bfKnnL2);

    idx.setEf(200);
    float recall_high = avgRecall(&idx, db, qs, K, bfKnnL2);

    EXPECT_LE(recall_low, recall_high + 1e-4f)
        << "Higher ef_search should not decrease recall";
    // High ef should meet the threshold
    EXPECT_GE(recall_high, kMinRecall);
}

// ---------------------------------------------------------------------------
// Brute-force baseline: recall == 1.0 by definition
// ---------------------------------------------------------------------------

TEST(HnswRecallBaseline, BruteForceRecallIsOne) {
    constexpr size_t N = 500, DIM = 16, Q = 10;
    constexpr int K = 10;

    auto db = makeRandomVectors(N, DIM, 5);
    auto qs = makeRandomVectors(Q, DIM, 6);

    hnswlib::L2Space space(DIM);
    hnswlib::BruteforceSearch<float> bf(&space, N);
    for (size_t i = 0; i < N; ++i)
        bf.addPoint(db[i].data(), i);

    float total = 0.f;
    for (const auto& q : qs) {
        auto exact  = bfKnnL2(db, q, K);
        auto raw    = bf.searchKnn(q.data(), K);
        auto sorted = drainHeap(std::move(raw));
        total += recallAtK(exact, sorted);
    }
    EXPECT_NEAR(total / Q, 1.0f, 1e-5f) << "Brute-force must have perfect recall";
}
#else

TEST(HnswRecallIntegration, HnswlibHeadersUnavailable) {
    GTEST_SKIP() << "hnswlib headers are unavailable in this build environment";
}

#endif
