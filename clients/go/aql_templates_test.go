package themisdb

import (
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
)

// ─── AQLQueryBuilder tests ────────────────────────────────────────────────────

func TestAQLQueryBuilder_SimpleFor(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		Return("u").
		Build()

	assert.Equal(t, "FOR u IN users\nRETURN u", query)
}

func TestAQLQueryBuilder_ForWithFilter(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		Filter("u.age > 18").
		Return("u").
		Build()

	assert.Contains(t, query, "FOR u IN users")
	assert.Contains(t, query, "FILTER u.age > 18")
	assert.Contains(t, query, "RETURN u")
}

func TestAQLQueryBuilder_ForWithMultipleFilters(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		Filter("u.age > 18").
		Filter("u.active == true").
		Return("u").
		Build()

	assert.Contains(t, query, "FILTER u.age > 18")
	assert.Contains(t, query, "FILTER u.active == true")
}

func TestAQLQueryBuilder_SortAndLimit(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		Sort("u.name", SortAsc).
		Limit(0, 10).
		Return("u").
		Build()

	assert.Contains(t, query, "SORT u.name ASC")
	assert.Contains(t, query, "LIMIT 10")
}

func TestAQLQueryBuilder_LimitWithOffset(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		Limit(20, 10).
		Return("u").
		Build()

	assert.Contains(t, query, "LIMIT 20, 10")
}

func TestAQLQueryBuilder_SortDesc(t *testing.T) {
	query := NewAQLQuery().
		For("o", "orders").
		Sort("o.total", SortDesc).
		Return("o").
		Build()

	assert.Contains(t, query, "SORT o.total DESC")
}

func TestAQLQueryBuilder_MultipleSortFields(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		Sort("u.city", SortAsc).
		Sort("u.name", SortAsc).
		Return("u").
		Build()

	assert.Contains(t, query, "SORT u.city ASC, u.name ASC")
}

func TestAQLQueryBuilder_LetClause(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		Let("fullName", "CONCAT(u.first, ' ', u.last)").
		Return("{ name: fullName }").
		Build()

	assert.Contains(t, query, "LET fullName = CONCAT(u.first, ' ', u.last)")
	assert.Contains(t, query, "RETURN { name: fullName }")
}

func TestAQLQueryBuilder_CollectWithAggregate(t *testing.T) {
	query := NewAQLQuery().
		For("o", "orders").
		Collect("city = o.city", "total = SUM(o.amount)").
		Sort("total", SortDesc).
		Return("{ city, total }").
		Build()

	assert.Contains(t, query, "COLLECT city = o.city AGGREGATE total = SUM(o.amount)")
	assert.Contains(t, query, "SORT total DESC")
}

func TestAQLQueryBuilder_CollectWithoutAggregate(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		Collect("city = u.city", "").
		Return("city").
		Build()

	assert.Contains(t, query, "COLLECT city = u.city")
	assert.NotContains(t, query, "AGGREGATE")
}

func TestAQLQueryBuilder_ForRange(t *testing.T) {
	query := NewAQLQuery().
		ForRange("i", 1, 5).
		Return("i").
		Build()

	assert.Contains(t, query, "FOR i IN 1..5")
}

func TestAQLQueryBuilder_ForTraversal(t *testing.T) {
	query := NewAQLQuery().
		ForTraversal("v", "e", "p", 1, 3, TraversalOutbound, "'users/john'", "friends").
		Return("v.name").
		Build()

	assert.Contains(t, query, "FOR v, e, p IN 1..3 OUTBOUND 'users/john' friends")
	assert.Contains(t, query, "RETURN v.name")
}

func TestAQLQueryBuilder_ForTraversalNoEdgeVar(t *testing.T) {
	query := NewAQLQuery().
		ForTraversal("v", "", "", 1, 2, TraversalInbound, "'orgs/acme'", "belongs_to").
		Return("v").
		Build()

	assert.Contains(t, query, "FOR v IN 1..2 INBOUND 'orgs/acme' belongs_to")
}

func TestAQLQueryBuilder_ForTraversalMultipleEdgeCollections(t *testing.T) {
	query := NewAQLQuery().
		ForTraversal("v", "", "", 1, 3, TraversalAny, "'users/alice'", "friends", "colleagues").
		Return("v").
		Build()

	assert.Contains(t, query, "FOR v IN 1..3 ANY 'users/alice' friends, colleagues")
}

func TestAQLQueryBuilder_ReturnDistinct(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		ReturnDistinct("u.city").
		Build()

	assert.Contains(t, query, "RETURN DISTINCT u.city")
}

func TestAQLQueryBuilder_NestedFor(t *testing.T) {
	query := NewAQLQuery().
		For("u", "users").
		For("o", "orders").
		Filter("o.user_id == u._key").
		Return("{ user: u.name, order: o.total }").
		Build()

	lines := strings.Split(query, "\n")
	assert.Equal(t, "FOR u IN users", lines[0])
	assert.Equal(t, "FOR o IN orders", lines[1])
}

// ─── Template function tests ──────────────────────────────────────────────────

func TestSimpleQueryTemplate_Full(t *testing.T) {
	query := SimpleQueryTemplate("users", "u", "u.age > 18", "u.name", SortAsc, 10)

	assert.Contains(t, query, "FOR u IN users")
	assert.Contains(t, query, "FILTER u.age > 18")
	assert.Contains(t, query, "SORT u.name ASC")
	assert.Contains(t, query, "LIMIT 10")
	assert.Contains(t, query, "RETURN u")
}

func TestSimpleQueryTemplate_NoFilter(t *testing.T) {
	query := SimpleQueryTemplate("products", "p", "", "p.price", SortDesc, 5)

	assert.NotContains(t, query, "FILTER")
	assert.Contains(t, query, "SORT p.price DESC")
	assert.Contains(t, query, "LIMIT 5")
}

func TestSimpleQueryTemplate_NoSortNoLimit(t *testing.T) {
	query := SimpleQueryTemplate("logs", "l", "l.level == 'ERROR'", "", SortAsc, 0)

	assert.Contains(t, query, "FILTER l.level == 'ERROR'")
	assert.NotContains(t, query, "SORT")
	assert.NotContains(t, query, "LIMIT")
}

func TestJoinQueryTemplate(t *testing.T) {
	query := JoinQueryTemplate(
		"users", "u",
		"orders", "o",
		"o.user_id == u._key",
		"{ user_name: u.name, order_total: o.total }",
	)

	assert.Contains(t, query, "FOR u IN users")
	assert.Contains(t, query, "FOR o IN orders")
	assert.Contains(t, query, "FILTER o.user_id == u._key")
	assert.Contains(t, query, "RETURN { user_name: u.name, order_total: o.total }")
}

func TestGraphTraversalTemplate(t *testing.T) {
	query := GraphTraversalTemplate("v", 1, 3, TraversalOutbound, "'users/john'", "friends", "v.name")

	assert.Contains(t, query, "FOR v IN 1..3 OUTBOUND 'users/john' friends")
	assert.Contains(t, query, "RETURN v.name")
}

func TestAggregationTemplate_WithFilter(t *testing.T) {
	query := AggregationTemplate(
		"orders", "o",
		"o.status == 'completed'",
		"city = o.city",
		"total_sales = SUM(o.amount)",
		"total_sales", SortDesc,
		"{ city, total_sales }",
	)

	assert.Contains(t, query, "FOR o IN orders")
	assert.Contains(t, query, "FILTER o.status == 'completed'")
	assert.Contains(t, query, "COLLECT city = o.city AGGREGATE total_sales = SUM(o.amount)")
	assert.Contains(t, query, "SORT total_sales DESC")
	assert.Contains(t, query, "RETURN { city, total_sales }")
}

func TestAggregationTemplate_NoFilter(t *testing.T) {
	query := AggregationTemplate(
		"sales", "s",
		"",
		"region = s.region",
		"revenue = SUM(s.revenue)",
		"", SortAsc,
		"{ region, revenue }",
	)

	assert.NotContains(t, query, "FILTER")
	assert.Contains(t, query, "COLLECT region = s.region AGGREGATE revenue = SUM(s.revenue)")
}

func TestVectorSearchTemplate(t *testing.T) {
	query := VectorSearchTemplate("documents", "doc", "SIMILARITY(doc.embedding, @query_vector, 10)", "doc")

	assert.Contains(t, query, "FOR doc IN documents")
	assert.Contains(t, query, "FILTER SIMILARITY(doc.embedding, @query_vector, 10)")
	assert.Contains(t, query, "RETURN doc")
}

func TestInsertTemplate(t *testing.T) {
	query := InsertTemplate("{ name: @name, age: @age }", "users")
	assert.Equal(t, "INSERT { name: @name, age: @age } INTO users", query)
}

func TestUpdateTemplate(t *testing.T) {
	query := UpdateTemplate("@key", "users", "{ status: @status }")
	assert.Equal(t, "UPDATE @key WITH { status: @status } IN users", query)
}

func TestReplaceTemplate(t *testing.T) {
	query := ReplaceTemplate("@key", "users", "{ name: @name }")
	assert.Equal(t, "REPLACE @key WITH { name: @name } IN users", query)
}

func TestDeleteTemplate(t *testing.T) {
	query := DeleteTemplate("{ _key: @key }", "users")
	assert.Equal(t, "REMOVE { _key: @key } IN users", query)
}

func TestUpsertTemplate(t *testing.T) {
	query := UpsertTemplate(
		"{ email: @email }",
		"{ email: @email, name: @name, created: DATE_NOW() }",
		"{ name: @name }",
		"users",
	)

	assert.Contains(t, query, "UPSERT { email: @email }")
	assert.Contains(t, query, "INSERT { email: @email, name: @name, created: DATE_NOW() }")
	assert.Contains(t, query, "UPDATE { name: @name }")
	assert.Contains(t, query, "IN users")
}

func TestLLMInferTemplate_WithModelAndOptions(t *testing.T) {
	query := LLMInferTemplate("Explain databases", "llama-2-7b", "{ max_tokens: 200, temperature: 0.7 }")

	assert.Contains(t, query, `LLM INFER "Explain databases"`)
	assert.Contains(t, query, `USING MODEL "llama-2-7b"`)
	assert.Contains(t, query, "OPTIONS { max_tokens: 200, temperature: 0.7 }")
}

func TestLLMInferTemplate_PromptOnly(t *testing.T) {
	query := LLMInferTemplate("Hello world", "", "")

	assert.Contains(t, query, `LLM INFER "Hello world"`)
	assert.NotContains(t, query, "USING MODEL")
	assert.NotContains(t, query, "OPTIONS")
}

func TestLLMRagTemplate_WithLora(t *testing.T) {
	query := LLMRagTemplate("What are key features?", "documents", 5, "technical-docs")

	assert.Contains(t, query, `LLM RAG "What are key features?"`)
	assert.Contains(t, query, "FROM COLLECTION documents")
	assert.Contains(t, query, "TOP 5")
	assert.Contains(t, query, `USING LORA "technical-docs"`)
}

func TestLLMRagTemplate_NoLora(t *testing.T) {
	query := LLMRagTemplate("Summarize this", "articles", 3, "")

	assert.Contains(t, query, "FROM COLLECTION articles")
	assert.Contains(t, query, "TOP 3")
	assert.NotContains(t, query, "USING LORA")
}

func TestLLMEmbedTemplate_WithModel(t *testing.T) {
	query := LLMEmbedTemplate("Hello ThemisDB", "all-minilm")

	assert.Contains(t, query, `LLM EMBED "Hello ThemisDB"`)
	assert.Contains(t, query, `USING MODEL "all-minilm"`)
}

func TestLLMEmbedTemplate_NoModel(t *testing.T) {
	query := LLMEmbedTemplate("some text", "")

	assert.Contains(t, query, `LLM EMBED "some text"`)
	assert.NotContains(t, query, "USING MODEL")
}

func TestPaginatedQueryTemplate(t *testing.T) {
	query := PaginatedQueryTemplate("products", "p", "p.active == true", "p.name", SortAsc, 20, 10)

	assert.Contains(t, query, "FOR p IN products")
	assert.Contains(t, query, "FILTER p.active == true")
	assert.Contains(t, query, "SORT p.name ASC")
	assert.Contains(t, query, "LIMIT 20, 10")
	assert.Contains(t, query, "RETURN p")
}

func TestPaginatedQueryTemplate_FirstPage(t *testing.T) {
	query := PaginatedQueryTemplate("products", "p", "", "p.price", SortDesc, 0, 25)

	assert.Contains(t, query, "LIMIT 25")
	assert.NotContains(t, query, "LIMIT 0, 25")
}

// ─── Constants tests ──────────────────────────────────────────────────────────

func TestSortDirectionConstants(t *testing.T) {
	assert.Equal(t, SortDirection("ASC"), SortAsc)
	assert.Equal(t, SortDirection("DESC"), SortDesc)
}

func TestTraversalDirectionConstants(t *testing.T) {
	assert.Equal(t, TraversalDirection("OUTBOUND"), TraversalOutbound)
	assert.Equal(t, TraversalDirection("INBOUND"), TraversalInbound)
	assert.Equal(t, TraversalDirection("ANY"), TraversalAny)
}
