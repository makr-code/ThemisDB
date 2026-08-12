#include <gtest/gtest.h>
#include "core/concerns/adapter_metadata.h"
#include "core/concerns/adapter_registry.h"
#include "core/concerns/adapter_signing.h"

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// SGN_01 — AdapterSignature::present() returns false when both fields empty
// ---------------------------------------------------------------------------
TEST(AdapterSigningTest, SGN_01_EmptySignatureNotPresent) {
    AdapterSignature sig;
    EXPECT_FALSE(sig.present());
}

// ---------------------------------------------------------------------------
// SGN_02 — AdapterSignature::present() returns true when both fields set
// ---------------------------------------------------------------------------
TEST(AdapterSigningTest, SGN_02_PopulatedSignatureIsPresent) {
    AdapterSignature sig{"sha256", "deadbeef"};
    EXPECT_TRUE(sig.present());
}

// ---------------------------------------------------------------------------
// SGN_03 — sha256Hex produces correct digest for empty string
//
// SHA-256("") == e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
// ---------------------------------------------------------------------------
TEST(AdapterSigningTest, SGN_03_Sha256HexEmptyInput) {
    const std::string hash = SignedAdapterValidator::sha256Hex("");
    ASSERT_EQ(hash.size(), 64u);
    EXPECT_EQ(hash,
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

// ---------------------------------------------------------------------------
// SGN_04 — canonicalString format: id:apiVersion:description
// ---------------------------------------------------------------------------
TEST(AdapterSigningTest, SGN_04_CanonicalStringFormat) {
    AdapterMetadata m;
    m.id          = "my_logger";
    m.apiVersion  = 2;
    m.description = "custom logger";

    const std::string cs = SignedAdapterValidator::canonicalString(m);
    EXPECT_EQ(cs, "my_logger:2:custom logger");
}

TEST(AdapterSigningTest, SGN_04b_CanonicalStringEmptyDescription) {
    AdapterMetadata m;
    m.id         = "tracer";
    m.apiVersion = 1;
    // description intentionally empty

    EXPECT_EQ(SignedAdapterValidator::canonicalString(m), "tracer:1:");
}

// ---------------------------------------------------------------------------
// SGN_05 — SignedAdapterValidator accepts matching digest
// ---------------------------------------------------------------------------
TEST(AdapterSigningTest, SGN_05_MatchingDigestAccepted) {
    AdapterMetadata m;
    m.id         = "alpha";
    m.apiVersion = 1;
    m.description = "";

    const std::string canonical = SignedAdapterValidator::canonicalString(m);
    const std::string digest    = SignedAdapterValidator::sha256Hex(canonical);
    ASSERT_FALSE(digest.empty());

    AdapterSignature sig{"sha256", digest};
    SignedAdapterValidator validator{sig};

    EXPECT_TRUE(validator.validate(m));
}

// ---------------------------------------------------------------------------
// SGN_06 — SignedAdapterValidator rejects mismatched digest
// ---------------------------------------------------------------------------
TEST(AdapterSigningTest, SGN_06_MismatchedDigestRejected) {
    AdapterMetadata m;
    m.id         = "beta";
    m.apiVersion = 1;

    AdapterSignature sig{
        "sha256",
        "0000000000000000000000000000000000000000000000000000000000000000"};
    SignedAdapterValidator validator{sig};

    EXPECT_FALSE(validator.validate(m));
}

// ---------------------------------------------------------------------------
// SGN_07 — SignedAdapterValidator rejects empty signature
// ---------------------------------------------------------------------------
TEST(AdapterSigningTest, SGN_07_EmptySignatureRejected) {
    AdapterMetadata m;
    m.id         = "gamma";
    m.apiVersion = 1;

    AdapterSignature sig; // empty
    SignedAdapterValidator validator{sig};

    EXPECT_FALSE(validator.validate(m));
}

// ---------------------------------------------------------------------------
// SGN_08 — SignedAdapterValidator rejects unknown algorithm
// ---------------------------------------------------------------------------
TEST(AdapterSigningTest, SGN_08_UnknownAlgorithmRejected) {
    AdapterMetadata m;
    m.id         = "delta";
    m.apiVersion = 1;

    AdapterSignature sig{"md5", "d41d8cd98f00b204e9800998ecf8427e"};
    SignedAdapterValidator validator{sig};

    EXPECT_FALSE(validator.validate(m));
}

// ---------------------------------------------------------------------------
// SGN_09 — Integration: registerAdapter passes when digest matches
// ---------------------------------------------------------------------------

struct IFakeAdapter {
    virtual ~IFakeAdapter() = default;
    virtual int value() const = 0;
};

struct FakeAdapterImpl : IFakeAdapter {
    explicit FakeAdapterImpl(int v) : v_(v) {}
    int value() const override { return v_; }
    int v_;
};

TEST(AdapterSigningTest, SGN_09_RegisterAdapterPassesWithValidSignature) {
    AdapterMetadata meta;
    meta.id         = "signed_adapter";
    meta.apiVersion = 1;
    meta.description = "";

    const std::string canonical = SignedAdapterValidator::canonicalString(meta);
    const std::string digest    = SignedAdapterValidator::sha256Hex(canonical);

    AdapterSignature sig{"sha256", digest};
    SignedAdapterValidator validator{sig};

    AdapterRegistry reg;
    auto adapter = std::make_shared<FakeAdapterImpl>(99);

    // Must NOT throw
    EXPECT_NO_THROW(
        reg.registerAdapter<IFakeAdapter>("signed_adapter", adapter, &validator, meta));

    auto resolved = reg.resolve<IFakeAdapter>();
    ASSERT_NE(resolved, nullptr);
    EXPECT_EQ(resolved->value(), 99);
}

// ---------------------------------------------------------------------------
// SGN_10 — Integration: registerAdapter throws with wrong signature
// ---------------------------------------------------------------------------
TEST(AdapterSigningTest, SGN_10_RegisterAdapterThrowsWithWrongSignature) {
    AdapterMetadata meta;
    meta.id         = "bad_sig_adapter";
    meta.apiVersion = 1;

    AdapterSignature sig{
        "sha256",
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"};
    SignedAdapterValidator validator{sig};

    AdapterRegistry reg;
    auto adapter = std::make_shared<FakeAdapterImpl>(1);

    EXPECT_THROW(
        reg.registerAdapter<IFakeAdapter>("bad_sig_adapter", adapter, &validator, meta),
        std::invalid_argument);
}
