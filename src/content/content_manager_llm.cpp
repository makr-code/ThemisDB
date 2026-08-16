/**
 * @file content_manager_llm.cpp
 * @brief Core content management system orchestrating processors, validators, and storage.
 * @version 0.0.47
 * @note Maturity: 🟡 BETA
 * @note Score: 74/100
 * @note Gap Summary: total=14; TODO=3, Stub=1, Unimpl=2, Mock=0, Sim=0, Debt=2, C=1, H=4, M=7, L=0
 * @note Status: Beta; LLM integration in progress; prompt engineering and response validation under review
 * @note This block is auto-generated and will be overwritten.
 */
#include <algorithm>
#include <sstream>

#include "content/content_manager.h"
#include "llm/embedded_llm.h"
#include <stdexcept>

namespace themis {
namespace content {

/**
 * @brief Analyze content using LLM
 *
 * Generates summary, extracts key topics, determines sentiment, and classifies category.
 *
 * @param content_id Content ID to analyze
 * @return JSON with analysis results
 */
json ContentManager::analyzeContent(const std::string &content_id) {
    json result;
    result["content_id"] = content_id;
    result["success"]    = false;

    try {
        // Get content metadata
        auto meta_opt = getContentMeta(content_id);
        if (!meta_opt) {
            result["error"] = "Content not found";
            return result;
        }

        auto meta = *meta_opt;

        // Extract text from content
        std::string text;
        if (meta.text_extracted) {
            // Get extracted text
            text = getExtractedText(content_id);
        } else {
            result["error"] = "Text not extracted from content";
            return result;
        }

        if (text.empty()) {
            result["error"] = "No text content available";
            return result;
        }

        // Limit text to 4000 characters for LLM analysis
        std::string analysis_text = text.substr(0, std::min(text.size(), size_t(4000)));

        // Build comprehensive analysis prompt
        std::stringstream prompt;
        prompt << "Analyze the following content and provide:\n";
        prompt << "1. A brief summary (2-3 sentences)\n";
        prompt << "2. Key topics (3-5 topics as comma-separated list)\n";
        prompt << "3. Sentiment (positive/negative/neutral)\n";
        prompt << "4. Content category (article/technical/business/personal/other)\n\n";
        prompt << "Content:\n" << analysis_text << "\n\n";
        prompt << "Analysis:\n";

        // Generate analysis using EmbeddedLLM
        std::string analysis = THEMIS_LLM_GENERATE(prompt.str());

        if (analysis.empty()) {
            result["error"] = "LLM analysis failed";
            return result;
        }

        // Parse the analysis result
        result                = parseAnalysisResult(analysis, meta);
        result["success"]     = true;
        result["analyzed_at"] = std::chrono::system_clock::now().time_since_epoch().count();

    } catch (const std::exception &e) {
        result["error"] = std::string("Exception: ") + e.what();
    }

    return result;
}

/**
 * @brief Generate tags for content using LLM
 *
 * @param content_id Content ID
 * @param max_tags Maximum number of tags (default: 10)
 * @return Vector of generated tags
 */
std::vector<std::string> ContentManager::generateTags(const std::string &content_id, int max_tags) {
    std::vector<std::string> tags;

    try {
        // Get content text
        std::string text = getExtractedText(content_id);
        if (text.empty()) {
            return tags;
        }

        // Limit text for prompt
        std::string tag_text = text.substr(0, std::min(text.size(), size_t(2000)));

        // Build prompt
        std::stringstream prompt;
        prompt << "Generate " << max_tags << " relevant tags for the following content.\n";
        prompt << "Return only the tags as a comma-separated list.\n\n";
        prompt << "Content:\n" << tag_text << "\n\n";
        prompt << "Tags: ";

        // Generate tags using LLM
        std::string tags_text = THEMIS_LLM_GENERATE(prompt.str());

        if (!tags_text.empty()) {
            // Parse comma-separated tags
            tags = parseTags(tags_text);

            // Limit to max_tags
            if (tags.size() > static_cast<size_t>(max_tags)) {
                tags.resize(max_tags);
            }
        }

    } catch (...) {
        // Log error
    }

    return tags;
}

/**
 * @brief Summarize content using LLM
 *
 * @param content_id Content ID
 * @param max_words Maximum words in summary
 * @return Summary text
 */
std::string ContentManager::summarizeContent(const std::string &content_id, int max_words) {
    try {
        // Get content text
        std::string text = getExtractedText(content_id);
        if (text.empty()) {
            return "No content to summarize";
        }

        // Limit text for analysis
        std::string summary_text = text.substr(0, std::min(text.size(), size_t(4000)));

        // Build prompt
        std::stringstream prompt;
        prompt << "Provide a " << max_words << "-word summary of the following content:\n\n";
        prompt << summary_text << "\n\n";
        prompt << "Summary: ";

        // Generate summary using LLM
        std::string summary = THEMIS_LLM_GENERATE(prompt.str());

        if (!summary.empty()) {
            return summary;
        }

    } catch (...) {
        // Log error
    }

    return "Summary generation failed";
}

/**
 * @brief Classify content by category using LLM
 *
 * @param content_id Content ID
 * @return Category string
 */
std::string ContentManager::classifyContent(const std::string &content_id) {
    try {
        // Get content text
        std::string text = getExtractedText(content_id);
        if (text.empty()) {
            return "unknown";
        }

        // Limit text for classification
        std::string class_text = text.substr(0, std::min(text.size(), size_t(1000)));

        // Build prompt
        std::stringstream prompt;
        prompt << "Classify the following content into one category: ";
        prompt << "article, technical, business, personal, news, academic, fiction, other\n\n";
        prompt << "Content:\n" << class_text << "\n\n";
        prompt << "Category: ";

        // Classify using LLM
        std::string category = THEMIS_LLM_GENERATE(prompt.str());

        if (!category.empty()) {
            // Clean up the result (remove newlines, lowercase)
            std::transform(category.begin(), category.end(), category.begin(), ::tolower);
            category.erase(std::remove(category.begin(), category.end(), '\n'), category.end());
            category.erase(std::remove(category.begin(), category.end(), '\r'), category.end());

            return category;
        }

    } catch (...) {
        // Log error
    }

    return "unknown";
}

/**
 * @brief Extract named entities from content using LLM
 *
 * @param content_id Content ID
 * @return JSON with entities (people, places, organizations)
 */
json ContentManager::extractEntities(const std::string &content_id) {
    json result             = json::object();
    result["people"]        = json::array();
    result["places"]        = json::array();
    result["organizations"] = json::array();

    try {
        // Get content text
        std::string text = getExtractedText(content_id);
        if (text.empty()) {
            return result;
        }

        // Limit text for NER
        std::string ner_text = text.substr(0, std::min(text.size(), size_t(3000)));

        // Build prompt
        std::stringstream prompt;
        prompt << "Extract named entities from the following text.\n";
        prompt << "List separately:\n";
        prompt << "- PEOPLE: Names of people\n";
        prompt << "- PLACES: Geographic locations\n";
        prompt << "- ORGANIZATIONS: Companies, institutions\n\n";
        prompt << "Text:\n" << ner_text << "\n\n";
        prompt << "Entities:\n";

        // Extract entities using LLM
        std::string entities_text = THEMIS_LLM_GENERATE(prompt.str());

        if (!entities_text.empty()) {
            // Parse the structured output
            result = parseEntities(entities_text);
        }

    } catch (...) {
        // Log error
    }

    return result;
}

// Helper methods

/**
 * @brief Parse LLM analysis result into structured JSON
 */
json ContentManager::parseAnalysisResult(const std::string &analysis_text, const ContentMeta &meta) {
    json result;

    // Simple parser - look for keywords
    std::istringstream iss(analysis_text);
    std::string line;
    std::string current_section;
    std::stringstream current_content;

    while (std::getline(iss, line)) {
        if (line.find("Summary:") != std::string::npos || line.find("1.") != std::string::npos) {
            if (!current_section.empty()) {
                result[current_section] = current_content.str();
            }
            current_section = "summary";
            current_content.str("");
            current_content.clear();

            // Extract content after the marker
            size_t pos = line.find(':');
            if (pos != std::string::npos && pos + 1 < line.size()) {
                current_content << line.substr(pos + 1);
            }
        } else if (line.find("Topics:") != std::string::npos || line.find("2.") != std::string::npos) {
            if (!current_section.empty()) {
                result[current_section] = current_content.str();
            }
            current_section = "topics";
            current_content.str("");
            current_content.clear();

            size_t pos = line.find(':');
            if (pos != std::string::npos && pos + 1 < line.size()) {
                current_content << line.substr(pos + 1);
            }
        } else if (line.find("Sentiment:") != std::string::npos || line.find("3.") != std::string::npos) {
            if (!current_section.empty()) {
                result[current_section] = current_content.str();
            }
            current_section = "sentiment";
            current_content.str("");
            current_content.clear();

            size_t pos = line.find(':');
            if (pos != std::string::npos && pos + 1 < line.size()) {
                current_content << line.substr(pos + 1);
            }
        } else if (line.find("Category:") != std::string::npos || line.find("4.") != std::string::npos) {
            if (!current_section.empty()) {
                result[current_section] = current_content.str();
            }
            current_section = "category";
            current_content.str("");
            current_content.clear();

            size_t pos = line.find(':');
            if (pos != std::string::npos && pos + 1 < line.size()) {
                current_content << line.substr(pos + 1);
            }
        } else if (!line.empty() && !current_section.empty()) {
            current_content << " " << line;
        }
    }

    // Add last section
    if (!current_section.empty()) {
        result[current_section] = current_content.str();
    }

    // Add metadata
    result["mime_type"]         = meta.mime_type;
    result["size_bytes"]        = meta.size_bytes;
    result["original_filename"] = meta.original_filename;

    return result;
}

/**
 * @brief Parse comma-separated tags from LLM output
 */
std::vector<std::string> ContentManager::parseTags(const std::string &tags_text) {
    std::vector<std::string> tags;
    std::stringstream ss(tags_text);
    std::string tag;

    while (std::getline(ss, tag, ',')) {
        // Trim whitespace
        tag.erase(0, tag.find_first_not_of(" \t\n\r"));
        tag.erase(tag.find_last_not_of(" \t\n\r") + 1);

        if (!tag.empty() && tag.size() <= 50) { // Reasonable tag length
            tags.push_back(tag);
        }
    }

    return tags;
}

/**
 * @brief Parse entities from LLM output
 */
json ContentManager::parseEntities(const std::string &entities_text) {
    json result;
    result["people"]        = json::array();
    result["places"]        = json::array();
    result["organizations"] = json::array();

    std::istringstream iss(entities_text);
    std::string line;
    std::string current_category;

    while (std::getline(iss, line)) {
        if (line.find("PEOPLE:") != std::string::npos) {
            current_category = "people";
        } else if (line.find("PLACES:") != std::string::npos) {
            current_category = "places";
        } else if (line.find("ORGANIZATIONS:") != std::string::npos) {
            current_category = "organizations";
        } else if (!line.empty() && !current_category.empty()) {
            // Remove markers like "- " or "* "
            if (line.find("- ") == 0) {
                line = line.substr(2);
            }
            if (line.find("* ") == 0) {
                line = line.substr(2);
            }

            // Trim
            line.erase(0, line.find_first_not_of(" \t\n\r"));
            line.erase(line.find_last_not_of(" \t\n\r") + 1);

            if (!line.empty()) {
                result[current_category].push_back(line);
            }
        }
    }

    return result;
}

/**
 * @brief Get extracted text from content
 */
std::string ContentManager::getExtractedText(const std::string &content_id) {
    const auto chunks = getContentChunks(content_id);
    std::string result;
    result.reserve(chunks.size() * 256);
    for (const auto &chunk : chunks) {
        result += chunk.text;
    }
    return result;
}

} // namespace content
} // namespace themis

