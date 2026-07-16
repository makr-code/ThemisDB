/**
 * @file search_highlighter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <string>
#include <vector>

namespace themis {

/**
 * @brief Search result highlighter and snippet extractor.
 *
 * All methods are stateless; the Config merely controls presentation details.
 */
class SearchHighlighter {
public:
    /**
     * @brief Configuration for the highlighter.
     */
    struct Config {
        /// HTML tag wrapping matched terms (open tag).
        std::string highlight_open  = "<mark>";
        /// HTML tag wrapping matched terms (close tag).
        std::string highlight_close = "</mark>";
        /// Ellipsis string inserted at snippet boundaries.
        std::string ellipsis        = "...";
        /// Minimum snippet window (characters on each side of a match).
        size_t      min_window      = 40;
        /// Maximum length of the returned snippet in characters.
        size_t      max_snippet_len = 300;
        /// Whether tokenisation is case-insensitive.
        bool        case_insensitive = true;
    };

    SearchHighlighter();
    explicit SearchHighlighter(Config config);

    // ── Primary public API ────────────────────────────────────────────────────

    /**
     * @brief Highlight all occurrences of @p terms inside @p text.
     *
     * Each token in @p terms that appears in @p text is wrapped with the
     * configured highlight_open / highlight_close tags.  Overlapping matches
     * are merged so that a single term never receives nested tags.
     *
     * @param text  Source document text (UTF-8).
     * @param terms Search terms to highlight.
     * @return      @p text with matched terms wrapped in highlight tags, or
     *              an empty string when @p text is empty.
     */
    std::string highlight(const std::string& text,
                          const std::vector<std::string>& terms) const noexcept;

    /**
     * @brief Extract the best-matching passage from @p text.
     *
     * A sliding window of @p window_size characters is scored by the number
     * of distinct search terms it contains.  The highest-scoring window is
     * returned with matched terms highlighted and boundary ellipses appended.
     *
     * @param text        Source document text (UTF-8).
     * @param terms       Search terms to locate.
     * @param window_size Passage width in characters (default: Config::max_snippet_len).
     * @return            Best matching snippet, or the first @p window_size
     *                    characters of @p text when no terms are found.
     */
    std::string snippet(const std::string& text,
                        const std::vector<std::string>& terms,
                        size_t window_size = 0) const noexcept;

    // ── Static helpers (exposed for unit-testing and pipeline reuse) ──────────

    /**
     * @brief Tokenise @p text into a vector of lowercase tokens.
     *
     * Splits on whitespace and standard ASCII punctuation.  Non-ASCII bytes
     * are preserved inside tokens (multi-byte UTF-8 sequences are not split).
     *
     * @param text           Input string.
     * @param case_insensitive When true, tokens are lowercased (ASCII only).
     * @return Vector of tokens in order of appearance.
     */
    static std::vector<std::string> tokenize(const std::string& text,
                                             bool case_insensitive = true);

    /**
     * @brief Apply highlight tags at the given byte @p offsets inside @p text.
     *
     * @p offsets is a list of [start, end) byte pairs describing the byte
     * ranges to wrap.  Ranges must be non-overlapping and sorted by start.
     *
     * @param text       Source text.
     * @param offsets    Sorted, non-overlapping [start,end) byte ranges.
     * @param open_tag   Opening highlight tag.
     * @param close_tag  Closing highlight tag.
     * @return           @p text with tags inserted at the specified ranges.
     */
    static std::string applyHighlight(
        const std::string& text,
        const std::vector<std::pair<size_t, size_t>>& offsets,
        const std::string& open_tag,
        const std::string& close_tag);

    /**
     * @brief Find the byte offset that maximises term coverage in a window.
     *
     * Scans @p text with a sliding window of @p window_size bytes and returns
     * the start offset that covers the greatest number of distinct term
     * occurrences.  Ties are broken by preferring the earliest window.
     *
     * @param text        Source text.
     * @param terms       Lowercase search terms.
     * @param window_size Window width in bytes.
     * @return            Start byte offset of the best window, or 0 if @p text
     *                    is shorter than @p window_size.
     */
    static size_t bestWindowOffset(const std::string& text,
                                   const std::vector<std::string>& terms,
                                   size_t window_size);

private:
    Config config_;

    /// Return sorted, merged [start,end) match ranges for @p terms in @p text.
    std::vector<std::pair<size_t, size_t>>
    findMatchRanges(const std::string& text,
                    const std::vector<std::string>& terms) const;
};

} // namespace themis
