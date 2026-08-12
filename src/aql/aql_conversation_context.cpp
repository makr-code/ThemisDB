/**
 * @file aql_conversation_context.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_conversation_context.h"

#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

#include "llm/llama_wrapper.h"

namespace themis {
namespace aql {

// ============================================================================
// Pimpl
// ============================================================================

class AQLConversationContext::Impl {
  public:
    explicit Impl(LLMAQLHandler &handler, AQLConversationContext::Config config,
                  std::unique_ptr<TokenEstimator> estimator,
                  IHistoryCompressor* compressor = nullptr)
        : handler_(handler), config_(config),
          estimator_(estimator ? std::move(estimator) : std::make_unique<CharDivisionEstimator>()), 
          compressor_(compressor), turn_count_(0) {}

    LLMAQLHandler &handler_;
    AQLConversationContext::Config config_;
    std::unique_ptr<TokenEstimator> estimator_;
    IHistoryCompressor* compressor_;  // Non-owning pointer to compressor (can be nullptr)
    std::string schema_context_;
    std::vector<llm::ChatMessage> history_;
    std::string last_query_;
    std::size_t turn_count_;
    mutable std::shared_mutex history_mutex_; // guards history_, turn_count_, last_query_, compressor_
    std::mutex call_mutex_;                   // serializes LLM round-trips and reset/start

    // Build the system prompt once so every call uses a consistent context.
    // For very small token budgets, shrink the prompt to a compact variant so
    // max_history_tokens can still be satisfied.
    std::string buildSystemPrompt() const {
        std::ostringstream sp;
        sp << "You are an expert in AQL (ArangoDB Query Language) for ThemisDB.\n"
           << "Your job is to produce a single valid AQL query based on the "
           << "conversation history.\n";
        if (!schema_context_.empty()) {
            sp << "\nDatabase schema:\n" << schema_context_ << "\n";
        }
        sp << "\nRules:\n"
           << "- Return ONLY the AQL query, no markdown fences, no explanations.\n"
           << "- Incorporate every refinement from the conversation history.\n"
           << "- Use standard AQL keywords (FOR, FILTER, SORT, LIMIT, RETURN).\n";

        const std::string prompt = sp.str();
        if (config_.max_history_tokens > 0 && estimator_->estimate(prompt) > config_.max_history_tokens) {
            if (!schema_context_.empty()) {
                return "Return exactly one valid AQL query. Use this schema context:\n" + schema_context_;
            }
            return "Return exactly one valid AQL query.";
        }
        return prompt;
    }

    // Strip markdown code fences from the LLM response
    static std::string cleanQuery(const std::string &raw) {
        std::string out = raw;
        // Remove ```aql ... ``` or ``` ... ``` blocks
        const std::string fence = "```";
        auto start_pos          = out.find(fence);
        if (start_pos != std::string::npos) {
            auto content_start = out.find('\n', start_pos);
            if (content_start != std::string::npos) {
                ++content_start;
                auto end_pos = out.find(fence, content_start);
                if (end_pos != std::string::npos) {
                    out = out.substr(content_start, end_pos - content_start);
                }
            }
        }
        // Trim leading/trailing whitespace
        auto first = out.find_first_not_of(" \t\r\n");
        auto last  = out.find_last_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return "";
        }
        return out.substr(first, last - first + 1);
    }

    // Compute the total estimated token count of the current history.
    // Caller must hold history_mutex_.
    std::size_t estimateHistoryTokens() const {
        std::size_t total = 0;
        for (const auto &msg : history_) {
            total += estimator_->estimate(msg.role);
            total += estimator_->estimate(msg.content);
        }
        return total;
    }

    // Evict oldest user+assistant pairs until both the turn-count and
    // token-budget constraints are satisfied (or no more pairs remain).
    // The system message at index 0 is always preserved.
    // Caller must hold history_mutex_.
    void evictOldestPairs(std::size_t extra_tokens) {
        // history_ layout: [system?, user, assistant, user, assistant, ...]
        // A "pair" is two consecutive messages starting at an odd index.
        while (!history_.empty()) {
            bool over_turns = (turn_count_ > 0) && (config_.max_turns > 0) && (turn_count_ >= config_.max_turns);

            std::size_t current_tokens = estimateHistoryTokens();
            bool over_tokens
                = (config_.max_history_tokens > 0) && (current_tokens + extra_tokens > config_.max_history_tokens);

            if (!over_turns && !over_tokens) {
                break;
            }

            // Find the first user message (after the optional system message)
            std::size_t first_user = (history_.front().role == "system") ? 1 : 0;
            if (first_user + 1 >= history_.size()) {
                break; // nothing left to evict
            }

            // Erase the user + assistant pair
            history_.erase(history_.begin() + static_cast<std::ptrdiff_t>(first_user),
                           history_.begin() + static_cast<std::ptrdiff_t>(first_user) + 2);
            if (turn_count_ > 0) {
                --turn_count_;
            }
        }
    }

    // Common implementation for one LLM round-trip.
    // Caller MUST hold call_mutex_.  Acquires/releases history_mutex_ internally
    // only for brief state reads and writes; the LLM call itself runs lock-free.
    std::string callLLMImpl(const std::string &user_message) {
        // Evict oldest pairs if needed, push the new user message, and snapshot.
        std::vector<llm::ChatMessage> history_snapshot;
        {
            std::unique_lock<std::shared_mutex> lock(history_mutex_);
            const std::size_t new_msg_tokens = estimator_->estimate("user") + estimator_->estimate(user_message);
            evictOldestPairs(new_msg_tokens);
            history_.emplace_back("user", user_message);
            history_snapshot = history_;
        }

        try {
            std::string response;
            if (config_.llm_executor) {
                std::vector<std::pair<std::string, std::string>> pairs;
                pairs.reserve(history_snapshot.size());
                for (const auto &m : history_snapshot) {
                    pairs.emplace_back(m.role, m.content);
                }
                response = config_.llm_executor(pairs);
            } else {
                response = handler_.executeChat(history_snapshot);
            }
            const std::string query = cleanQuery(response);

            {
                std::unique_lock<std::shared_mutex> lock(history_mutex_);

                // Reserve room for the assistant response before appending it.
                const std::size_t assistant_tokens = estimator_->estimate("assistant") + estimator_->estimate(response);
                evictOldestPairs(assistant_tokens);

                history_.emplace_back("assistant", response);
                last_query_ = query;
                ++turn_count_;

                // Also enforce token budget after append; do not apply max_turns
                // here because turn-count eviction is handled before adding a turn.
                if (config_.max_history_tokens > 0) {
                    while (estimateHistoryTokens() > config_.max_history_tokens) {
                        const std::size_t first_user = (history_.front().role == "system") ? 1 : 0;
                        if (first_user + 1 >= history_.size()) {
                            break;
                        }
                        history_.erase(history_.begin() + static_cast<std::ptrdiff_t>(first_user),
                                       history_.begin() + static_cast<std::ptrdiff_t>(first_user) + 2);
                        if (turn_count_ > 0) {
                            --turn_count_;
                        }
                    }
                }

                // Trigger episodic memory compression if enabled and threshold exceeded
                if (config_.enable_episodic_compaction && 
                    config_.episodic_compaction_trigger_tokens > 0 && 
                    compressor_ && compressor_->isAvailable()) {
                    
                    const std::size_t current_tokens = estimateHistoryTokens();
                    if (current_tokens > static_cast<std::size_t>(config_.episodic_compaction_trigger_tokens)) {
                        try {
                            // Convert history to vector of pairs for compressor
                            std::vector<std::pair<std::string, std::string>> history_pairs;
                            history_pairs.reserve(history_.size());
                            for (const auto &msg : history_) {
                                history_pairs.emplace_back(msg.role, msg.content);
                            }

                            // Compress history
                            int32_t max_tokens = config_.max_history_tokens > 0 ? 
                                static_cast<int32_t>(config_.max_history_tokens) : 8192;
                            float min_similarity = config_.episodic_compression_gate_similarity;
                            
                            auto result = compressor_->compressHistory(history_pairs, max_tokens, min_similarity);
                            if (result && result->semantic_similarity >= min_similarity) {
                                // Compression succeeded; replace history with compressed summary + system message
                                std::vector<llm::ChatMessage> compressed_history;
                                if (!history_.empty() && history_.front().role == "system") {
                                    compressed_history.emplace_back("system", history_.front().content);
                                }
                                compressed_history.emplace_back("assistant", result->summary);
                                history_ = std::move(compressed_history);
                                spdlog::debug("AQLConversationContext: Episodic compression triggered. "
                                           "Original={} tokens, Compressed={} tokens, Similarity={}",
                                           result->original_token_count, result->compressed_token_count,
                                           result->semantic_similarity);
                            }
                        } catch (const std::exception &e) {
                            spdlog::warn("AQLConversationContext: Episodic compression failed: {}", e.what());
                            // Continue without compression if it fails
                        }
                    }
                }
            }
            return query;
        } catch (const std::exception &e) {
            // Roll back the user message we pushed.  Since call_mutex_ is held,
            // no other thread could have appended to history_ since we pushed it;
            // the check guards against an empty history_ resulting from reset().
            {
                std::unique_lock<std::shared_mutex> lock(history_mutex_);
                if (!history_.empty() && history_.back().role == "user") {
                    history_.pop_back();
                }
            }
            spdlog::warn("AQLConversationContext: LLM call failed: {}", e.what());
            return "";
        }
    }

    // External entry point: acquires call_mutex_ then delegates.
    std::string callLLM(const std::string &user_message) {
        std::lock_guard<std::mutex> call_lock(call_mutex_);
        return callLLMImpl(user_message);
    }
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

AQLConversationContext::AQLConversationContext(LLMAQLHandler &handler)
    : AQLConversationContext(handler, Config{}, nullptr, nullptr) {}

AQLConversationContext::AQLConversationContext(LLMAQLHandler &handler, Config config,
                                               std::unique_ptr<TokenEstimator> estimator)
    : AQLConversationContext(handler, config, std::move(estimator), nullptr) {}

AQLConversationContext::AQLConversationContext(LLMAQLHandler &handler, Config config,
                                               std::unique_ptr<TokenEstimator> estimator,
                                               IHistoryCompressor* compressor)
    : impl_(std::make_unique<Impl>(handler, config, std::move(estimator), compressor)) {}

AQLConversationContext::~AQLConversationContext() = default;

AQLConversationContext::AQLConversationContext(AQLConversationContext &&) noexcept            = default;
AQLConversationContext &AQLConversationContext::operator=(AQLConversationContext &&) noexcept = default;

// ============================================================================
// Configuration
// ============================================================================

void AQLConversationContext::setSchemaContext(const std::string &schema) {
    std::unique_lock<std::shared_mutex> lock(impl_->history_mutex_);
    impl_->schema_context_ = schema;
    // If the history already has a system message, update it
    if (!impl_->history_.empty() && impl_->history_.front().role == "system") {
        impl_->history_.front().content = impl_->buildSystemPrompt();
    }
}

std::string AQLConversationContext::getSchemaContext() const {
    std::shared_lock<std::shared_mutex> lock(impl_->history_mutex_);
    return impl_->schema_context_;
}

void AQLConversationContext::setCompressor(IHistoryCompressor* compressor) {
    std::unique_lock<std::shared_mutex> lock(impl_->history_mutex_);
    impl_->compressor_ = compressor;
}

IHistoryCompressor* AQLConversationContext::getCompressor() const {
    std::shared_lock<std::shared_mutex> lock(impl_->history_mutex_);
    return impl_->compressor_;
}


// ============================================================================
// Conversation
// ============================================================================

std::string AQLConversationContext::start(const std::string &intent) {
    if (intent.empty()) {
        throw std::invalid_argument("AQLConversationContext::start: intent must not be empty");
    }

    // Hold call_mutex_ for the entire operation so that a concurrent refine()
    // or reset() waits until this start() — including the LLM call — finishes.
    std::lock_guard<std::mutex> call_lock(impl_->call_mutex_);
    {
        std::unique_lock<std::shared_mutex> lock(impl_->history_mutex_);
        impl_->history_.clear();
        impl_->last_query_.clear();
        impl_->turn_count_ = 0;
        impl_->history_.emplace_back("system", impl_->buildSystemPrompt());
    }

    return impl_->callLLMImpl(intent);
}

std::string AQLConversationContext::refine(const std::string &instruction) {
    if (instruction.empty()) {
        throw std::invalid_argument("AQLConversationContext::refine: instruction must not be empty");
    }
    {
        std::unique_lock<std::shared_mutex> lock(impl_->history_mutex_);
        if (impl_->last_query_.empty()) {
            throw std::logic_error("AQLConversationContext::refine: call start() before refine()");
        }
    }

    // Compose a user message that includes the current query for context
    std::ostringstream msg;
    {
        std::unique_lock<std::shared_mutex> lock(impl_->history_mutex_);
        if (!impl_->last_query_.empty()) {
            msg << "Current query:\n```\n" << impl_->last_query_ << "\n```\n\n";
        }
    }
    msg << "Refinement: " << instruction;

    return impl_->callLLM(msg.str());
}

void AQLConversationContext::reset() {
    // Acquire call_mutex_ so that any in-flight LLM call finishes before the
    // history is cleared; otherwise a finishing callLLMImpl could append to an
    // already-reset history.
    std::lock_guard<std::mutex> call_lock(impl_->call_mutex_);
    std::unique_lock<std::shared_mutex> lock(impl_->history_mutex_);
    impl_->history_.clear();
    impl_->last_query_.clear();
    impl_->turn_count_ = 0;
}

// ============================================================================
// Inspection
// ============================================================================

std::size_t AQLConversationContext::turnCount() const {
    std::shared_lock<std::shared_mutex> lock(impl_->history_mutex_);
    return impl_->turn_count_;
}

std::size_t AQLConversationContext::tokenCount() const {
    std::shared_lock<std::shared_mutex> lock(impl_->history_mutex_);
    return impl_->estimateHistoryTokens();
}

std::string AQLConversationContext::lastQuery() const {
    std::shared_lock<std::shared_mutex> lock(impl_->history_mutex_);
    return impl_->last_query_;
}

std::vector<std::pair<std::string, std::string>> AQLConversationContext::getHistory() const {
    std::shared_lock<std::shared_mutex> lock(impl_->history_mutex_);
    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(impl_->history_.size());
    for (const auto &msg : impl_->history_) {
        out.emplace_back(msg.role, msg.content);
    }
    return out;
}

} // namespace aql
} // namespace themis
