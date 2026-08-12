// Unit tests for SPARQLParser and SPARQLToAQLTranspiler
// (SPARQL compatibility layer for RDF / knowledge-graph queries)

#include <gtest/gtest.h>
#include "query/sparql_parser.h"

using namespace themis::query;

// ============================================================================
// Helpers
// ============================================================================

static SPARQLASTNode mustParse(const std::string& sparql) {
    SPARQLParser parser;
    auto result = parser.parse(sparql);
    EXPECT_TRUE(result.has_value()) << (!result ? result.error().message() : "");
    return result.value();
}

static std::string mustTranspile(const SPARQLASTNode& ast,
                                  const std::string& collection = "rdf_triples") {
    SPARQLToAQLTranspiler t(collection);
    auto result = t.transpile(ast);
    EXPECT_TRUE(result.has_value()) << (!result ? result.error().message() : "");
    return result.value();
}

static std::string sparqlToAQL(const std::string& sparql,
                                const std::string& collection = "rdf_triples") {
    return mustTranspile(mustParse(sparql), collection);
}

// ============================================================================
// SPARQLParser – SELECT projection
// ============================================================================

TEST(SPARQLParserTest, SelectStar) {
    auto ast = mustParse("SELECT * WHERE { ?s ?p ?o }");
    EXPECT_TRUE(ast.select.star);
    EXPECT_TRUE(ast.select.variables.empty());
}

TEST(SPARQLParserTest, SelectVariables) {
    auto ast = mustParse("SELECT ?s ?p ?o WHERE { ?s ?p ?o }");
    EXPECT_FALSE(ast.select.star);
    ASSERT_EQ(ast.select.variables.size(), 3u);
    EXPECT_EQ(ast.select.variables[0], "s");
    EXPECT_EQ(ast.select.variables[1], "p");
    EXPECT_EQ(ast.select.variables[2], "o");
}

TEST(SPARQLParserTest, SelectSingleVariable) {
    auto ast = mustParse("SELECT ?name WHERE { ?person rdf:type foaf:Person . ?person foaf:name ?name }");
    ASSERT_EQ(ast.select.variables.size(), 1u);
    EXPECT_EQ(ast.select.variables[0], "name");
}

// ============================================================================
// SPARQLParser – FROM clause
// ============================================================================

TEST(SPARQLParserTest, FromClause) {
    auto ast = mustParse("SELECT * FROM <http://example.org/graph> WHERE { ?s ?p ?o }");
    ASSERT_TRUE(ast.select.from_graph.has_value());
    EXPECT_EQ(*ast.select.from_graph, "http://example.org/graph");
}

// ============================================================================
// SPARQLParser – WHERE triple patterns
// ============================================================================

TEST(SPARQLParserTest, SingleTripleAllVariables) {
    auto ast = mustParse("SELECT * WHERE { ?s ?p ?o }");
    ASSERT_EQ(ast.select.where_clauses.size(), 1u);
    const auto& clause = ast.select.where_clauses[0];
    ASSERT_EQ(clause.kind, SPARQLClauseKind::TriplePattern);
    ASSERT_TRUE(clause.triple.has_value());
    EXPECT_EQ(clause.triple->subject.type,   SPARQLTermType::Variable);
    EXPECT_EQ(clause.triple->subject.value,  "s");
    EXPECT_EQ(clause.triple->predicate.type, SPARQLTermType::Variable);
    EXPECT_EQ(clause.triple->predicate.value,"p");
    EXPECT_EQ(clause.triple->object.type,    SPARQLTermType::Variable);
    EXPECT_EQ(clause.triple->object.value,   "o");
}

TEST(SPARQLParserTest, TripleWithURIAndPrefixedName) {
    auto ast = mustParse(
        "SELECT ?person WHERE { ?person rdf:type <http://xmlns.com/foaf/0.1/Person> }");
    ASSERT_EQ(ast.select.where_clauses.size(), 1u);
    const auto& tp = *ast.select.where_clauses[0].triple;
    EXPECT_EQ(tp.predicate.type,  SPARQLTermType::PrefixedName);
    EXPECT_EQ(tp.predicate.value, "rdf:type");
    EXPECT_EQ(tp.object.type,     SPARQLTermType::URIRef);
    EXPECT_EQ(tp.object.value,    "http://xmlns.com/foaf/0.1/Person");
}

TEST(SPARQLParserTest, MultipleTriplePatterns) {
    auto ast = mustParse(
        "SELECT ?person ?name WHERE { "
        "  ?person rdf:type foaf:Person . "
        "  ?person foaf:name ?name "
        "}");
    ASSERT_EQ(ast.select.where_clauses.size(), 2u);
    EXPECT_EQ(ast.select.where_clauses[0].kind, SPARQLClauseKind::TriplePattern);
    EXPECT_EQ(ast.select.where_clauses[1].kind, SPARQLClauseKind::TriplePattern);
}

TEST(SPARQLParserTest, TripleWithStringLiteral) {
    auto ast = mustParse(
        "SELECT ?s WHERE { ?s dc:title \"Hello World\" }");
    ASSERT_EQ(ast.select.where_clauses.size(), 1u);
    const auto& tp = *ast.select.where_clauses[0].triple;
    EXPECT_EQ(tp.object.type, SPARQLTermType::Literal);
    EXPECT_EQ(tp.object.value, "Hello World");
}

TEST(SPARQLParserTest, TripleWithIntegerLiteral) {
    auto ast = mustParse("SELECT ?s WHERE { ?s ex:age 42 }");
    const auto& tp = *ast.select.where_clauses[0].triple;
    EXPECT_EQ(tp.object.type, SPARQLTermType::Literal);
    EXPECT_TRUE(tp.object.is_literal_value);
    EXPECT_EQ(std::get<int64_t>(tp.object.literal_value), 42);
}

TEST(SPARQLParserTest, TripleWithBooleanLiteral) {
    auto ast = mustParse("SELECT ?s WHERE { ?s ex:active true }");
    const auto& tp = *ast.select.where_clauses[0].triple;
    EXPECT_EQ(tp.object.type, SPARQLTermType::Literal);
    EXPECT_TRUE(std::get<bool>(tp.object.literal_value));
}

// ============================================================================
// SPARQLParser – FILTER clause
// ============================================================================

TEST(SPARQLParserTest, SimpleFilter) {
    auto ast = mustParse("SELECT ?s ?v WHERE { ?s ex:value ?v . FILTER(?v > 100) }");
    ASSERT_EQ(ast.select.where_clauses.size(), 2u);
    EXPECT_EQ(ast.select.where_clauses[0].kind, SPARQLClauseKind::TriplePattern);
    EXPECT_EQ(ast.select.where_clauses[1].kind, SPARQLClauseKind::Filter);
    ASSERT_TRUE(ast.select.where_clauses[1].filter_expr != nullptr);
}

TEST(SPARQLParserTest, FilterWithAndOr) {
    auto ast = mustParse(
        "SELECT ?s ?v WHERE { ?s ex:val ?v . FILTER(?v > 10 && ?v < 100) }");
    EXPECT_EQ(ast.select.where_clauses[1].kind, SPARQLClauseKind::Filter);
}

TEST(SPARQLParserTest, FilterWithNot) {
    auto ast = mustParse(
        "SELECT ?s WHERE { ?s ex:active ?a . FILTER(!?a) }");
    EXPECT_EQ(ast.select.where_clauses[1].kind, SPARQLClauseKind::Filter);
}

// ============================================================================
// SPARQLParser – ORDER BY
// ============================================================================

TEST(SPARQLParserTest, OrderByAsc) {
    auto ast = mustParse("SELECT * WHERE { ?s ?p ?o } ORDER BY ASC(?s)");
    ASSERT_EQ(ast.select.order_by.size(), 1u);
    EXPECT_EQ(ast.select.order_by[0].variable, "s");
    EXPECT_TRUE(ast.select.order_by[0].ascending);
}

TEST(SPARQLParserTest, OrderByDesc) {
    auto ast = mustParse("SELECT * WHERE { ?s ?p ?o } ORDER BY DESC(?s)");
    ASSERT_EQ(ast.select.order_by.size(), 1u);
    EXPECT_EQ(ast.select.order_by[0].variable, "s");
    EXPECT_FALSE(ast.select.order_by[0].ascending);
}

TEST(SPARQLParserTest, OrderByBareVariable) {
    auto ast = mustParse("SELECT * WHERE { ?s ?p ?o } ORDER BY ?s");
    ASSERT_EQ(ast.select.order_by.size(), 1u);
    EXPECT_EQ(ast.select.order_by[0].variable, "s");
    EXPECT_TRUE(ast.select.order_by[0].ascending);
}

// ============================================================================
// SPARQLParser – LIMIT / OFFSET
// ============================================================================

TEST(SPARQLParserTest, LimitOnly) {
    auto ast = mustParse("SELECT * WHERE { ?s ?p ?o } LIMIT 10");
    ASSERT_TRUE(ast.select.limit.has_value());
    EXPECT_EQ(*ast.select.limit, 10);
    EXPECT_FALSE(ast.select.offset.has_value());
}

TEST(SPARQLParserTest, LimitAndOffset) {
    auto ast = mustParse("SELECT * WHERE { ?s ?p ?o } LIMIT 20 OFFSET 40");
    ASSERT_TRUE(ast.select.limit.has_value());
    ASSERT_TRUE(ast.select.offset.has_value());
    EXPECT_EQ(*ast.select.limit,  20);
    EXPECT_EQ(*ast.select.offset, 40);
}

// ============================================================================
// SPARQLParser – trailing semicolon
// ============================================================================

TEST(SPARQLParserTest, TrailingSemicolon) {
    auto ast = mustParse("SELECT * WHERE { ?s ?p ?o };");
    EXPECT_TRUE(ast.select.star);
}

// ============================================================================
// SPARQLParser – case-insensitive keywords
// ============================================================================

TEST(SPARQLParserTest, CaseInsensitiveKeywords) {
    auto ast = mustParse("select * where { ?s ?p ?o } limit 5");
    EXPECT_TRUE(ast.select.star);
    ASSERT_TRUE(ast.select.limit.has_value());
    EXPECT_EQ(*ast.select.limit, 5);
}

// ============================================================================
// SPARQLParser – error cases
// ============================================================================

TEST(SPARQLParserTest, MissingSelect) {
    SPARQLParser parser;
    auto result = parser.parse("WHERE { ?s ?p ?o }");
    EXPECT_FALSE(result.has_value());
}

TEST(SPARQLParserTest, MissingWhere) {
    SPARQLParser parser;
    auto result = parser.parse("SELECT * { ?s ?p ?o }");
    EXPECT_FALSE(result.has_value());
}

TEST(SPARQLParserTest, MissingOpenBrace) {
    SPARQLParser parser;
    auto result = parser.parse("SELECT * WHERE ?s ?p ?o }");
    EXPECT_FALSE(result.has_value());
}

TEST(SPARQLParserTest, MissingCloseBrace) {
    SPARQLParser parser;
    auto result = parser.parse("SELECT * WHERE { ?s ?p ?o");
    EXPECT_FALSE(result.has_value());
}

TEST(SPARQLParserTest, EmptyVariableList) {
    SPARQLParser parser;
    auto result = parser.parse("SELECT WHERE { ?s ?p ?o }");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// SPARQLToAQLTranspiler – SELECT * (all triples)
// ============================================================================

TEST(SPARQLTranspilerTest, SelectStarAllVariables) {
    std::string aql = sparqlToAQL("SELECT * WHERE { ?s ?p ?o }");
    EXPECT_NE(aql.find("FOR _t0 IN rdf_triples"), std::string::npos);
    EXPECT_NE(aql.find("RETURN {"), std::string::npos);
    EXPECT_NE(aql.find("_t0.subject"),   std::string::npos);
    EXPECT_NE(aql.find("_t0.predicate"), std::string::npos);
    EXPECT_NE(aql.find("_t0.object"),    std::string::npos);
}

TEST(SPARQLTranspilerTest, SelectSingleVariable) {
    std::string aql = sparqlToAQL("SELECT ?s WHERE { ?s ?p ?o }");
    EXPECT_NE(aql.find("FOR _t0 IN rdf_triples"), std::string::npos);
    EXPECT_NE(aql.find("RETURN _t0.subject"), std::string::npos);
}

// ============================================================================
// SPARQLToAQLTranspiler – constant predicates/objects generate FILTER
// ============================================================================

TEST(SPARQLTranspilerTest, ConstantPredicateFilter) {
    std::string aql = sparqlToAQL(
        "SELECT ?person WHERE { ?person rdf:type foaf:Person }");
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("\"rdf:type\""),    std::string::npos);
    EXPECT_NE(aql.find("\"foaf:Person\""), std::string::npos);
}

TEST(SPARQLTranspilerTest, URIObjectFilter) {
    std::string aql = sparqlToAQL(
        "SELECT ?s WHERE { ?s rdf:type <http://schema.org/Person> }");
    EXPECT_NE(aql.find("\"http://schema.org/Person\""), std::string::npos);
}

// ============================================================================
// SPARQLToAQLTranspiler – variable unification across patterns
// ============================================================================

TEST(SPARQLTranspilerTest, VariableUnificationTwoPatterns) {
    std::string aql = sparqlToAQL(
        "SELECT ?person ?name WHERE { "
        "  ?person rdf:type foaf:Person . "
        "  ?person foaf:name ?name "
        "}");
    // Both patterns should reference rdf_triples
    EXPECT_NE(aql.find("FOR _t0 IN rdf_triples"), std::string::npos);
    EXPECT_NE(aql.find("FOR _t1 IN rdf_triples"), std::string::npos);
    // Second pattern must unify ?person to _t0.subject
    EXPECT_NE(aql.find("_t1.subject == _t0.subject"), std::string::npos);
    // Return object
    EXPECT_NE(aql.find("person:"), std::string::npos);
    EXPECT_NE(aql.find("name:"),   std::string::npos);
}

TEST(SPARQLTranspilerTest, ThreePatternsWithUnification) {
    std::string aql = sparqlToAQL(
        "SELECT ?person ?name ?email WHERE { "
        "  ?person rdf:type foaf:Person . "
        "  ?person foaf:name ?name . "
        "  ?person foaf:mbox ?email "
        "}");
    EXPECT_NE(aql.find("FOR _t0 IN rdf_triples"), std::string::npos);
    EXPECT_NE(aql.find("FOR _t1 IN rdf_triples"), std::string::npos);
    EXPECT_NE(aql.find("FOR _t2 IN rdf_triples"), std::string::npos);
}

// ============================================================================
// SPARQLToAQLTranspiler – FILTER expressions
// ============================================================================

TEST(SPARQLTranspilerTest, FilterGreaterThan) {
    std::string aql = sparqlToAQL(
        "SELECT ?s ?v WHERE { ?s ex:value ?v . FILTER(?v > 100) }");
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("> 100"),  std::string::npos);
}

TEST(SPARQLTranspilerTest, FilterLessThanOrEqual) {
    std::string aql = sparqlToAQL(
        "SELECT ?s ?v WHERE { ?s ex:score ?v . FILTER(?v <= 50) }");
    EXPECT_NE(aql.find("<= 50"), std::string::npos);
}

TEST(SPARQLTranspilerTest, FilterEquality) {
    std::string aql = sparqlToAQL(
        "SELECT ?s WHERE { ?s ex:status ?st . FILTER(?st == \"active\") }");
    EXPECT_NE(aql.find("=="), std::string::npos);
    EXPECT_NE(aql.find("\"active\""), std::string::npos);
}

TEST(SPARQLTranspilerTest, FilterAndCondition) {
    std::string aql = sparqlToAQL(
        "SELECT ?s ?v WHERE { ?s ex:val ?v . FILTER(?v > 10 && ?v < 100) }");
    EXPECT_NE(aql.find("AND"), std::string::npos);
    EXPECT_NE(aql.find("> 10"),  std::string::npos);
    EXPECT_NE(aql.find("< 100"), std::string::npos);
}

TEST(SPARQLTranspilerTest, FilterOrCondition) {
    std::string aql = sparqlToAQL(
        "SELECT ?s ?v WHERE { ?s ex:val ?v . FILTER(?v < 0 || ?v > 100) }");
    EXPECT_NE(aql.find("OR"), std::string::npos);
}

TEST(SPARQLTranspilerTest, FilterNotCondition) {
    std::string aql = sparqlToAQL(
        "SELECT ?s ?a WHERE { ?s ex:active ?a . FILTER(!?a) }");
    EXPECT_NE(aql.find("NOT"), std::string::npos);
}

// ============================================================================
// SPARQLToAQLTranspiler – ORDER BY
// ============================================================================

TEST(SPARQLTranspilerTest, OrderByAsc) {
    std::string aql = sparqlToAQL(
        "SELECT ?s WHERE { ?s ?p ?o } ORDER BY ASC(?s)");
    EXPECT_NE(aql.find("SORT"), std::string::npos);
    EXPECT_NE(aql.find("ASC"),  std::string::npos);
}

TEST(SPARQLTranspilerTest, OrderByDesc) {
    std::string aql = sparqlToAQL(
        "SELECT ?s WHERE { ?s ?p ?o } ORDER BY DESC(?s)");
    EXPECT_NE(aql.find("SORT"), std::string::npos);
    EXPECT_NE(aql.find("DESC"), std::string::npos);
}

// ============================================================================
// SPARQLToAQLTranspiler – LIMIT / OFFSET
// ============================================================================

TEST(SPARQLTranspilerTest, LimitOnly) {
    std::string aql = sparqlToAQL("SELECT * WHERE { ?s ?p ?o } LIMIT 10");
    EXPECT_NE(aql.find("LIMIT 10"), std::string::npos);
}

TEST(SPARQLTranspilerTest, LimitAndOffset) {
    std::string aql = sparqlToAQL("SELECT * WHERE { ?s ?p ?o } LIMIT 10 OFFSET 20");
    // AQL LIMIT format: LIMIT offset, count
    EXPECT_NE(aql.find("LIMIT 20, 10"), std::string::npos);
}

// ============================================================================
// SPARQLToAQLTranspiler – custom collection name
// ============================================================================

TEST(SPARQLTranspilerTest, CustomCollection) {
    std::string aql = sparqlToAQL("SELECT * WHERE { ?s ?p ?o }", "kg_triples");
    EXPECT_NE(aql.find("FOR _t0 IN kg_triples"), std::string::npos);
    EXPECT_EQ(aql.find("rdf_triples"), std::string::npos);
}

// ============================================================================
// SPARQLToAQLTranspiler – full pipeline
// ============================================================================

TEST(SPARQLTranspilerTest, FullPipeline) {
    std::string aql = sparqlToAQL(
        "SELECT ?person ?name WHERE { "
        "  ?person rdf:type foaf:Person . "
        "  ?person foaf:name ?name . "
        "  FILTER(?name != \"unknown\") "
        "} ORDER BY ASC(?name) LIMIT 5");

    EXPECT_NE(aql.find("FOR _t0 IN rdf_triples"), std::string::npos);
    EXPECT_NE(aql.find("FOR _t1 IN rdf_triples"), std::string::npos);
    EXPECT_NE(aql.find("\"rdf:type\""),    std::string::npos);
    EXPECT_NE(aql.find("\"foaf:Person\""), std::string::npos);
    EXPECT_NE(aql.find("\"foaf:name\""),   std::string::npos);
    EXPECT_NE(aql.find("FILTER"),          std::string::npos);
    EXPECT_NE(aql.find("\"unknown\""),     std::string::npos);
    EXPECT_NE(aql.find("SORT"),            std::string::npos);
    EXPECT_NE(aql.find("LIMIT 5"),         std::string::npos);
    EXPECT_NE(aql.find("RETURN {"),        std::string::npos);
}

TEST(SPARQLTranspilerTest, StringLiteralEscaping) {
    // String with embedded double-quote → must be properly escaped in AQL
    std::string aql = sparqlToAQL(
        "SELECT ?s WHERE { ?s dc:title \"He said \\\"hello\\\"\" }");
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
}

TEST(SPARQLTranspilerTest, EmptyWhereClause) {
    // SELECT * WHERE {} → no FOR loops, RETURN {} with empty bindings
    std::string aql = sparqlToAQL("SELECT * WHERE {}");
    EXPECT_NE(aql.find("RETURN {}"), std::string::npos);
    EXPECT_EQ(aql.find("FOR"), std::string::npos);
}

// ============================================================================
// SPARQLParser – Numeric overflow guards (REL-13..15, issue #5177)
// ============================================================================

static bool sparqlParseError(const std::string& sparql) {
    SPARQLParser parser;
    auto result = parser.parse(sparql);
    return !result.has_value();
}

// LIMIT with out-of-range integer is rejected
TEST(SPARQLParserTest, LimitOverflowIsError) {
    EXPECT_TRUE(sparqlParseError("SELECT * WHERE { ?s ?p ?o } LIMIT 99999999999999999999"));
}

// OFFSET with out-of-range integer is rejected
TEST(SPARQLParserTest, OffsetOverflowIsError) {
    EXPECT_TRUE(sparqlParseError("SELECT * WHERE { ?s ?p ?o } LIMIT 10 OFFSET 99999999999999999999"));
}

// Integer literal overflow in a filter expression is rejected
TEST(SPARQLParserTest, IntLiteralOverflowIsError) {
    EXPECT_TRUE(sparqlParseError(
        "SELECT * WHERE { ?s <ex:id> 99999999999999999999 }"));
}

// Float literal overflow in a filter expression is rejected
TEST(SPARQLParserTest, FloatLiteralOverflowIsError) {
    EXPECT_TRUE(sparqlParseError(
        "SELECT * WHERE { ?s <ex:score> 1e99999 }"));
}

// Valid LIMIT / OFFSET are still accepted after adding the guard
TEST(SPARQLParserTest, ValidLimitOffsetStillAccepted) {
    EXPECT_FALSE(sparqlParseError("SELECT * WHERE { ?s ?p ?o } LIMIT 100 OFFSET 50"));
}
