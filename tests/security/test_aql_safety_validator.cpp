// Copyright 2026 ThemisDB — Licensed under MIT License
// AI Safety Layer — Schicht 3: AQL Read-Only Enforcer
// ASL-3: AqlSafetyValidator unit tests
//
// Tests:
//   ASV-01  Safe AQL (FOR/FILTER/RETURN) → nullopt (no violation)
//   ASV-02  AQL with REMOVE keyword → violation
//   ASV-03  AQL with INSERT keyword → violation
//   ASV-04  AQL with UPDATE keyword → violation
//   ASV-05  AQL with REPLACE keyword → violation
//   ASV-06  AQL with UPSERT keyword → violation
//   ASV-07  AQL with DROP COLLECTION → violation
//   ASV-08  AQL with TRUNCATE → violation
//   ASV-09  FOR...REMOVE without FILTER → violation (full-collection delete)
//   ASV-10  FOR...REMOVE with FILTER → still violation (REMOVE always banned)
//   ASV-11  isSafe() mirrors validate() result
//   ASV-12  violation.message contains "AQL_READ_ONLY_VIOLATION"
//   ASV-13  violation.keyword is populated correctly
//   ASV-14  violation.position points into the query string
//   ASV-15  Empty query → safe (nullopt)
//   ASV-16  CREATE COLLECTION → violation

#include <gtest/gtest.h>
#include "query/aql_safety_validator.h"

#include <string>
#include <string_view>

using namespace themis::query;
using namespace std::literals;

namespace {
using ViolationOpt = std::optional<AqlSafetyValidator::Violation>;

ViolationOpt validateSV(const AqlSafetyValidator& validator, std::string_view query) {
    static constexpr auto kValidateSv =
        static_cast<ViolationOpt (AqlSafetyValidator::*)(std::string_view) const>(
            &AqlSafetyValidator::validate);
    return (validator.*kValidateSv)(query);
}

bool isSafeSV(const AqlSafetyValidator& validator, std::string_view query) {
    return !validateSV(validator, query).has_value();
}
}  // namespace

// ---------------------------------------------------------------------------
// ASV-01  Safe read-only AQL → nullopt
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, SafeReadOnlyAql) {
    AqlSafetyValidator v;
    // Basic FOR/FILTER/RETURN
    EXPECT_FALSE(validateSV(v, "FOR u IN users FILTER u.age > 18 RETURN u"sv).has_value());
    EXPECT_FALSE(validateSV(v, "FOR d IN docs RETURN d.title"sv).has_value());
    EXPECT_FALSE(validateSV(v, "RETURN 1 + 1"sv).has_value());
    EXPECT_FALSE(validateSV(v, "FOR x IN col FILTER x.key == @k RETURN x"sv).has_value());
    // Nested FOR loops (read-only)
    EXPECT_FALSE(validateSV(v,
        "FOR u IN users FOR r IN roles FILTER r.user_id == u._key RETURN {u, r}"sv
    ).has_value());
    // SORT, LIMIT, LET
    EXPECT_FALSE(validateSV(v,
        "FOR d IN docs LET score = d.rank SORT score DESC LIMIT 10 RETURN d"sv
    ).has_value());
    // Multiple FILTER clauses
    EXPECT_FALSE(validateSV(v,
        "FOR p IN products FILTER p.active == true FILTER p.price < 100 RETURN p"sv
    ).has_value());
    // COLLECT / AGGREGATE
    EXPECT_FALSE(validateSV(v,
        "FOR u IN users COLLECT country = u.country AGGREGATE cnt = COUNT(1) RETURN {country, cnt}"sv
    ).has_value());
}

// ---------------------------------------------------------------------------
// ASV-02  REMOVE keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, RemoveKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = validateSV(v,
        "FOR u IN users FILTER u._key == '42' REMOVE u IN users"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "REMOVE");
}

// ---------------------------------------------------------------------------
// ASV-03  INSERT keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, InsertKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = validateSV(v, "INSERT {name: 'Eve'} INTO users"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "INSERT");
}

// ---------------------------------------------------------------------------
// ASV-04  UPDATE keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, UpdateKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = validateSV(v, "UPDATE '42' WITH {status: 'banned'} IN users"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "UPDATE");
}

// ---------------------------------------------------------------------------
// ASV-05  REPLACE keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ReplaceKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = validateSV(v, "REPLACE '42' WITH {name: 'X'} IN users"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "REPLACE");
}

// ---------------------------------------------------------------------------
// ASV-06  UPSERT keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, UpsertKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = validateSV(v,
        "UPSERT {_key: '42'} INSERT {name: 'X'} UPDATE {name: 'X'} IN users"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "UPSERT");
}

// ---------------------------------------------------------------------------
// ASV-07  DROP COLLECTION → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, DropCollectionBlocked) {
    AqlSafetyValidator v;
    const auto r = validateSV(v, "DROP COLLECTION users"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "DROP");
}

// ---------------------------------------------------------------------------
// ASV-08  TRUNCATE → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, TruncateBlocked) {
    AqlSafetyValidator v;
    const auto r = validateSV(v, "TRUNCATE users"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "TRUNCATE");
}

// ---------------------------------------------------------------------------
// ASV-09  FOR...REMOVE without FILTER → compound violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ForRemoveWithoutFilterBlocked) {
    AqlSafetyValidator v;
    // This variant has REMOVE detected as single keyword first — still a violation
    const auto r = validateSV(v, "FOR u IN users REMOVE u IN users"sv);
    ASSERT_TRUE(r.has_value());
}

// ---------------------------------------------------------------------------
// ASV-10  FOR...REMOVE even with FILTER → still blocked (REMOVE always banned)
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ForRemoveWithFilterStillBlocked) {
    AqlSafetyValidator v;
    const auto r = validateSV(v,
        "FOR u IN users FILTER u.role == 'guest' REMOVE u IN users"sv);
    ASSERT_TRUE(r.has_value());  // REMOVE keyword is blocked regardless of FILTER
}

// ---------------------------------------------------------------------------
// ASV-11  isSafe() mirrors validate()
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, IsSafeMirrorsValidate) {
    AqlSafetyValidator v;
    const std::string safe_query  = "FOR u IN users RETURN u";
    const std::string unsafe_query = "REMOVE '1' IN users";
    EXPECT_TRUE(isSafeSV(v, std::string_view{safe_query}));
    EXPECT_FALSE(isSafeSV(v, std::string_view{unsafe_query}));
    EXPECT_EQ(isSafeSV(v, std::string_view{safe_query}),
              !validateSV(v, std::string_view{safe_query}).has_value());
    EXPECT_EQ(isSafeSV(v, std::string_view{unsafe_query}),
              !validateSV(v, std::string_view{unsafe_query}).has_value());
}

// ---------------------------------------------------------------------------
// ASV-12  violation.message contains "AQL_READ_ONLY_VIOLATION"
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ViolationMessageContainsErrorCode) {
    AqlSafetyValidator v;
    const auto r = validateSV(v, "INSERT {x:1} INTO col"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_NE(r->message.find("AQL_READ_ONLY_VIOLATION"), std::string::npos);
}

// ---------------------------------------------------------------------------
// ASV-13  violation.keyword is populated correctly
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ViolationKeywordPopulated) {
    AqlSafetyValidator v;
    const auto r = validateSV(v, "UPDATE '1' WITH {a:1} IN col"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r->keyword.empty());
}

// ---------------------------------------------------------------------------
// ASV-14  violation.position points into the query
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ViolationPositionInBounds) {
    AqlSafetyValidator v;
    const std::string query = "FOR u IN col REMOVE u IN col";
    const auto r = validateSV(v, std::string_view{query});
    ASSERT_TRUE(r.has_value());
    EXPECT_LT(r->position, query.size());
}

// ---------------------------------------------------------------------------
// ASV-15  Empty query → safe
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, EmptyQueryIsSafe) {
    AqlSafetyValidator v;
    EXPECT_FALSE(validateSV(v, ""sv).has_value());
    EXPECT_TRUE(isSafeSV(v, ""sv));
}

// ---------------------------------------------------------------------------
// ASV-16  CREATE COLLECTION → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, CreateCollectionBlocked) {
    AqlSafetyValidator v;
    const auto r = validateSV(v, "CREATE COLLECTION new_users"sv);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "CREATE COLLECTION");
}
