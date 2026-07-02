// File: include/security/xxe_safe_xml_parser.h
// Sprint 5 QW-7: XXE Vulnerability Remediation
//
// Provides XXE-hardened XML parsing utilities for SAML, Office, and Scraper modules.
// All pugixml parsing calls must route through these helpers to ensure XXE protection.

#pragma once

#include <pugixml.hpp>
#include <string>
#include <functional>

namespace themis {
namespace security {

/**
 * @struct XxeSafeXmlParseResult
 * @brief Result of XXE-safe XML parsing operation.
 * 
 * Encapsulates the parsed XML document, success status, and any error messages
 * from the XXE-hardened parsing process.
 * 
 * XXE Protection Features:
 * - Size limits (256 MB maximum blob)
 * - Nesting depth limits (1024 levels)
 * - Pre-parse XXE pattern detection
 * - Post-parse XXE validation
 * - Network access disabled via pugixml flags
 * 
 * @section Usage
 * ```cpp
 * XxeSafeXmlParseResult result = parseXmlSafe(xml_content, "SAML Response");
 * if (!result.success) {
 *     // Handle error: result.error_message contains details
 * } else {
 *     // Use result.document safely
 *     auto root = result.document.first_child();
 * }
 * ```
 */
struct XxeSafeXmlParseResult {
    /// Flag indicating successful parse with XXE validation passed
    bool success = false;
    
    /// Parsed XML document (only valid if success == true)
    pugi::xml_document document;
    
    /// Human-readable error message (populated if success == false)
    std::string error_message;
    
    /// Maximum nesting depth encountered during parsing
    int max_depth = 0;
    
    /// Constructor
    XxeSafeXmlParseResult() : success(false), max_depth(0) {}
};

/**
 * @brief Parse XML content with comprehensive XXE protection.
 * 
 * Implements multi-layer XXE defense:
 * 1. Input validation (size limits, emptiness check)
 * 2. Pre-parse XXE pattern detection (ENTITY, SYSTEM, file://, etc.)
 * 3. Safe pugixml configuration (parse_no_network flag)
 * 4. Post-parse XXE validation (nesting depth, suspicious elements)
 * 
 * @param xml_content        Raw XML content to parse
 * @param source_hint        Human-readable hint for logging (e.g., "SAML Response", "document.xml")
 * @param allow_external_entities  Reserved for future use; currently ignored (always disables)
 * 
 * @return XxeSafeXmlParseResult containing parsed document or error details
 * 
 * @section Error Handling
 * - Empty XML: returns error "XML content is empty"
 * - Oversized XML: returns error "XML content exceeds maximum size"
 * - Parse failure: returns error from pugixml parse result
 * - XXE detection: returns error "XML nesting depth exceeds maximum allowed"
 * 
 * @section Security Notes
 * - Maximum blob size: 256 MB
 * - Maximum nesting depth: 1024 levels
 * - Network access: always disabled (pugixml parse_no_network flag)
 * - External entities: validated post-parse for suspicious patterns
 * 
 * @section Performance
 * - O(n) where n = size of XML content (single parse + traversal)
 * - XXE pattern detection adds minimal overhead (string searches)
 * - Suitable for high-volume parsing (SAML assertions, Office documents, web scraping)
 * 
 * @see validateNoExternalEntities()
 */
XxeSafeXmlParseResult parseXmlSafe(
    const std::string& xml_content,
    const std::string& source_hint,
    bool allow_external_entities = false);

/**
 * @brief Validate that a parsed XML document contains no external entity references.
 * 
 * This is a best-effort check; pugixml does not expand external entities by default.
 * However, this function validates that the parsed document doesn't contain entity
 * declarations that could indicate an XXE attack attempt.
 * 
 * @param doc  Parsed XML document to validate
 * 
 * @return true if no suspicious entity declarations found; false otherwise
 * 
 * @section Implementation Notes
 * - Traverses the entire DOM tree (O(n) where n = number of elements)
 * - Checks for ENTITY and DOCTYPE elements
 * - Does not validate attribute content (assumes pugixml handles this)
 */
bool validateNoExternalEntities(const pugi::xml_document& doc);

}  // namespace security
}  // namespace themis
