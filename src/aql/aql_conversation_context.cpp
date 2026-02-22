/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_conversation_context.cpp                       ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-22 08:38:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     209                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "aql/aql_conversation_context.h"
#include "llm/llama_wrapper.h"

#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace themis {
namespace aql {

// ============================================================================
// Pimpl
// ============================================================================

class AQLConversationContext::Impl {
public:
    explicit Impl(LLMAQLHandler& handler)
        : handler_(handler)
        , turn_count_(0)
    {}

    LLMAQLHandler&                   handler_;
    std::string                      schema_context_;
    std::vector<llm::ChatMessage>    history_;
    std::string                      last_query_;
    std::size_t                      turn_count_;

    // Build the system prompt once so every call uses a consistent context
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
        return sp.str();
    }

    // Strip markdown code fences from the LLM response
    static std::string cleanQuery(const std::string& raw) {
        std::string out = raw;
        // Remove ```aql ... ``` or ``` ... ``` blocks
        const std::string fence = "```";
        auto start_pos = out.find(fence);
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
        if (first == std::string::npos) return "";
        return out.substr(first, last - first + 1);
    }

    // Common implementation: push a user message, call LLM, store the response
    std::string callLLM(const std::string& user_message) {
        history_.emplace_back("user", user_message);

        try {
            const std::string response = handler_.executeChat(history_);
            const std::string query    = cleanQuery(response);

            // Store assistant turn
            history_.emplace_back("assistant", response);
            last_query_ = query;
            ++turn_count_;
            return query;
        } catch (const std::exception& e) {
            // Remove the user message we just added so history stays consistent
            history_.pop_back();
            spdlog::warn("AQLConversationContext: LLM call failed: {}", e.what());
            return "";
        }
    }
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

AQLConversationContext::AQLConversationContext(LLMAQLHandler& handler)
    : impl_(std::make_unique<Impl>(handler)) {}

AQLConversationContext::~AQLConversationContext() = default;

AQLConversationContext::AQLConversationContext(AQLConversationContext&&) noexcept = default;
AQLConversationContext& AQLConversationContext::operator=(AQLConversationContext&&) noexcept = default;

// ============================================================================
// Configuration
// ============================================================================

void AQLConversationContext::setSchemaContext(const std::string& schema) {
    impl_->schema_context_ = schema;
    // If the history already has a system message, update it
    if (!impl_->history_.empty() && impl_->history_.front().role == "system") {
        impl_->history_.front().content = impl_->buildSystemPrompt();
    }
}

const std::string& AQLConversationContext::getSchemaContext() const {
    return impl_->schema_context_;
}

// ============================================================================
// Conversation
// ============================================================================

std::string AQLConversationContext::start(const std::string& intent) {
    if (intent.empty()) {
        throw std::invalid_argument(
            "AQLConversationContext::start: intent must not be empty"
        );
    }

    // Reset state for a fresh conversation
    impl_->history_.clear();
    impl_->last_query_.clear();
    impl_->turn_count_ = 0;

    // Always prepend a fresh system prompt
    impl_->history_.emplace_back("system", impl_->buildSystemPrompt());

    return impl_->callLLM(intent);
}

std::string AQLConversationContext::refine(const std::string& instruction) {
    if (instruction.empty()) {
        throw std::invalid_argument(
            "AQLConversationContext::refine: instruction must not be empty"
        );
    }
    if (impl_->turn_count_ == 0) {
        throw std::logic_error(
            "AQLConversationContext::refine: call start() before refine()"
        );
    }

    // Compose a user message that includes the current query for context
    std::ostringstream msg;
    if (!impl_->last_query_.empty()) {
        msg << "Current query:\n```\n" << impl_->last_query_ << "\n```\n\n";
    }
    msg << "Refinement: " << instruction;

    return impl_->callLLM(msg.str());
}

void AQLConversationContext::reset() {
    impl_->history_.clear();
    impl_->last_query_.clear();
    impl_->turn_count_ = 0;
}

// ============================================================================
// Inspection
// ============================================================================

std::size_t AQLConversationContext::turnCount() const {
    return impl_->turn_count_;
}

const std::string& AQLConversationContext::lastQuery() const {
    return impl_->last_query_;
}

std::vector<std::pair<std::string, std::string>> AQLConversationContext::getHistory() const {
    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(impl_->history_.size());
    for (const auto& msg : impl_->history_) {
        out.emplace_back(msg.role, msg.content);
    }
    return out;
}

} // namespace aql
} // namespace themis
