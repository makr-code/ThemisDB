/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            search_highlighter.h                               ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace themis {

/**
 * @brief Highlight matched query terms in document text and extract context snippets.
 *
 * `SearchHighlighter` provides two complementary operations for presenting
 * search results to end users:
 *
 * 1. **Highlight** – wraps every occurrence of a query term in @p text with
 *    configurable open/close markup tags (default `<em>` / `</em>`).
 *
 * 2. **Snippet** – extracts a short excerpt of @p text (≤ `windowSize` chars)
 *    centred on the densest cluster of query-term matches and returns it with
 *    the terms highlighted.  When the text is truncated a configurable
 *    separator (default `...`) is prepended and/or appended.
 *
 * Both operations are case-insensitive and preserve the original capitalisation
 * of the source text.
 *
 * Example:
 * ```cpp
 * SearchHighlighter h;
 * // Full-text highlight
 * auto lit = h.highlight("Machine Learning is great", {"machine", "learning"});
 * // lit == "<em>Machine</em> <em>Learning</em> is great"
 *
 * // Snippet extraction
 * std::string longDoc = std::string(300, 'a') + " neural networks " + std::string(300, 'z');
 * auto snip = h.snippet(longDoc, {"neural"}, 80);
 * // snip == "...<em>neural</em> networks..."
 * ```
 *
 * @note Thread Safety: `SearchHighlighter` is stateless; all methods are
 *   `const` and can be called concurrently from multiple threads.
 *
 * @note Exception Safety: `highlight()` and `snippet()` never throw.  Any
 *   invalid input (empty text, empty terms) is handled gracefully by
 *   returning the original text or an empty string.
 *
 * v2.1.0 Feature: initial delivery (Issue #2457).
 */
class SearchHighlighter {
public:
    /**
     * @brief Configuration for highlight and snippet rendering.
     */
    struct Config {
        /// Opening markup tag inserted before each matched term.  Default `<em>`.
        std::string open_tag = "<em>";
        /// Closing markup tag inserted after each matched term.  Default `</em>`.
        std::string close_tag = "</em>";
        /// String prepended/appended when the snippet is truncated.  Default `...`.
        std::string separator = "...";
        /// Maximum number of characters in an extracted snippet.  Default 200.
        size_t window_size = 200;
        /// Hard upper bound on window_size; value is clamped at construction.
        size_t max_window_size = 10'000;
    };

    /**
     * @brief Construct with default configuration.
     */
    SearchHighlighter() = default;

    /**
     * @brief Construct with custom configuration.
     *
     * @throws std::invalid_argument if window_size is 0 or exceeds max_window_size.
     */
    explicit SearchHighlighter(const Config& config);

    /**
     * @brief Return the current configuration.
     */
    const Config& getConfig() const { return config_; }

    /**
     * @brief Replace the current configuration.
     *
     * @throws std::invalid_argument if the new config is invalid.
     */
    void setConfig(const Config& config);

    // -----------------------------------------------------------------------
    // Core operations
    // -----------------------------------------------------------------------

    /**
     * @brief Wrap every occurrence of a query term in @p text with markup tags.
     *
     * Matching is case-insensitive; original capitalisation is preserved in the
     * output.  Only whole-word (alnum-boundary) matches are highlighted.
     *
     * @param text   Source document text.
     * @param terms  Set of lower-case query terms to highlight.
     * @return       Annotated copy of @p text; empty string if @p text is empty.
     */
    std::string highlight(const std::string& text,
                          const std::unordered_set<std::string>& terms) const;

    /**
     * @brief Convenience overload: tokenise @p query and highlight its terms.
     *
     * The query string is split on non-alnum characters and each token is
     * lower-cased before being used as a search term.
     *
     * @param text   Source document text.
     * @param query  Raw query string (e.g. `"machine learning"`).
     */
    std::string highlight(const std::string& text,
                          const std::string& query) const;

    /**
     * @brief Overload accepting a pre-split term list.
     *
     * @param text   Source document text.
     * @param terms  List of query terms (may be mixed-case; converted internally).
     */
    std::string highlight(const std::string& text,
                          const std::vector<std::string>& terms) const;

    /**
     * @brief Extract a context snippet centred on the densest term cluster.
     *
     * If the text fits within @p window_size (or `Config::window_size` when
     * `window_size == 0`), the entire highlighted text is returned without
     * separators.  Otherwise a window of up to `window_size` characters is
     * extracted from the position with the highest term density, and
     * `Config::separator` is prepended/appended where the text was cut.
     *
     * @param text        Source document text.
     * @param terms       Set of lower-case query terms.
     * @param window_size Override for Config::window_size (0 = use config value).
     * @return            Short highlighted excerpt; empty string if text is empty.
     */
    std::string snippet(const std::string& text,
                        const std::unordered_set<std::string>& terms,
                        size_t window_size = 0) const;

    /**
     * @brief Convenience overload: tokenise @p query and extract a snippet.
     */
    std::string snippet(const std::string& text,
                        const std::string& query,
                        size_t window_size = 0) const;

    /**
     * @brief Overload accepting a pre-split term list.
     */
    std::string snippet(const std::string& text,
                        const std::vector<std::string>& terms,
                        size_t window_size = 0) const;

    // -----------------------------------------------------------------------
    // Static helpers (directly unit-testable)
    // -----------------------------------------------------------------------

    /**
     * @brief Tokenise a query string into lower-case terms.
     *
     * Splits on non-alnum characters, lower-cases each token, and removes
     * duplicates and empty strings.
     *
     * @param query  Raw query string.
     * @return       Set of unique lower-case terms.
     */
    static std::unordered_set<std::string> tokenize(const std::string& query);

    /**
     * @brief Apply open/close tag wrapping around each term match in @p text.
     *
     * Pure function; does not use instance configuration.
     *
     * @param text      Source text (original capitalisation preserved).
     * @param terms     Lower-case terms to match.
     * @param open_tag  Tag to insert before each match.
     * @param close_tag Tag to insert after each match.
     * @return          Annotated text.
     */
    static std::string applyHighlight(const std::string& text,
                                      const std::unordered_set<std::string>& terms,
                                      const std::string& open_tag,
                                      const std::string& close_tag);

    /**
     * @brief Find the byte offset of the best snippet window.
     *
     * Locates the window of @p window_size bytes within @p lower_text that
     * contains the greatest number of term occurrences.  Returns 0 when no
     * match is found or the full text fits within the window.
     *
     * @param lower_text  Lower-case copy of the source text for scanning.
     * @param terms       Lower-case terms to locate.
     * @param window_size Window width in bytes.
     * @return            Start offset of the best window.
     */
    static size_t bestWindowOffset(const std::string& lower_text,
                                   const std::unordered_set<std::string>& terms,
                                   size_t window_size);

private:
    Config config_;

    /// Convert a vector of (potentially mixed-case) terms to a lower-case set.
    static std::unordered_set<std::string> toLowerSet(
        const std::vector<std::string>& terms);
};

} // namespace themis
