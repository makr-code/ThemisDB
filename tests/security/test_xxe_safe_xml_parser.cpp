#include "security/xxe_safe_xml_parser.h"

#include <gtest/gtest.h>

TEST(XxeSafeXmlParserTest, ParsesSimpleXml) {
    const auto result = themis::security::parseXmlSafe("<root><child>value</child></root>", "unit-test");
    ASSERT_TRUE(result.success) << result.error_message;
    ASSERT_TRUE(result.document);
    EXPECT_STREQ(result.document.first_child().name(), "root");
    EXPECT_TRUE(themis::security::validateNoExternalEntities(result.document));
}

TEST(XxeSafeXmlParserTest, RejectsDoctypeWithExternalEntity) {
    const auto result = themis::security::parseXmlSafe(
        "<?xml version=\"1.0\"?><!DOCTYPE root [<!ENTITY xxe SYSTEM \"file:///etc/passwd\">]><root>&xxe;</root>",
        "unit-test");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}