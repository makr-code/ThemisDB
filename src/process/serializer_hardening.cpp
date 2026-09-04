// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file serializer_hardening.cpp
 * @brief Implementation of serializer hardening utilities.
 */

#include "process/serializer_hardening.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <sstream>

namespace themis::process {

// ─────────────────────────────────────────────────────────────────────────────
// SerializerInputValidator implementation
// ─────────────────────────────────────────────────────────────────────────────

SerializerValidationResult SerializerInputValidator::validateInput(
    std::string_view input,
    std::string_view format_name
) {
    // Check empty input
    if (input.empty()) {
        return SerializerValidationResult::failure(
            "Input is empty",
            DiagnosticIncidentType::MALFORMED_INPUT_INCIDENT
        );
    }

    // Check size limit
    if (!isInputSizeValid(input.size())) {
        std::ostringstream oss;
        oss << format_name << " input size (" << input.size()
            << " bytes) exceeds maximum (" << kMaxModelInputBytes << " bytes)";
        return SerializerValidationResult::failure(
            oss.str(),
            DiagnosticIncidentType::RESOURCE_INCIDENT
        );
    }

    // Check UTF-8 validity (for text formats)
    if (!isValidUtf8(input)) {
        return SerializerValidationResult::failure(
            "Input contains invalid UTF-8 sequences",
            DiagnosticIncidentType::MALFORMED_INPUT_INCIDENT
        );
    }

    // XML-specific truncation check
    if (input.find("<?xml") != std::string_view::npos ||
        input.find("<bpmn") != std::string_view::npos ||
        input.find("<process") != std::string_view::npos) {
        if (isXmlTruncated(input)) {
            return SerializerValidationResult::failure(
                "Input appears truncated (unmatched XML tags)",
                DiagnosticIncidentType::MALFORMED_INPUT_INCIDENT
            );
        }
    }

    return SerializerValidationResult::success();
}

bool SerializerInputValidator::isXmlTruncated(std::string_view xml) {
    auto [open_count, close_count] = countXmlTags(xml);
    // Account for self-closing tags: each self-closing tag has both an open
    // count contribution and a close count contribution
    return open_count != close_count;
}

bool SerializerInputValidator::isValidUtf8(std::string_view s) {
    const unsigned char* data = reinterpret_cast<const unsigned char*>(s.data());
    size_t pos = 0;
    size_t len = s.size();

    while (pos < len) {
        unsigned char byte = data[pos];

        // ASCII range (0x00 – 0x7F)
        if (byte <= 0x7F) {
            // Check for ASCII control characters (except tab, newline, carriage return)
            if (byte < 0x09 || (byte > 0x0D && byte < 0x20) || byte == 0x7F) {
                // Allow some control chars in XML: tab (0x09), LF (0x0A), CR (0x0D)
                if (byte != 0x09 && byte != 0x0A && byte != 0x0D) {
                    return false;
                }
            }
            pos++;
            continue;
        }

        // Multi-byte sequence
        size_t seq_len = 0;
        if (!isValidUtf8Sequence(data + pos, len - pos, seq_len)) {
            return false;
        }
        pos += seq_len;
    }

    return true;
}

bool SerializerInputValidator::isValidUtf8Sequence(
    const unsigned char* data,
    size_t remaining_bytes,
    size_t& sequence_length
) {
    unsigned char byte = *data;

    // 2-byte sequence: 110xxxxx 10xxxxxx
    if ((byte & 0xE0) == 0xC0) {
        if (remaining_bytes < 2) {
          return false;
        }
        if ((data[1] & 0xC0) != 0x80) {
          return false;
        }
        sequence_length = 2;
        return true;
    }

    // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
    if ((byte & 0xF0) == 0xE0) {
        if (remaining_bytes < 3) {
          return false;
        }
        if ((data[1] & 0xC0) != 0x80) {
          return false;
        }
        if ((data[2] & 0xC0) != 0x80) {
          return false;
        }
        sequence_length = 3;
        return true;
    }

    // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    if ((byte & 0xF8) == 0xF0) {
        if (remaining_bytes < 4) {
          return false;
        }
        if ((data[1] & 0xC0) != 0x80) {
          return false;
        }
        if ((data[2] & 0xC0) != 0x80) {
          return false;
        }
        if ((data[3] & 0xC0) != 0x80) {
          return false;
        }
        sequence_length = 4;
        return true;
    }

    // Invalid start byte
    return false;
}

std::string SerializerInputValidator::extractXmlVersion(std::string_view xml) {
    // Look for version in XML declaration: <?xml version="X.Y"?>
    size_t decl_start = xml.find("<?xml");
    if (decl_start == std::string_view::npos) {
        return "";
    }

    size_t decl_end = xml.find("?>", decl_start);
    if (decl_end == std::string_view::npos) {
        return "";
    }

    std::string_view decl = xml.substr(decl_start, decl_end - decl_start + 2);

    // Look for version="X.Y" or version='X.Y'
    size_t ver_pos = decl.find("version");
    if (ver_pos == std::string_view::npos) {
        return "";
    }

    size_t eq_pos = decl.find('=', ver_pos);
    if (eq_pos == std::string_view::npos) {
        return "";
    }

    // Skip whitespace and quotes
    size_t val_start = decl.find_first_of("\"'", eq_pos);
    if (val_start == std::string_view::npos) {
        return "";
    }

    char quote = decl[val_start];
    size_t val_end = decl.find(quote, val_start + 1);
    if (val_end == std::string_view::npos) {
        return "";
    }

    return std::string(decl.substr(val_start + 1, val_end - val_start - 1));
}

std::pair<int32_t, int32_t> SerializerInputValidator::countXmlTags(std::string_view xml) {
    int32_t open_count = 0;
    int32_t close_count = 0;

    size_t pos = 0;
    while (pos < xml.size()) {
        size_t tag_start = xml.find('<', pos);
        if (tag_start == std::string_view::npos) {
            break;
        }

        size_t tag_end = xml.find('>', tag_start);
        if (tag_end == std::string_view::npos) {
            // Unclosed tag – count as truncation indicator
            break;
        }

        std::string_view tag = xml.substr(tag_start, tag_end - tag_start + 1);

        // Skip comments, processing instructions, DOCTYPE
        if (tag.find("<!--") != std::string_view::npos ||
            tag.find("<?") != std::string_view::npos ||
            tag.find("<!") != std::string_view::npos) {
            pos = tag_end + 1;
            continue;
        }

        // Check for closing tag
        if (tag[1] == '/') {
            close_count++;
        } else if (tag[tag.size() - 2] != '/') {
            // Not a self-closing tag
            open_count++;
        } else {
            // Self-closing tag (e.g., <element />)
            // Don't count toward open/close
        }

        pos = tag_end + 1;
    }

    return {open_count, close_count};
}

bool SerializerInputValidator::isAsciiControlChar(unsigned char c) {
    return c < 0x20 && c != 0x09 && c != 0x0A && c != 0x0D;
}

// ─────────────────────────────────────────────────────────────────────────────
// ParserStateTracker implementation
// ─────────────────────────────────────────────────────────────────────────────

ParserStateTracker::ParserStateTracker(
    int32_t max_depth,
    int32_t max_elements,
    int64_t timeout_ms
)
    : max_depth_(max_depth),
      max_elements_(max_elements),
      timeout_ms_(timeout_ms),
      start_time_ms_(nowMs())
{
}

bool ParserStateTracker::enterScope() {
    if (current_depth_ >= max_depth_) {
        return false;
    }
    current_depth_++;
    return true;
}

bool ParserStateTracker::exitScope() {
    if (current_depth_ == 0) {
        return false;  // Underflow
    }
    current_depth_--;
    return true;
}

bool ParserStateTracker::recordElement() {
    if (element_count_ >= max_elements_) {
        return false;
    }
    element_count_++;
    return true;
}

bool ParserStateTracker::hasTimedOut() const {
    return hasOperationTimedOut(start_time_ms_);
}

int64_t ParserStateTracker::getElapsedMs() const {
    return nowMs() - start_time_ms_;
}

std::string ParserStateTracker::getDiagnosticMessage() const {
    std::ostringstream oss;
    oss << "Parser state: depth=" << current_depth_ << "/" << max_depth_
        << ", elements=" << element_count_ << "/" << max_elements_
        << ", elapsed=" << getElapsedMs() << "ms/" << timeout_ms_ << "ms";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Format-specific validators
// ─────────────────────────────────────────────────────────────────────────────

SerializerValidationResult BpmnValidator::validateBpmnConstraints(
    const std::vector<std::string>& element_ids
) {
    if (element_ids.empty()) {
        return SerializerValidationResult::failure(
            "BPMN process has no elements"
        );
    }

    // Check for duplicate IDs
    std::set<std::string> seen = {};

    for (const auto& id : element_ids) {
        if (id.empty()) {
            return SerializerValidationResult::failure(
                "BPMN element has empty ID"
            );
        }
        if (seen.count(id) > 0) {
            return SerializerValidationResult::failure(
                "Duplicate BPMN element ID: " + id
            );
        }
        seen.insert(id);
    }

    return SerializerValidationResult::success();
}

SerializerValidationResult EpkValidator::validateEpkConstraints(
    const std::vector<std::string>& nodes,
    const std::vector<std::pair<std::string, std::string>>& edges
) {
    if (nodes.empty()) {
        return SerializerValidationResult::failure(
            "EPK process has no nodes"
        );
    }

    // Build node set for edge validation
    std::set<std::string> node_set(nodes.begin(), nodes.end());

    // Check for duplicate node IDs
    if (node_set.size() != nodes.size()) {
        return SerializerValidationResult::failure(
            "EPK contains duplicate node IDs"
        );
    }

    // Validate all edges reference existing nodes
    for (const auto& [source, target] : edges) {
        if (node_set.count(source) == 0) {
            return SerializerValidationResult::failure(
                "EPK edge references non-existent source node: " + source
            );
        }
        if (node_set.count(target) == 0) {
            return SerializerValidationResult::failure(
                "EPK edge references non-existent target node: " + target
            );
        }
    }

    return SerializerValidationResult::success();
}

SerializerValidationResult CmmnValidator::validateCmmnConstraints(
    std::string_view case_id,
    const std::vector<std::string>& item_ids
) {
    if (case_id.empty()) {
        return SerializerValidationResult::failure(
            "CMMN case model has empty ID"
        );
    }

    if (item_ids.empty()) {
        return SerializerValidationResult::failure(
            "CMMN case has no plan items"
        );
    }

    // Check for duplicate item IDs
    std::set<std::string> seen = {};

    for (const auto& id : item_ids) {
        if (id.empty()) {
            return SerializerValidationResult::failure(
                "CMMN plan item has empty ID"
            );
        }
        if (seen.count(id) > 0) {
            return SerializerValidationResult::failure(
                "Duplicate CMMN plan item ID: " + id
            );
        }
        seen.insert(id);
    }

    return SerializerValidationResult::success();
}

SerializerValidationResult DmnValidator::validateDmnConstraints(
    std::string_view decision_id,
    int32_t rule_count
) {
    if (decision_id.empty()) {
        return SerializerValidationResult::failure(
            "DMN decision model has empty ID"
        );
    }

    if (rule_count <= 0) {
        return SerializerValidationResult::failure(
            "DMN decision table must have at least one rule"
        );
    }

    return SerializerValidationResult::success();
}

} // namespace themis::process
