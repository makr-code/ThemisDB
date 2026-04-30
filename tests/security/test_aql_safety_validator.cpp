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

using namespace themis::query;

// ---------------------------------------------------------------------------
// ASV-01  Safe read-only AQL → nullopt
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, SafeReadOnlyAql) {
    AqlSafetyValidator v;
    // Basic FOR/FILTER/RETURN
    EXPECT_FALSE(v.validate("FOR u IN users FILTER u.age > 18 RETURN u").has_value());
    EXPECT_FALSE(v.validate("FOR d IN docs RETURN d.title").has_value());
    EXPECT_FALSE(v.validate("RETURN 1 + 1").has_value());
    EXPECT_FALSE(v.validate("FOR x IN col FILTER x.key == @k RETURN x").has_value());
    // Nested FOR loops (read-only)
    EXPECT_FALSE(v.validate(
        "FOR u IN users FOR r IN roles FILTER r.user_id == u._key RETURN {u, r}"
    ).has_value());
    // SORT, LIMIT, LET
    EXPECT_FALSE(v.validate(
        "FOR d IN docs LET score = d.rank SORT score DESC LIMIT 10 RETURN d"
    ).has_value());
    // Multiple FILTER clauses
    EXPECT_FALSE(v.validate(
        "FOR p IN products FILTER p.active == true FILTER p.price < 100 RETURN p"
    ).has_value());
    // COLLECT / AGGREGATE
    EXPECT_FALSE(v.validate(
        "FOR u IN users COLLECT country = u.country AGGREGATE cnt = COUNT(1) RETURN {country, cnt}"
    ).has_value());
}

// ---------------------------------------------------------------------------
// ASV-02  REMOVE keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, RemoveKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = v.validate(
        "FOR u IN users FILTER u._key == '42' REMOVE u IN users");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "REMOVE");
}

// ---------------------------------------------------------------------------
// ASV-03  INSERT keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, InsertKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = v.validate("INSERT {name: 'Eve'} INTO users");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "INSERT");
}

// ---------------------------------------------------------------------------
// ASV-04  UPDATE keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, UpdateKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = v.validate("UPDATE '42' WITH {status: 'banned'} IN users");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "UPDATE");
}

// ---------------------------------------------------------------------------
// ASV-05  REPLACE keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ReplaceKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = v.validate("REPLACE '42' WITH {name: 'X'} IN users");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "REPLACE");
}

// ---------------------------------------------------------------------------
// ASV-06  UPSERT keyword → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, UpsertKeywordBlocked) {
    AqlSafetyValidator v;
    const auto r = v.validate(
        "UPSERT {_key: '42'} INSERT {name: 'X'} UPDATE {name: 'X'} IN users");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "UPSERT");
}

// ---------------------------------------------------------------------------
// ASV-07  DROP COLLECTION → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, DropCollectionBlocked) {
    AqlSafetyValidator v;
    const auto r = v.validate("DROP COLLECTION users");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "DROP");
}

// ---------------------------------------------------------------------------
// ASV-08  TRUNCATE → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, TruncateBlocked) {
    AqlSafetyValidator v;
    const auto r = v.validate("TRUNCATE users");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "TRUNCATE");
}

// ---------------------------------------------------------------------------
// ASV-09  FOR...REMOVE without FILTER → compound violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ForRemoveWithoutFilterBlocked) {
    AqlSafetyValidator v;
    // This variant has REMOVE detected as single keyword first — still a violation
    const auto r = v.validate("FOR u IN users REMOVE u IN users");
    ASSERT_TRUE(r.has_value());
}

// ---------------------------------------------------------------------------
// ASV-10  FOR...REMOVE even with FILTER → still blocked (REMOVE always banned)
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ForRemoveWithFilterStillBlocked) {
    AqlSafetyValidator v;
    const auto r = v.validate(
        "FOR u IN users FILTER u.role == 'guest' REMOVE u IN users");
    ASSERT_TRUE(r.has_value());  // REMOVE keyword is blocked regardless of FILTER
}

// ---------------------------------------------------------------------------
// ASV-11  isSafe() mirrors validate()
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, IsSafeMirrorsValidate) {
    AqlSafetyValidator v;
    const std::string safe_query  = "FOR u IN users RETURN u";
    const std::string unsafe_query = "REMOVE '1' IN users";
    EXPECT_TRUE(v.isSafe(safe_query));
    EXPECT_FALSE(v.isSafe(unsafe_query));
    EXPECT_EQ(v.isSafe(safe_query),  !v.validate(safe_query).has_value());
    EXPECT_EQ(v.isSafe(unsafe_query), !v.validate(unsafe_query).has_value());
}

// ---------------------------------------------------------------------------
// ASV-12  violation.message contains "AQL_READ_ONLY_VIOLATION"
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ViolationMessageContainsErrorCode) {
    AqlSafetyValidator v;
    const auto r = v.validate("INSERT {x:1} INTO col");
    ASSERT_TRUE(r.has_value());
    EXPECT_NE(r->message.find("AQL_READ_ONLY_VIOLATION"), std::string::npos);
}

// ---------------------------------------------------------------------------
// ASV-13  violation.keyword is populated correctly
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ViolationKeywordPopulated) {
    AqlSafetyValidator v;
    const auto r = v.validate("UPDATE '1' WITH {a:1} IN col");
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r->keyword.empty());
}

// ---------------------------------------------------------------------------
// ASV-14  violation.position points into the query
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, ViolationPositionInBounds) {
    AqlSafetyValidator v;
    const std::string query = "FOR u IN col REMOVE u IN col";
    const auto r = v.validate(query);
    ASSERT_TRUE(r.has_value());
    EXPECT_LT(r->position, query.size());
}

// ---------------------------------------------------------------------------
// ASV-15  Empty query → safe
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, EmptyQueryIsSafe) {
    AqlSafetyValidator v;
    EXPECT_FALSE(v.validate("").has_value());
    EXPECT_TRUE(v.isSafe(""));
}

// ---------------------------------------------------------------------------
// ASV-16  CREATE COLLECTION → violation
// ---------------------------------------------------------------------------
TEST(AqlSafetyValidatorTest, CreateCollectionBlocked) {
    AqlSafetyValidator v;
    const auto r = v.validate("CREATE COLLECTION new_users");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->keyword, "CREATE COLLECTION");
}
