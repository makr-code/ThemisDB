/**
 * @file chunk_text_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/ingestion_step.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <regex>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

/**
 * @brief `builtin.chunk_text` — splits `ctx.raw_text` into `TextChunk` objects.
 *
 * Strategies:
 *  - `fixed`    — chunks of `size` characters with `overlap` overlap
 *  - `sentence` — one chunk per sentence (split on ". " / "! " / "? ")
 *  - `section`  — §-aware: each paragraph starting with "§" or "Art." is a chunk
 *
 * Config keys (all optional):
 *  - `strategy`  string  default "fixed"
 *  - `size`      number  default 512
 *  - `overlap`   number  default 64
 */
class ChunkTextStep : public IIngestionStep {
public:
    // IThemisPlugin
    const char* getName()    const override { return "builtin.chunk_text"; }
    const char* getVersion() const override { return "0.0.1"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx,
                         const StepConfig& cfg) override {
        if (ctx.raw_text.empty()) {
            ctx.warnings.push_back("chunk_text: raw_text is empty — skipping");
            return {};
        }

        const std::string strategy =
            cfg.config.value("strategy", std::string("fixed"));
        const std::size_t chunk_size =
            cfg.config.value("size", 512);
        const std::size_t overlap =
            cfg.config.value("overlap", 64);

        if (strategy == "section") {
            chunkBySection(ctx);
        } else if (strategy == "sentence") {
            chunkBySentence(ctx, chunk_size, overlap);
        } else {
            chunkFixed(ctx, chunk_size, overlap);
        }
        return {};
    }

private:
    // ── §-aware section chunking ───────────────────────────────────────────
    static void chunkBySection(ExtractionContext& ctx) {
        // Split on lines that start with § or Art.
        static const std::regex section_re(
            R"((?m)^(§\s*\d+[\w\s]*|Art\.\s*\d+[\w\s]*)\s*\n)",
            std::regex::ECMAScript);

        const std::string& text = ctx.raw_text;
        std::sregex_iterator it(text.begin(), text.end(), section_re);
        std::sregex_iterator end = {};

        std::uint32_t seq = 0;
        std::size_t   prev_start = 0;
        std::string   prev_ref = {};

        auto emit = [&](std::size_t start, std::size_t end_pos,
                        const std::string& ref) {
            if (end_pos <= start) {
              return;
            }
            TextChunk c;
            c.seq         = seq++;
            c.text        = text.substr(start, end_pos - start);
            c.char_start  = static_cast<std::uint64_t>(start);
            c.char_end    = static_cast<std::uint64_t>(end_pos);
            c.section_ref = ref;
            ctx.chunks.push_back(std::move(c));
        };

        for (; it != end; ++it) {
            const std::size_t match_pos =
                static_cast<std::size_t>(it->position());
            emit(prev_start, match_pos, prev_ref);
            prev_start = match_pos;
            prev_ref   = (*it)[1].str();
        }
        // Last section
        emit(prev_start,static_cast<int>(text.size()), prev_ref);
    }

    // ── Sentence chunking ──────────────────────────────────────────────────
    static void chunkBySentence(ExtractionContext& ctx,
                                 std::size_t max_size, std::size_t overlap) {
        const std::string& text = ctx.raw_text;
        // Simple sentence splitter on ". " / "! " / "? "
        std::size_t pos = 0;
        std::uint32_t seq = 0;
        std::string current = {};
        std::size_t current_start = 0;

        auto emit = [&]() {
            if (current.empty()) {
              return;
            }
            TextChunk c;
            c.seq        = seq++;
            c.text       = current;
            c.char_start = static_cast<std::uint64_t>(current_start);
            c.char_end   = static_cast<std::uint64_t>(current_start + static_cast<int>(current.size()) );
            ctx.chunks.push_back(std::move(c));
            // Overlap: keep last `overlap` chars for next chunk
            if (overlap > 0 && static_cast<int>(current.size()) > overlap) {
                current = current.substr(static_cast<int>(current.size()) - overlap);
                current_start += (static_cast<int>(current.size()) - overlap); // approximate
            } else {
                current.clear();
            }
        };

        while (static_cast<size_t>(pos) <static_cast<int>(text.size())) {
            current += text[pos];
            if (pos + 1 < text.size() &&
                (text[pos] == '.' || text[pos] == '!' || text[pos] == '?') &&
                text[pos + 1] == ' ') {
                if (static_cast<int>(current.size()) >= max_size / 2) {
                    emit();
                    current_start = pos + 2;
                }
            }
            if (static_cast<int>(current.size()) >= max_size) {
                emit();
                current_start = pos + 1;
            }
            ++pos;
        }
        if (!current.empty()) {
            TextChunk c;
            c.seq        = seq++;
            c.text       = current;
            c.char_start = static_cast<std::uint64_t>(current_start);
            c.char_end   = static_cast<std::uint64_t>(current_start + static_cast<int>(current.size()) );
            ctx.chunks.push_back(std::move(c));
        }
    }

    // ── Fixed-size chunking ────────────────────────────────────────────────
    static void chunkFixed(ExtractionContext& ctx,
                            std::size_t size, std::size_t overlap) {
        const std::string& text = ctx.raw_text;
        const std::size_t step = (size > overlap) ? (size - overlap) : size;
        std::uint32_t seq = 0;
        for (std::size_t start = 0; start < text.size(); start += step) {
            const std::size_t end = std::min(start + size, text.size());
            TextChunk c;
            c.seq        = seq++;
            c.text       = text.substr(start, end - start);
            c.char_start = static_cast<std::uint64_t>(start);
            c.char_end   = static_cast<std::uint64_t>(end);
            ctx.chunks.push_back(std::move(c));
            if (end == text.size()) {
              break;
            }
        }
    }
};

} // namespace builtin
} // namespace ingestion
} // namespace themis
