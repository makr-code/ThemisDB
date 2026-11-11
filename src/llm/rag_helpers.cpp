#include "llm/rag_helpers.h"
#include "llm/prompt_safety_utils.h"
#include "rag/rag_context_assembler.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace themis::llm {

InferenceRequest prepareRAGRequest(
    const RAGContext& context, const InferenceRequest& request,
    std::size_t model_context_tokens) {
    if (context.max_context_tokens < 0 || context.response_budget_tokens <= 0 ||
        request.max_tokens <= 0) {
        throw std::invalid_argument("Invalid RAG context or generation budget");
    }

    auto sanitize = [](const std::string& input, const char* field) {
        std::string output;
        if (!prompt_safety::sanitizePromptWithSharedPolicy(input, output, nullptr, nullptr)) {
            throw std::invalid_argument(std::string("RAG ") + field + " blocked by prompt policy");
        }
        return output;
    };
    const auto query = sanitize(request.prompt.empty() ? context.query : request.prompt, "query");
    InferenceRequest prepared = request;
    const std::string system = request.system_prompt
        ? sanitize(*request.system_prompt, "system prompt") : std::string{};
    if (request.system_prompt) {
        prepared.system_prompt = system;
    }

    const std::string mode = request.metadata.is_object()
        ? request.metadata.value("rag_mode", std::string{"text"}) : std::string{"text"};
    const bool memory_slots = mode == "tensor_hybrid" || mode == "tensor_prefix";
    if (mode != "text" && !memory_slots) {
        throw std::invalid_argument("Unsupported RAG mode: " + mode);
    }
    const auto prompt_template = sanitize(
        context.context_template.empty()
            ? std::string{"Context:\n{context}\n\nQuestion: {query}\n\n"
                          "Answer based on the context provided above:"}
            : context.context_template, "template");
    if (prompt_template.find("{context}") == std::string::npos ||
        prompt_template.find("{query}") == std::string::npos) {
        throw std::invalid_argument("RAG template must contain {context} and {query}");
    }

    std::vector<rag::RetrievedChunk> chunks;
    chunks.reserve(context.documents.size());
    std::size_t blocked_documents = 0;
    for (const auto& document : context.documents) {
        if (!std::isfinite(document.relevance_score)) {
            throw std::invalid_argument("RAG document score must be finite");
        }
        rag::RetrievedChunk chunk;
        if (!prompt_safety::sanitizePromptWithSharedPolicy(
                document.content, chunk.content, nullptr, nullptr)) {
            chunk.content = "[BLOCKED_CONTEXT]";
            ++blocked_documents;
        }
        chunk.source = sanitize(document.source, "source");
        chunk.chunk_id = chunk.source;
        chunk.relevance_score = document.relevance_score;
        chunks.push_back(std::move(chunk));
    }
    std::stable_sort(chunks.begin(), chunks.end(), [](const auto& left, const auto& right) {
        if (left.relevance_score != right.relevance_score) {
            return left.relevance_score > right.relevance_score;
        }
        if (left.source != right.source) {
            return left.source < right.source;
        }
        return left.content < right.content;
    });

    int slots = 6;
    int slot_chars = 280;
    if (memory_slots && request.metadata.is_object()) {
        slots = std::clamp(request.metadata.value("rag_tensor_slots", slots), 1, 16);
        slot_chars = std::clamp(request.metadata.value("rag_tensor_slot_chars", slot_chars), 80, 1200);
    }
    bool compacted = false;
    if (memory_slots && chunks.size() > static_cast<std::size_t>(slots)) {
        chunks.resize(static_cast<std::size_t>(slots));
        compacted = true;
    }
    for (auto& chunk : chunks) {
        if (memory_slots && chunk.content.size() > static_cast<std::size_t>(slot_chars)) {
            auto end = static_cast<std::size_t>(slot_chars);
            while (end > 0 &&
                   (static_cast<unsigned char>(chunk.content[end]) & 0xC0) == 0x80) {
                --end;
            }
            chunk.content.resize(end);
            compacted = true;
        }
        const auto label = memory_slots ? "MEMORY_SLOT" : "DOCUMENT";
        chunk.content = "[" + std::string(label) + " source=" + chunk.source + "]\n" +
                        chunk.content + "\n[/" + label + "]\n\n";
    }

    prompt_engineering::RAGPromptConfig format_config;
    format_config.max_context_length = std::numeric_limits<std::size_t>::max();
    format_config.context_header.clear();
    format_config.chunk_separator.clear();
    format_config.include_source_citations = false;
    format_config.rank_by_relevance = false;
    const prompt_engineering::RAGPromptBuilder formatter(format_config);
    const auto overhead = formatter.build(prompt_template, query, {});

    rag::RAGContextAssemblerConfig config;
    const auto actual_window = model_context_tokens > 0
        ? model_context_tokens : kDefaultContextWindowTokens;
    config.model_context_tokens = context.max_context_tokens > 0
        ? std::min(actual_window, static_cast<std::size_t>(context.max_context_tokens))
        : actual_window;
    config.min_response_tokens = static_cast<std::size_t>(context.response_budget_tokens);
    config.allow_partial_chunk = false;
    const auto assembled = rag::RAGContextAssembler(config).assemble(chunks, system, overhead);
    if (!chunks.empty() && assembled.chunks_used.empty()) {
        throw std::invalid_argument("RAG context budget cannot fit any evidence");
    }
    prepared.prompt = formatter.build(prompt_template, query, assembled.chunks_used);
    const auto budget = ContextWindowBudget::compute(
        config.model_context_tokens, system, prepared.prompt, config.min_response_tokens);
    if (budget.system_prompt_tokens + budget.query_tokens + budget.reserved_response_tokens >
        budget.model_max_tokens) {
        throw std::invalid_argument("RAG prompt and response reservation exceed the model window");
    }
    prepared.max_tokens = static_cast<int>(std::min<std::size_t>(
        static_cast<std::size_t>(request.max_tokens), budget.reserved_response_tokens));
    prepared.metadata["rag_documents_used"] = assembled.chunks_used.size();
    prepared.metadata["rag_context_truncated"] =
        compacted || assembled.chunks_used.size() < context.documents.size();
    prepared.metadata["rag_blocked_documents"] = blocked_documents;
    return prepared;
}

} // namespace themis::llm
