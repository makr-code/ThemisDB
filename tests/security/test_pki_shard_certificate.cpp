#include <gtest/gtest.h>
#include "sharding/pki_shard_certificate.h"
#include <fstream>

using namespace themis::sharding;

namespace {
constexpr const char* kValidNotBefore = "Jan  1 00:00:00 2024 GMT";
constexpr const char* kValidNotAfter = "Jan  1 00:00:00 2099 GMT";
constexpr const char* kCertificateWithSANs = R"(-----BEGIN CERTIFICATE-----
MIIB5zCCAYygAwIBAgIUex2xbCGLWo1rtrfEFKmwPibM244wCgYIKoZIzj0EAwIw
ITEfMB0GA1UEAwwWc2hhcmQtMDAxLmV4YW1wbGUudGVzdDAeFw0yNjExMjQxODI1
NThaFw0yNjExMjUxODI1NThaMCExHzAdBgNVBAMMFnNoYXJkLTAwMS5leGFtcGxl
LnRlc3QwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAARCGVwbKdSnJ/k1hjkkhH2q
GWgMso0UCML/OpFKAzIYQwccusBmsTpHoUJLrUP6chrC2okV3sp+UXjTji10CYwD
o4GhMIGeMB0GA1UdDgQWBBRtCyjz7F72XPr4kjLxsQsKEDuwNDAfBgNVHSMEGDAW
gBRtCyjz7F72XPr4kjLxsQsKEDuwNDAPBgNVHRMBAf8EBTADAQH/MEsGA1UdEQRE
MEKCEnNoYXJkLmV4YW1wbGUudGVzdIcEwAACgocQIAENuAAAAAAAAAAAAAAAAYYU
dXJuOnRoZW1pczpzaGFyZDowMDEwCgYIKoZIzj0EAwIDSQAwRgIhANlh/4MazZ8S
RoKLsDNAhq+Q6DhGxn/bJuc934m+LRR8AiEAoj3crSAKm9Q63gnt4Fkp8mldGX6W
zjQUZhuNkoQYrd0=
-----END CERTIFICATE-----
)";
}

TEST(PKIShardCertificateTest, ParseSubjectAlternativeNames) {
    const auto info = PKIShardCertificate::parseCertificatePEM(kCertificateWithSANs);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->subject_cn, "shard-001.example.test");
    EXPECT_EQ(info->san_dns, (std::vector<std::string>{"shard.example.test"}));
    EXPECT_EQ(info->san_ip, (std::vector<std::string>{"192.0.2.130"}));
    EXPECT_EQ(info->san_uri, (std::vector<std::string>{"urn:themis:shard:001"}));
}

TEST(PKIShardCertificateTest, ParseCertificateStructure) {
    // Test the structure is defined correctly
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.datacenter = "dc1";
    info.capabilities = {"read", "write", "replicate"};
    
    EXPECT_TRUE(info.hasCapability("read"));
    EXPECT_TRUE(info.hasCapability("write"));
    EXPECT_FALSE(info.hasCapability("admin"));
}

TEST(PKIShardCertificateTest, ValidateShardCertificate) {
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = kValidNotBefore;
    info.not_after = kValidNotAfter;
    info.capabilities = {"read", "write"};
    info.token_range_start = 0;
    info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;
    
    EXPECT_TRUE(PKIShardCertificate::validateShardCertificate(info));
}

TEST(PKIShardCertificateTest, ValidateInvalidCertificate) {
    ShardCertificateInfo info;
    // Missing shard_id
    info.not_before = kValidNotBefore;
    info.not_after = kValidNotAfter;
    info.capabilities = {"read"};
    
    EXPECT_FALSE(PKIShardCertificate::validateShardCertificate(info));
}

TEST(PKIShardCertificateTest, ValidateNoCapabilities) {
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = kValidNotBefore;
    info.not_after = kValidNotAfter;
    // No capabilities
    
    EXPECT_FALSE(PKIShardCertificate::validateShardCertificate(info));
}

TEST(PKIShardCertificateTest, ValidateInvalidTokenRange) {
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = kValidNotBefore;
    info.not_after = kValidNotAfter;
    info.capabilities = {"read"};
    info.token_range_start = 1000;
    info.token_range_end = 100;  // Invalid: start > end
    
    EXPECT_FALSE(PKIShardCertificate::validateShardCertificate(info));
}

TEST(PKIShardCertificateTest, ValidateRejectsMalformedPrintedTime) {
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = "Janu  1 00:00:00 2024 GMT";
    info.not_after = kValidNotAfter;
    info.capabilities = {"read"};
    info.token_range_start = 0;
    info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;

    EXPECT_FALSE(PKIShardCertificate::validateShardCertificate(info));
}
