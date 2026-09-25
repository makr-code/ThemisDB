/**
 * @file xxe_safe_xml_parser.cpp
 * @brief XXE-safe XML parser implementation.
 *
 * Implements the hardened XML parsing routines declared in
 * xxe_safe_xml_parser.h with all dangerous XML features disabled.
 */

// ThemisDB XXE-safe XML parsing utilities

#include "security/xxe_safe_xml_parser.h"

#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <functional>

namespace themis {
namespace security {

namespace {

constexpr std::size_t kMaxXmlBlobSize = 256 * 1024 * 1024;
constexpr int kMaxXmlDepth = 1024;

/**
 * @brief To Lower Ascii.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: std::transform(), begin(), end(), std::tolower().
 */
std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

} // namespace

/**
 * @brief Parse Xml Safe.
 * @param[in] xml_content Input parameter.
 * @param[in] source_hint Input parameter.
 * @param[in] bool Input parameter.
 * @return Return value.
 * @details Calls: empty(), THEMIS_WARN(), size(), THEMIS_ERROR(), toLowerAscii(), find(), load_string(), c_str().
 */
XxeSafeXmlParseResult parseXmlSafe(const std::string& xml_content,
                                   const std::string& source_hint,
                                   bool /*allow_external_entities*/) {
    XxeSafeXmlParseResult result = {};

    if (xml_content.empty()) {
        result.error_message = "XML content is empty";
        THEMIS_WARN("XXE-safe parser: empty XML from {}", source_hint);
        return result;
    }

    if (xml_content.size() > kMaxXmlBlobSize) {
        result.error_message = "XML content exceeds maximum size";
        THEMIS_ERROR("XXE-safe parser: oversized XML ({} bytes) from {}",xml_content.size(), source_hint);
        return result;
    }

    const std::string lowered = toLowerAscii(xml_content);
    for (const std::string marker : {"<!doctype", "<!entity", "file://"}) {
        if (lowered.find(marker) != std::string::npos) {
            result.error_message = "XML content contains a disallowed external-entity marker";
            THEMIS_WARN("XXE-safe parser: rejected {} due to marker '{}'", source_hint, marker);
            return result;
        }
    }

    auto parse_result = result.document.load_string(xml_content.c_str(), pugi::parse_default);
    if (!parse_result) {
        result.error_message = std::string("XML parse error: ") + parse_result.description();
        THEMIS_WARN("XXE-safe parser: parse failure from {}: {}", source_hint, result.error_message);
        return result;
    }

    int max_depth = 0;
    std::function<void(const pugi::xml_node&, int)> visit = [&](const pugi::xml_node& node, int depth) {
        if (!result.error_message.empty()) {
            return;
        }

        max_depth = std::max(max_depth, depth);
        if (depth > kMaxXmlDepth) {
            result.error_message = "XML nesting depth exceeds maximum allowed";
            THEMIS_ERROR("XXE-safe parser: excessive nesting ({}) from {}", depth, source_hint);
            return;
        }

        for (auto child : node.children()) {
            if (child.type() == pugi::node_doctype) {
                result.error_message = "XML document contains a DOCTYPE declaration";
                THEMIS_WARN("XXE-safe parser: DOCTYPE rejected from {}", source_hint);
                return;
            }
            visit(child, depth + 1);
        }
    };

    visit(result.document, 0);
    if (!result.error_message.empty()) {
        return result;
    }

    result.success = true;
    result.max_depth = max_depth;
    return result;
}

/**
 * @brief Validate No External Entities.
 * @param[in] doc Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: children(), type().
 */
bool validateNoExternalEntities(const pugi::xml_document& doc) {
    for (auto child : doc.children()) {
        if (child.type() == pugi::node_doctype) {
            return false;
        }
    }
    return true;
}

} // namespace security
} // namespace themis