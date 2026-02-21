package themisdb

import (
	"fmt"
	"strings"
)

// SortDirection specifies the sort order in an AQL query.
type SortDirection string

const (
	// SortAsc sorts results in ascending order.
	SortAsc SortDirection = "ASC"
	// SortDesc sorts results in descending order.
	SortDesc SortDirection = "DESC"
)

// TraversalDirection specifies the edge direction for graph traversals.
type TraversalDirection string

const (
	// TraversalOutbound follows outgoing edges.
	TraversalOutbound TraversalDirection = "OUTBOUND"
	// TraversalInbound follows incoming edges.
	TraversalInbound TraversalDirection = "INBOUND"
	// TraversalAny follows edges in either direction.
	TraversalAny TraversalDirection = "ANY"
)

// AQLQueryBuilder provides a fluent interface for building AQL queries.
//
// Example usage:
//
//	query := NewAQLQuery().
//	    For("u", "users").
//	    Filter("u.age > 18").
//	    Sort("u.name", SortAsc).
//	    Limit(0, 10).
//	    Return("u").
//	    Build()
type AQLQueryBuilder struct {
	forClauses     []string
	letClauses     []string
	filterClauses  []string
	collectClause  string
	sortClauses    []string
	limitClause    string
	returnClause   string
}

// NewAQLQuery creates a new AQLQueryBuilder.
func NewAQLQuery() *AQLQueryBuilder {
	return &AQLQueryBuilder{}
}

// For adds a FOR clause iterating over a collection.
func (b *AQLQueryBuilder) For(variable, collection string) *AQLQueryBuilder {
	b.forClauses = append(b.forClauses, fmt.Sprintf("FOR %s IN %s", variable, collection))
	return b
}

// ForRange adds a FOR clause iterating over a numeric range.
func (b *AQLQueryBuilder) ForRange(variable string, start, end int) *AQLQueryBuilder {
	b.forClauses = append(b.forClauses, fmt.Sprintf("FOR %s IN %d..%d", variable, start, end))
	return b
}

// ForTraversal adds a FOR clause performing a graph traversal.
func (b *AQLQueryBuilder) ForTraversal(vertexVar, edgeVar, pathVar string, minDepth, maxDepth int, direction TraversalDirection, startVertex string, edgeCollections ...string) *AQLQueryBuilder {
	vars := vertexVar
	if edgeVar != "" {
		vars += ", " + edgeVar
	}
	if pathVar != "" {
		vars += ", " + pathVar
	}
	clause := fmt.Sprintf("FOR %s IN %d..%d %s %s %s",
		vars, minDepth, maxDepth, direction, startVertex, strings.Join(edgeCollections, ", "))
	b.forClauses = append(b.forClauses, clause)
	return b
}

// Let adds a LET clause binding a variable to an expression.
func (b *AQLQueryBuilder) Let(variable, expression string) *AQLQueryBuilder {
	b.letClauses = append(b.letClauses, fmt.Sprintf("LET %s = %s", variable, expression))
	return b
}

// Filter adds a FILTER clause with the given condition.
func (b *AQLQueryBuilder) Filter(condition string) *AQLQueryBuilder {
	b.filterClauses = append(b.filterClauses, fmt.Sprintf("FILTER %s", condition))
	return b
}

// Collect adds a COLLECT clause for grouping and optional aggregation.
// Pass an empty aggregation string to omit AGGREGATE.
func (b *AQLQueryBuilder) Collect(groupExpr, aggregation string) *AQLQueryBuilder {
	if aggregation != "" {
		b.collectClause = fmt.Sprintf("COLLECT %s AGGREGATE %s", groupExpr, aggregation)
	} else {
		b.collectClause = fmt.Sprintf("COLLECT %s", groupExpr)
	}
	return b
}

// Sort adds a SORT clause on the given field with the specified direction.
func (b *AQLQueryBuilder) Sort(field string, direction SortDirection) *AQLQueryBuilder {
	b.sortClauses = append(b.sortClauses, fmt.Sprintf("%s %s", field, direction))
	return b
}

// Limit adds a LIMIT clause. Use offset=0 to omit the offset.
func (b *AQLQueryBuilder) Limit(offset, count int) *AQLQueryBuilder {
	if offset > 0 {
		b.limitClause = fmt.Sprintf("LIMIT %d, %d", offset, count)
	} else {
		b.limitClause = fmt.Sprintf("LIMIT %d", count)
	}
	return b
}

// Return sets the RETURN clause projection.
func (b *AQLQueryBuilder) Return(projection string) *AQLQueryBuilder {
	b.returnClause = fmt.Sprintf("RETURN %s", projection)
	return b
}

// ReturnDistinct sets a RETURN DISTINCT clause projection.
func (b *AQLQueryBuilder) ReturnDistinct(projection string) *AQLQueryBuilder {
	b.returnClause = fmt.Sprintf("RETURN DISTINCT %s", projection)
	return b
}

// Build assembles and returns the complete AQL query string.
func (b *AQLQueryBuilder) Build() string {
	var parts []string
	parts = append(parts, b.forClauses...)
	parts = append(parts, b.letClauses...)
	parts = append(parts, b.filterClauses...)
	if b.collectClause != "" {
		parts = append(parts, b.collectClause)
	}
	if len(b.sortClauses) > 0 {
		parts = append(parts, "SORT "+strings.Join(b.sortClauses, ", "))
	}
	if b.limitClause != "" {
		parts = append(parts, b.limitClause)
	}
	if b.returnClause != "" {
		parts = append(parts, b.returnClause)
	}
	return strings.Join(parts, "\n")
}

// ─── Pre-built template helpers ──────────────────────────────────────────────

// SimpleQueryTemplate builds a basic FOR/FILTER/SORT/LIMIT/RETURN query.
// Pass empty strings / zero values to omit optional clauses.
func SimpleQueryTemplate(collection, variable, filterCondition, sortField string, sortDir SortDirection, limit int) string {
	b := NewAQLQuery().For(variable, collection)
	if filterCondition != "" {
		b.Filter(filterCondition)
	}
	if sortField != "" {
		b.Sort(sortField, sortDir)
	}
	if limit > 0 {
		b.Limit(0, limit)
	}
	return b.Return(variable).Build()
}

// JoinQueryTemplate builds a nested-loop join between two collections.
// joinCondition is the FILTER predicate linking the two variables (e.g. "b.user_id == a._key").
// projection is the RETURN expression (e.g. `{ name: a.name, total: b.total }`).
func JoinQueryTemplate(collA, varA, collB, varB, joinCondition, projection string) string {
	return NewAQLQuery().
		For(varA, collA).
		For(varB, collB).
		Filter(joinCondition).
		Return(projection).
		Build()
}

// GraphTraversalTemplate builds a graph traversal query.
func GraphTraversalTemplate(vertexVar string, minDepth, maxDepth int, direction TraversalDirection, startVertex string, edgeCollection, returnExpr string) string {
	return NewAQLQuery().
		ForTraversal(vertexVar, "", "", minDepth, maxDepth, direction, startVertex, edgeCollection).
		Return(returnExpr).
		Build()
}

// AggregationTemplate builds a COLLECT/AGGREGATE query.
// groupExpr example: `city = doc.city`
// aggregation example: `total_sales = SUM(doc.amount)`
func AggregationTemplate(collection, variable, filterCondition, groupExpr, aggregation, sortField string, sortDir SortDirection, returnExpr string) string {
	b := NewAQLQuery().For(variable, collection)
	if filterCondition != "" {
		b.Filter(filterCondition)
	}
	b.Collect(groupExpr, aggregation)
	if sortField != "" {
		b.Sort(sortField, sortDir)
	}
	return b.Return(returnExpr).Build()
}

// VectorSearchTemplate builds a vector similarity search query.
// similarityExpr is the SIMILARITY call, e.g. `SIMILARITY(doc.embedding, @query_vector, 10)`.
func VectorSearchTemplate(collection, variable, similarityExpr, returnExpr string) string {
	return NewAQLQuery().
		For(variable, collection).
		Filter(similarityExpr).
		Return(returnExpr).
		Build()
}

// InsertTemplate builds an INSERT statement.
// docExpr is the document expression, e.g. `{ name: @name, age: @age }`.
func InsertTemplate(docExpr, collection string) string {
	return fmt.Sprintf("INSERT %s INTO %s", docExpr, collection)
}

// UpdateTemplate builds an UPDATE statement.
// keyExpr is the document key expression, e.g. `@key`.
// updateExpr is the update object, e.g. `{ status: @status }`.
func UpdateTemplate(keyExpr, collection, updateExpr string) string {
	return fmt.Sprintf("UPDATE %s WITH %s IN %s", keyExpr, updateExpr, collection)
}

// ReplaceTemplate builds a REPLACE statement.
func ReplaceTemplate(keyExpr, collection, docExpr string) string {
	return fmt.Sprintf("REPLACE %s WITH %s IN %s", keyExpr, docExpr, collection)
}

// DeleteTemplate builds a REMOVE statement.
// keyExpr is the document key expression, e.g. `@key` or `{ _key: @key }`.
func DeleteTemplate(keyExpr, collection string) string {
	return fmt.Sprintf("REMOVE %s IN %s", keyExpr, collection)
}

// UpsertTemplate builds an UPSERT statement.
// searchExpr is used to find the document, e.g. `{ email: @email }`.
// insertExpr is the document inserted when not found.
// updateExpr is the object merged when found.
func UpsertTemplate(searchExpr, insertExpr, updateExpr, collection string) string {
	return fmt.Sprintf("UPSERT %s INSERT %s UPDATE %s IN %s",
		searchExpr, insertExpr, updateExpr, collection)
}

// LLMInferTemplate builds an LLM INFER statement.
// model and options are optional; pass empty strings to omit them.
func LLMInferTemplate(prompt, model, options string) string {
	query := fmt.Sprintf("LLM INFER %q", prompt)
	if model != "" {
		query += fmt.Sprintf("\n  USING MODEL %q", model)
	}
	if options != "" {
		query += fmt.Sprintf("\n  OPTIONS %s", options)
	}
	return query
}

// LLMRagTemplate builds an LLM RAG statement.
// loraAdapter is optional; pass an empty string to omit it.
func LLMRagTemplate(prompt, collection string, topN int, loraAdapter string) string {
	query := fmt.Sprintf("LLM RAG %q\n  FROM COLLECTION %s\n  TOP %d", prompt, collection, topN)
	if loraAdapter != "" {
		query += fmt.Sprintf("\n  USING LORA %q", loraAdapter)
	}
	return query
}

// LLMEmbedTemplate builds an LLM EMBED statement.
func LLMEmbedTemplate(text, model string) string {
	query := fmt.Sprintf("LLM EMBED %q", text)
	if model != "" {
		query += fmt.Sprintf("\n  USING MODEL %q", model)
	}
	return query
}

// PaginatedQueryTemplate builds a query with offset-based pagination.
func PaginatedQueryTemplate(collection, variable, filterCondition, sortField string, sortDir SortDirection, offset, pageSize int) string {
	b := NewAQLQuery().For(variable, collection)
	if filterCondition != "" {
		b.Filter(filterCondition)
	}
	if sortField != "" {
		b.Sort(sortField, sortDir)
	}
	b.Limit(offset, pageSize)
	return b.Return(variable).Build()
}
