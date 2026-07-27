// ThemisDB XXE-safe XML parsing utilities

#pragma once

#include <pugixml.hpp>

#include <string>

namespace themis {
namespace security {

/**
 * @brief Result of an XXE-hardened XML parse.
 *
 * The document member is only valid when success is true.
 */
struct XxeSafeXmlParseResult {
    bool success = false;
    pugi::xml_document document;
    std::string error_message;
    int max_depth = 0;
};

/**
 * @brief Parse XML content with basic XXE hardening.
 *
 * The parser rejects common external-entity and DTD attack markers before
 * invoking pugixml.
 *
 * @param xml_content XML payload to parse.
 * @param source_hint Short identifier used in diagnostics.
 * @param allow_external_entities Reserved for future compatibility; currently ignored.
 * @return Parse result with document or error details.
 */
XxeSafeXmlParseResult parseXmlSafe(const std::string& xml_content,
                                   const std::string& source_hint,
                                   bool allow_external_entities = false);

/**
 * @brief Best-effort validation that no external entity declarations remain.
 *
 * @param doc Parsed XML document.
 * @return true when no suspicious nodes are detected.
 */
bool validateNoExternalEntities(const pugi::xml_document& doc);

} // namespace security
} // namespace themis