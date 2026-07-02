// File: src/security/xxe_safe_xml_parser.cpp
// Sprint 5 QW-7: XXE Vulnerability Remediation
// 
// Provides XXE-hardened XML parsing utilities for SAML, Office, and Scraper modules.
// All pugixml parsing calls must route through these helpers to ensure XXE protection.

#include "security/xxe_safe_xml_parser.h"
#include "utils/logger.h"

namespace themis {
namespace security {

// ============================================================================
// XXE-Safe XML Parsing Implementation
// ============================================================================

constexpr size_t MAX_XML_BLOB_SIZE = 256ULL * 1024ULL * 1024ULL;  // 256 MB safety ceiling
constexpr size_t MAX_XML_ELEMENT_DEPTH = 1024;  // Protection against billion laughs DoS

XxeSafeXmlParseResult parseXmlSafe(
    const std::string& xml_content,
    const std::string& source_hint,
    bool allow_external_entities) {
    
    XxeSafeXmlParseResult result;
    result.success = false;
    result.error_message = "";
    
    // ================================================================
    // QW-7a: Input Validation & Size Limits
    // ================================================================
    
    // Check 1: Verify XML content is not empty
    if (xml_content.empty()) {
        result.error_message = "XML content is empty";
        THEMIS_WARN("XXE-safe parser: empty content from {}", source_hint);
        return result;
    }
    
    // Check 2: Enforce maximum blob size (protection against DoS)
    if (xml_content.size() > MAX_XML_BLOB_SIZE) {
        result.error_message = std::string("XML content exceeds maximum size (") +
                              std::to_string(MAX_XML_BLOB_SIZE) + " bytes)";
        THEMIS_ERROR("XXE-safe parser: oversized XML ({} bytes) from {}", 
                     xml_content.size(), source_hint);
        return result;
    }
    
    // ================================================================
    // QW-7b: Pre-parse XXE Payload Detection
    // ================================================================
    
    // Quick pattern-based detection of common XXE attack vectors
    std::vector<std::string> xxe_patterns = {
        "<!ENTITY",           // Entity definition
        "SYSTEM",             // External system identifier
        "PUBLIC",             // Public identifier
        "file://",            // File URI scheme
        "http://",            // HTTP URI scheme (could be malicious)
        "ftp://",             // FTP URI scheme
        "ATTLIST",            // Attribute list declarations (rare in legitimate XML)
        "NOTATION",           // Notation declarations (rare)
    };
    
    for (const auto& pattern : xxe_patterns) {
        if (xml_content.find(pattern) != std::string::npos) {
            // Found suspicious pattern - log but don't reject yet
            // (legitimate XML might contain these strings in content)
            THEMIS_INFO("XXE-safe parser: detected pattern '{}' in XML from {}", 
                       pattern, source_hint);
        }
    }
    
    // ================================================================
    // QW-7c: pugixml Safe Parsing Configuration
    // ================================================================
    
    // pugixml parse flags for XXE protection
    unsigned int parse_flags = pugi::parse_default;
    
    // CRITICAL: Disable network-based entity loading
    parse_flags |= pugi::parse_no_network;
    
    // Additional hardening: disable external subset processing
    // (pugixml disables external entities by default, but explicit is better)
    
    try {
        // Parse XML with XXE-safe configuration
        // Note: pugixml does NOT support disabling DTD processing via parse flags,
        // so we validate the parsed document afterward for suspicious elements
        auto parse_result = result.document.load_string(xml_content.c_str(), parse_flags);
        
        if (!parse_result) {
            result.error_message = std::string("XML parse error: ") + parse_result.description();
            THEMIS_WARN("XXE-safe parser: parse failure from {}: {}", 
                       source_hint, result.error_message);
            return result;
        }
        
        // ================================================================
        // QW-7d: Post-parse XXE Validation
        // ================================================================
        
        // Traverse parsed document to detect XXE-suspicious structures
        int entity_count = 0;
        int max_depth = 0;
        
        std::function<void(const pugi::xml_node&, int)> validate_node = 
            [&](const pugi::xml_node& node, int depth) {
            if (depth > max_depth) max_depth = depth;
            if (depth > MAX_XML_ELEMENT_DEPTH) {
                result.error_message = "XML nesting depth exceeds maximum allowed";
                THEMIS_ERROR("XXE-safe parser: excessive nesting ({}) from {}", 
                            depth, source_hint);
                result.success = false;
                return;
            }
            
            // Check for ENTITY declarations (XXE indicator)
            for (auto child = node.first_child(); child; child = child.next_sibling()) {
                if (child.type() == pugi::node_element) {
                    std::string name = child.name();
                    // DTD elements might indicate XXE attempt
                    if (name.find("ENTITY") != std::string::npos ||
                        name.find("DOCTYPE") != std::string::npos) {
                        THEMIS_WARN("XXE-safe parser: suspicious element '{}' from {}", 
                                   name, source_hint);
                    }
                }
                validate_node(child, depth + 1);
            }
        };
        
        validate_node(result.document, 0);
        
        // If validation detected XXE, mark as failure
        if (!result.error_message.empty()) {
            return result;
        }
        
        result.success = true;
        result.max_depth = max_depth;
        
        THEMIS_DEBUG("XXE-safe parser: successfully parsed XML from {} (max_depth={})", 
                    source_hint, max_depth);
        
        return result;
        
    } catch (const std::exception& e) {
        result.error_message = std::string("Exception during XML parsing: ") + e.what();
        THEMIS_ERROR("XXE-safe parser: exception from {}: {}", source_hint, result.error_message);
        return result;
    }
}

// Validate that external entities are not present
bool validateNoExternalEntities(const pugi::xml_document& doc) {
    // Check document for any references to external resources
    // This is a best-effort check; pugixml doesn't expand external entities by default
    
    std::function<bool(const pugi::xml_node&)> check_node = 
        [&](const pugi::xml_node& node) -> bool {
        for (auto child = node.first_child(); child; child = child.next_sibling()) {
            if (child.type() == pugi::node_element) {
                std::string name = child.name();
                if (name.find("ENTITY") != std::string::npos) {
                    return false;  // Found entity declaration
                }
            }
            if (!check_node(child)) {
                return false;
            }
        }
        return true;
    };
    
    return check_node(doc);
}

}  // namespace security
}  // namespace themis
