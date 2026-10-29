/**
 * @file test_content_fingerprinter_binary_focused.cpp
 * @brief Regression coverage for raw binary content fingerprints.
 */

#include <gtest/gtest.h>

#include "toolbox/content_fingerprinter.h"

#include <string>
#include <string_view>

namespace {

TEST(ContentFingerprinterBinaryTest, PreservesEmbeddedNulAndHighBitBytes) {
    const std::string bytes{"A\0B\xFF", 4};
    const themis::toolbox::ContentFingerprinter fingerprinter;

    const auto text_view_result =
        fingerprinter.compute(std::string_view{bytes.data(), bytes.size()});
    const auto raw_result = fingerprinter.compute(
        reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());

    EXPECT_EQ(text_view_result.byte_len, 4u);
    EXPECT_EQ(raw_result.byte_len, 4u);
    EXPECT_EQ(text_view_result.sha256_hex,
              "6c45397132bfadbb1abc09e7e14010a84d34b55ff56286333d9a17ff57e99ecc");
    EXPECT_EQ(raw_result.sha256_hex, text_view_result.sha256_hex);
}

} // namespace
