/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            markdown_utils.h                                   ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-18 17:54:00                                ║
  Author:          Copilot Agent                                      ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   N/A (New)                                      ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
  Purpose: Centralized markdown processing utilities (Phase 1 consolidation)
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <regex>

namespace themis::prompt_engineering {

/**
 * @brief Strip markdown code fence markers from text.
 * 
 * Removes outer code-fence delimiters (``` or ```language). Handles both
 * Unix (LF) and Windows (CRLF) line endings automatically.
 * 
 * Examples:
 * - Input:  "```\nselect * from foo\n```"
 *   Output: "select * from foo"
 * - Input:  "```json\n{\"a\": 1}\n```"
 *   Output: "{\"a\": 1}"
 * 
 * @param text Input text possibly wrapped in markdown code fences.
 * @param language_tag Optional output parameter filled with language tag if present
 *                     (e.g., "json", "aql", "sql"). Empty string if no tag.
 * @return Text with fences removed and trimmed of leading/trailing whitespace.
 *         Returns empty string if input contains only fence markers.
 */
std::string stripMarkdownFences(const std::string& text,
                                std::string* language_tag = nullptr);

/**
 * @brief Strip markdown code fences and remove line comments.
 * 
 * Combines fence stripping with removal of C++-style line comments (// ...).
 * Useful for processing LLM-generated code that may include explanatory comments.
 * 
 * @param text Input text (possibly wrapped in fences, possibly with comments).
 * @return Cleaned text with fences and comments removed.
 */
std::string stripMarkdownAndComments(const std::string& text);

/**
 * @brief Check if text appears to be wrapped in markdown code fences.
 * 
 * @param text Text to check.
 * @return true if text starts with ``` and ends with ```, false otherwise.
 */
bool isWrappedInMarkdownFences(std::string_view text);

} // namespace themis::prompt_engineering
