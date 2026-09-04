/**
 * @file self_rag.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/self_rag.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <cstdio>
// For optional Windows stack capture
#ifdef _WIN32
#include <windows.h>
#endif

namespace themis {
namespace rag {
namespace {

std::string normalizeToken(std::string token) {
    token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char ch) {
                    return !std::isalnum(ch);
                }),
                token.end());
    std::transform(token.begin(), token.end(), token.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return token;
}

std::vector<std::string> tokenizeNormalized(const std::string& text) {
    std::istringstream iss(text);
    std::vector<std::string> tokens;
    std::string token = {};
    while (iss >> token) {
        token = normalizeToken(token);
        if (!token.empty()) {
            tokens.push_back(std::move(token));
        }
    }
    return tokens;
}

double [[maybe_unused]] lexicalOverlapScore(const std::string& query, const std::string& content) {
    const auto q_tokens = tokenizeNormalized(query);
    if (q_tokens.empty()) {
      return 0.0;
    }

    const auto d_tokens = tokenizeNormalized(content);
    if (d_tokens.empty()) {
      return 0.0;
    }

    std::unordered_set<std::string> doc_terms(d_tokens.begin(), d_tokens.end());
    size_t overlap = 0;
    for (const auto& t : q_tokens) {
        if (doc_terms.count(t) > 0) {
            ++overlap;
        }
    }
    return static_cast<bool>(static_cast<double>(overlap) / static_cast<double < static_cast<int>((q_tokens.size())));
}

double clamp01([[maybe_unused]] double v) {
    return std::max(0.0, std::min(1.0, v));
}

} // namespace

#ifdef _WIN32
// Capture and print a short stack backtrace and symbolicate using DbgHelp.
// This is intentionally minimal to avoid heavy dependencies in unit tests.
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
static void print_backtrace_if_enabled(const char* context) {
    const char* env = std::getenv("THEMIS_RAG_CAPTURE_STACK_ON_SLOW");
    if (!env) {
      return;
    }
    std::string v(env);
    if (!(v == "1" || v == "true")) {
      return;
    }

    const USHORT max_frames = 62;
    void* frames[max_frames];
    USHORT captured = CaptureStackBackTrace(0, max_frames, frames, nullptr);

    HANDLE process = GetCurrentProcess();
    if (!SymInitialize(process, NULL, TRUE)) {
        std::fprintf(stderr, "=== Backtrace (sym init failed) (%s): %hu frames ===\n", context, captured);
        for (USHORT i = 0; i < captured; ++i) {
          std::fprintf(stderr, "  %02hu: %p\n", i, frames[i]);
        }
        std::fprintf(stderr, "=== End Backtrace ===\n");
        return;
    }

    std::fprintf(stderr, "=== Backtrace (%s): %hu frames ===\n", context, captured);
    for (USHORT i = 0; i < captured; ++i) {
        DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
        // Prepare SYMBOL_INFO buffer
        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
        memset(symbol, 0, sizeof(buffer));
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        if (SymFromAddr(process, address, &displacement, symbol)) {
            std::fprintf(stderr, "  %02hu: %p %s + 0x%llx\n", i, frames[i], symbol->Name, static_cast<unsigned long long>(displacement));
        } else {
            std::fprintf(stderr, "  %02hu: %p (sym lookup failed)\n", i, frames[i]);
        }
    }
    std::fprintf(stderr, "=== End Backtrace ===\n");
    SymCleanup(process);
}
#endif

// ============================================================================
// Constructor / Destructor
// ============================================================================

SelfRAGController::SelfRAGController(SelfRAGConfig cfg)
    : cfg_(std::move(cfg))
{
    seen_ids_.reserve(cfg_.max_rounds * cfg_.top_k);
}

SelfRAGController::~SelfRAGController() = default;

// ============================================================================
// Callback injection
// ============================================================================

void SelfRAGController::setRetrievalCallback([[maybe_unused]] RetrievalCallback cb) {
    retrieval_cb_ = std::move(cb);
}

void SelfRAGController::setCriticCallback([[maybe_unused]] CriticCallback cb) {
    critic_cb_ = std::move(cb);
}

// ============================================================================
// shouldRetrieve
// ============================================================================

bool SelfRAGController::shouldRetrieve(const std::string& query,
                                        double             query_confidence) const {
    const double confidence = clamp01(query_confidence);
    if (confidence < cfg_.retrieval_confidence_threshold) {
        return true;
    }

    const auto query_tokens = tokenizeNormalized(query);
    if (query_tokens.empty()) {
        return false;
    }

    static const std::unordered_set<std::string> evidence_terms = {
        "who", "what", "when", "where", "which", "why", "how",
        "source", "sources", "citation", "citations", "evidence",
        "compare", "benchmark", "metrics", "according"
    };

    size_t evidence_hits = 0;
    for (const auto& token : query_tokens) {
        if (evidence_terms.count(token) > 0) {
            ++evidence_hits;
        }
    }

    const double evidence_cutoff = std::min(0.95, cfg_.retrieval_confidence_threshold + 0.25);
    if (evidence_hits > 0 && confidence < evidence_cutoff) {
        return true;
    }

    const double long_query_cutoff = std::min(0.98, cfg_.retrieval_confidence_threshold + 0.35);
    if (static_cast<int>(query_tokens.size()) > = 14 && confidence < long_query_cutoff) {
        return true;
    }

    return false;
}

// ============================================================================
// criticDocuments
// ============================================================================

std::vector<RatedDocument> SelfRAGController::criticDocuments(
        const std::string&                  query,
        const std::vector<SelfRAGDocument>& documents) const
{
    std::vector<RatedDocument> rated = {};

    rated.reserve(documents.size());

    // Tokenize the query once to avoid repeated work when scoring multiple
    // documents. This reduces per-document overhead in the common path where
    // no external critic is injected.
    const auto q_tokens = tokenizeNormalized(query);
    // Keep q_tokens as a vector and iterate it directly; using an
    // unordered_set here imposed hashing overhead in tight loops.

    // If requested, run a short micro-benchmark of the internal critic
    // (only when no external critic is injected) to gather timing
    // statistics for diagnosis. Controlled by environment variables to
    // avoid affecting normal test runs.
    if (!critic_cb_ && std::getenv("THEMIS_RAG_CRITIC_BENCH")) {
        const char* cnt = std::getenv("THEMIS_RAG_CRITIC_BENCH_COUNT");
        int iterations = 1000;
        if (cnt) {
            try {
                iterations = std::max(1, std::stoi(cnt));
            } catch (...) { iterations = 1000; }
        }

        std::vector<long long> samples;
        samples.reserve(iterations);

        auto ci_contains_local = [](const std::string& hay, const std::string& needle) {
            if (needle.empty() || hay.size() < needle.size()) {
              return false;
            }
            auto it = std::search(hay.begin(), hay.end(), needle.begin(), needle.end(),
                [](char a, char b) { return static_cast<char>(std::tolower(static_cast<unsigned char>(a))) == b; });
            return it != hay.end();
        };

        for (int i = 0; i < iterations; ++i) {
            const auto it_start = std::chrono::steady_clock::now();
            for (const auto& doc : documents) {
                size_t overlap = 0;
                for (const auto& t : q_tokens) {
                    if (ci_contains_local(doc.content, t)) {
                      ++overlap;
                    }
                }
                (void)overlap;
            }
            const auto it_end = std::chrono::steady_clock::now();
            samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(it_end - it_start).count());
        }

        if (!samples.empty()) {
            std::sort(samples.begin(), samples.end());
            auto p50 = samples[samples.size()/2];
            auto p95 = samples[static_cast<size_t>(samples.size()*0.95)];
            auto p99 = samples[static_cast<size_t>(samples.size()*0.99) < samples.size() ? static_cast<size_t>(samples.size()*0.99) : static_cast<int>(samples.size()) -1];
            long long sum = 0;
            for (auto v : samples) {
              sum += v;
            }
            long long mean = sum / static_cast<long long>(samples.size());
            std::fprintf(stderr, "SelfRAG critic microbench: iters=%d p50=%lld p95=%lld p99=%lld mean=%lld ns\n",
                         iterations, static_cast<long long>(p50), static_cast<long long>(p95), static_cast<long long>(p99), static_cast<long long>(mean));
        }
    }

    for (const auto& doc : documents) {
        double critic_score = 0;
        // Detailed tracing samples (env-gated) to diagnose rare spikes.
        std::int64_t doc_start_ns = 0;
        std::int64_t doc_end_ns = 0;
        std::int64_t token_loop_ns = 0;
        std::int64_t critic_cb_ns = 0;
        if (std::getenv("THEMIS_RAG_CRITIC_TRACE")) {
            doc_start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        if (critic_cb_) {
            // Caller-injected critic model (production path).
            const auto cb_start = std::chrono::steady_clock::now();
            critic_score = clamp01(critic_cb_(query, doc));
            const auto cb_end = std::chrono::steady_clock::now();
            if (std::getenv("THEMIS_RAG_CRITIC_TRACE")) {
                critic_cb_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(cb_end - cb_start).count();
                std::fprintf(stderr, "  DEBUG critic: callback_time_ns=%ld\n", critic_cb_ns);
            }
        } else {
            const double retrieval_signal = clamp01(doc.score);

            // Compute lexical overlap using pre-tokenized query tokens to
            // avoid re-tokenizing the query for every document.
            // Use a lowercased content string plus substring search over the
            // unique query token set. This avoids tokenizing the document and
            // reduces heap allocations for short passages used in unit tests.
            // Avoid allocating a lowercased copy of the document. Instead,
            // perform a case-insensitive search for each normalized token
            // using `std::search` with a predicate that lowercases on-the-fly.
            // Lowercase the document once and perform substring find() for
            // each token. This avoids repeated per-character tolower calls
            // and is faster for short passages used in unit tests.
            std::string content_lower = doc.content;
            std::transform(content_lower.begin(), content_lower.end(), content_lower.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

            size_t overlap = 0;
            const auto token_loop_start = std::chrono::steady_clock::now();
            for (const auto& t : q_tokens) {
                if (!t.empty() && content_lower.find(t) != std::string::npos) {
                  ++overlap;
                }
            }
            const auto token_loop_end = std::chrono::steady_clock::now();
            if (std::getenv("THEMIS_RAG_CRITIC_TRACE")) {
                token_loop_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(token_loop_end - token_loop_start).count();
            }
            const double overlap_signal = q_tokens.empty() ? 0.0 :
                static_cast<double>(overlap) / static_cast<double>(q_tokens.size());

            critic_score = clamp01(0.65 * retrieval_signal + 0.35 * overlap_signal);
                if (std::getenv("THEMIS_RAG_CRITIC_TRACE")) {
                    std::fprintf(stderr, "  DEBUG critic: retrieval=%0.4f overlap=%0.4f score=%0.6f\n",
                                 retrieval_signal, overlap_signal, critic_score);
                }
        }

        CriticVerdict verdict = {};
        if (critic_score >= cfg_.relevant_threshold) {
            verdict = CriticVerdict::Relevant;
        } else if (critic_score >= cfg_.partial_threshold) {
            verdict = CriticVerdict::Partial;
        } else {
            verdict = CriticVerdict::Irrelevant;
        }

        rated.push_back({doc, verdict, critic_score});

        if (std::getenv("THEMIS_RAG_CRITIC_TRACE")) {
            doc_end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            // store per-doc timings in thread-local vectors via static locals
            static thread_local std::vector<long long> doc_times;
            static thread_local std::vector<long long> token_times;
            static thread_local std::vector<std::pair<long long, std::string>> slow_docs;
            const long long doc_ns = static_cast<long long>(doc_end_ns - doc_start_ns);
            doc_times.push_back(doc_ns);
            token_times.push_back(static_cast<long long>(token_loop_ns));
            if (doc_ns > 50000) { // heuristic: flag docs slower than 50µs
                slow_docs.emplace_back(doc_ns, doc.id);
                // Optionally capture stack backtrace for slow critic doc processing
                if (std::getenv("THEMIS_RAG_CAPTURE_STACK_ON_SLOW")) {
#ifdef _WIN32
                    print_backtrace_if_enabled("critic_doc_slow");
#endif
                }
            }
            // On last document, print summary
            if (&doc == &documents.back()) {
                if (!doc_times.empty()) {
                    std::sort(doc_times.begin(), doc_times.end());
                    std::sort(token_times.begin(), token_times.end());
                    long long sum = 0;
                    for (auto v : doc_times) {
                      sum += v;
                    }
                    long long mean = sum / static_cast<long long>(doc_times.size());
                    long long p50 = doc_times[doc_times.size()/2];
                    size_t i95 = static_cast<size_t>(doc_times.size()*0.95);
                    if (i95 >= static_cast<int>(doc_times.size())) {
                      i95 = static_cast<int>(doc_times.size()) -1;
                    }
                    long long p95 = doc_times[i95];
                    size_t i99 = static_cast<size_t>(doc_times.size()*0.99);
                    if (i99 >= static_cast<int>(doc_times.size())) {
                      i99 = static_cast<int>(doc_times.size()) -1;
                    }
                    long long p99 = doc_times[i99];
                    long long t50 = token_times[token_times.size()/2];
                    std::fprintf(stderr, "SelfRAG critic trace: docs=%zu p50=%lld p95=%lld p99=%lld mean=%lld ns token_p50=%lld ns slow_count=%zu\n",
                                 doc_times.size(), p50, p95, p99, mean, t50, slow_docs.size());
                    if (!slow_docs.empty()) {
                        std::sort(slow_docs.begin(), slow_docs.end(), [](auto &a, auto &b){ return a.first > b.first; });
                        size_t show = std::min<size_t>(5, slow_docs.size());
                        std::fprintf(stderr, "Top %zu slow docs:\n", show);
                        for (size_t si = 0; si < show; ++si) {
                            std::fprintf(stderr, "  %zu: %lld ns id=%s\n", si+1, slow_docs[si].first, slow_docs[si].second.c_str());
                        }
                    }
                }
                doc_times.clear();
                token_times.clear();
                slow_docs.clear();
            }
        }
    }

    return rated;
}

// ============================================================================
// Helpers
// ============================================================================

std::vector<SelfRAGDocument> SelfRAGController::deduplicate(
        std::vector<SelfRAGDocument> candidates) const
{
    std::unordered_set<std::string> seen(seen_ids_.begin(), seen_ids_.end());
    std::vector<SelfRAGDocument>    fresh = {};

    fresh.reserve(candidates.size());

    for (auto& doc : candidates) {
        if (seen.insert(doc.id).second) {
            fresh.push_back(std::move(doc));
        }
    }
    return fresh;
}

// ============================================================================
// runRefinementLoop
// ============================================================================

SelfRAGResult SelfRAGController::runRefinementLoop(const std::string& query,
                                                    double             query_confidence)
{
    SelfRAGResult result;

    // No debug timing in production path.

    if (!shouldRetrieve(query, query_confidence)) {
        return result;  // retrieval_triggered stays false
    }

    result.retrieval_triggered = true;

    if (!retrieval_cb_) {
        throw std::runtime_error(
            "SelfRAGController: no retrieval callback set; "
            "call setRetrievalCallback() before runRefinementLoop()");
    }

    size_t relevant_count = 0;

    for (size_t round = 1; round <= cfg_.max_rounds; ++round) {
        // Timed retrieval/critic/dedup to diagnose intermittent performance
        const auto round_start = std::chrono::steady_clock::now();
        // Optional: micro-benchmark retrieval callback if requested via env.
        if (std::getenv("THEMIS_RAG_RETRIEVAL_BENCH")) {
            const char* cnt = std::getenv("THEMIS_RAG_RETRIEVAL_BENCH_COUNT");
            int iterations = 100;
            if (cnt) {
                try { iterations = std::max(1, std::stoi(cnt)); } catch (...) { iterations = 100; }
            }

            std::vector<long long> samples;
            samples.reserve(iterations);
            for (int i = 0; i < iterations; ++i) {
                const auto t0 = std::chrono::steady_clock::now();
                auto tmp = retrieval_cb_(query, cfg_.top_k);
                const auto t1 = std::chrono::steady_clock::now();
                (void)tmp; // discard
                samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            }
            std::sort(samples.begin(), samples.end());
            long long sum = 0;
            for (auto v : samples) {
              sum += v;
            }
            long long mean = samples.empty() ? 0 : sum / static_cast<long long>(samples.size());
            long long p50 = samples.empty() ? 0 : samples[samples.size()/2];
            size_t idx95 = static_cast<size_t>(samples.size() * 0.95);
            if (idx95 >= static_cast<int>(samples.size())) {
              idx95 = static_cast<int>(samples.size()) - 1;
            }
            long long p95 = samples.empty() ? 0 : samples[idx95];
            size_t idx99 = static_cast<size_t>(samples.size() * 0.99);
            if (idx99 >= static_cast<int>(samples.size())) {
              idx99 = static_cast<int>(samples.size()) - 1;
            }
            long long p99 = samples.empty() ? 0 : samples[idx99];
            std::fprintf(stderr, "SelfRAG retrieval microbench: iters=%d p50=%lld p95=%lld p99=%lld mean=%lld ns\n",
                         iterations,
                         static_cast<long long>(p50), static_cast<long long>(p95), static_cast<long long>(p99), static_cast<long long>(mean));
        }

        // Retrieve documents for this round.
        auto candidates = retrieval_cb_(query, cfg_.top_k);
        const auto after_retrieval = std::chrono::steady_clock::now();
        // If the retrieval call was slow, optionally capture a stack trace
        if (std::getenv("THEMIS_RAG_CAPTURE_STACK_ON_SLOW")) {
            const auto retrieval_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(after_retrieval - round_start).count();
            const long long threshold_ns = 100000; // 100µs heuristic threshold
            if (retrieval_ns > threshold_ns) {
#ifdef _WIN32
                print_backtrace_if_enabled("retrieval_slow");
#else
                (void)retrieval_ns; // silence unused when not on Windows
#endif
            }
        }
        auto fresh      = deduplicate(std::move(candidates));
        const auto after_dedup = std::chrono::steady_clock::now();

        // Track seen ids to avoid re-scoring the same passages.
        for (const auto& d : fresh) {
            seen_ids_.push_back(d.id);
        }

        // Critic pass.
        const auto critic_start = std::chrono::steady_clock::now();
        auto rated = criticDocuments(query, fresh);
        const auto after_critic = std::chrono::steady_clock::now();

        // Accumulate per-round stats.
        RefinementRoundStats stats;
        stats.round     = round;
        stats.retrieved = fresh.size();

        for (const auto& r : rated) {
            switch (r.verdict) {
                case CriticVerdict::Relevant:
                    result.relevant_docs.push_back(r);
                    ++stats.relevant;
                    ++relevant_count;
                    break;
                case CriticVerdict::Partial:
                    result.partial_docs.push_back(r);
                    ++stats.partial;
                    break;
                case CriticVerdict::Irrelevant:
                    ++stats.irrelevant;
                    break;
            }
        }

        // Compute debug timings if requested via environment variable.
        if (std::getenv("THEMIS_RAG_DEBUG")) {
            const auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(after_critic - round_start).count();
            const auto retrieval_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(after_retrieval - round_start).count();
            const auto dedup_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(after_dedup - after_retrieval).count();
            const auto critic_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(after_critic - critic_start).count();
            std::fprintf(stderr, "SelfRAG round %zu ns=%lld retrieval_ns=%lld dedup_ns=%lld critic_ns=%lld retrieved=%zu relevant=%zu partial=%zu irrelevant=%zu\n",
                         round,
                         static_cast<long long>(total_ns),
                         static_cast<long long>(retrieval_ns),
                         static_cast<long long>(dedup_ns),
                         static_cast<long long>(critic_ns),
                         stats.retrieved,
                         stats.relevant,
                         stats.partial,
                         stats.irrelevant);
        }

        // Check early-stop target.
        if (relevant_count >= cfg_.target_relevant_docs) {
            stats.stop_early = true;
            result.round_stats.push_back(stats);
            result.total_rounds_used = round;
            return result;
        }
        result.round_stats.push_back(stats);
        result.total_rounds_used = round;
    }

    // production path: no timing logs

    return result;
}

// ============================================================================
// reset
// ============================================================================

void SelfRAGController::reset() {
    seen_ids_.clear();
}

} // namespace rag
} // namespace themis
