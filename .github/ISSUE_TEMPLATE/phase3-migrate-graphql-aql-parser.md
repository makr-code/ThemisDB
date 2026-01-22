---
name: Phase 3 - Migrate GraphQL/AQL Parser to Result<T>
about: Migrate GraphQL and AQL parser methods to Result<T>
title: '[Phase 3] Migrate GraphQL/AQL Parser to Result<T>'
labels: ['enhancement', 'error-handling', 'phase-3', 'parser', 'graphql', 'aql']
assignees: ''
---

## 📋 Overview

Migrate GraphQL and AQL parser methods from legacy error patterns to `Result<T>` for better syntax error reporting and diagnostics.

**Current Status:** 0% complete  
**Target:** ~8 methods  
**Priority:** 🟡 Medium

## 🎯 Goals

- Provide detailed syntax error messages with line/column information
- Better error recovery during parsing
- Type-safe error propagation from parser to query engine

## 🔨 Methods to Migrate (~8 methods)

### GraphQL Parser
- [ ] `parseGraphQLQuery()` - Parse GraphQL query string
- [ ] `parseGraphQLSchema()` - Parse schema definition
- [ ] `validateGraphQLQuery()` - Validate parsed query
- [ ] `parseGraphQLFragments()` - Parse query fragments

### AQL Parser
- [ ] `parseAQLQuery()` - Parse AQL query string
- [ ] `parseAQLExpression()` - Parse expressions
- [ ] `validateAQLSyntax()` - Syntax validation
- [ ] `parseAQLOperators()` - Operator parsing

## 📝 Implementation Strategy

### Error Information to Capture

**Syntax Errors:**
- Line number
- Column number
- Token that caused error
- Expected tokens
- Suggestion for fix (if possible)

**Example Error:**
```
Query syntax error at line 3, column 15:
  query { users(id: 123) { name email } 
                ^
Expected: ')', got: '123'
Did you forget to quote the ID? Try: users(id: "123")
```

### Error Codes to Use
- `ERR_QUERY_SYNTAX_ERROR` - Parsing failed
- `ERR_QUERY_VALIDATION_FAILED` - Semantic validation failed
- `ERR_API_INVALID_REQUEST` - Malformed query request

## 📋 Implementation Checklist

### GraphQL Parser
- [ ] Update `include/query/graphql_parser.h` signatures
- [ ] Update `src/query/graphql_parser.cpp` implementations
- [ ] Add detailed error context (line, column, token)
- [ ] Update all call sites
- [ ] Update test files

### AQL Parser
- [ ] Update `include/query/aql_parser.h` signatures
- [ ] Update `src/query/aql_parser.cpp` implementations
- [ ] Add detailed error context
- [ ] Update all call sites
- [ ] Update test files

### Error Handling Improvements
- [ ] Create helper for line/column tracking
- [ ] Add syntax error formatter
- [ ] Implement "did you mean?" suggestions
- [ ] Add context snippets to errors

## 🧪 Testing Requirements

### Unit Tests
```cpp
TEST(GraphQLParserTest, SyntaxErrorWithLineColumn) {
    auto result = parser.parseGraphQLQuery("query { users(id: 123 { name } }");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_QUERY_SYNTAX_ERROR);
    EXPECT_THAT(result.error().message(), HasSubstr("line 1"));
    EXPECT_THAT(result.error().message(), HasSubstr("column 23"));
}

TEST(AQLParserTest, ValidationErrorWithSuggestion) {
    auto result = parser.parseAQLQuery("FOR u IN user RETURN u.name");
    EXPECT_FALSE(result.has_value());
    EXPECT_THAT(result.error().message(), HasSubstr("Did you mean 'users'?"));
}
```

### Integration Tests
- [ ] Test parser errors propagate to API layer
- [ ] Test error messages are user-friendly
- [ ] Test error recovery mechanisms
- [ ] Test performance impact (< 5% overhead)

## 📚 Documentation Updates

- [ ] Update parser documentation
- [ ] Document error message format
- [ ] Add examples of common parse errors
- [ ] Update query language documentation

### Error Message Guidelines

**Good Error Message:**
```
GraphQL syntax error at line 5, column 12:
  mutation { updateUser(id: "123", data: { name: "John" }) }
            ^
Missing opening brace '{' after mutation keyword.
Expected: mutation { ... }
```

**Bad Error Message:**
```
Parse error
```

## 🎯 Success Criteria

- [ ] All parser methods use `Result<T>`
- [ ] Error messages include line/column info
- [ ] Syntax errors provide helpful suggestions
- [ ] All tests pass
- [ ] Performance impact < 5%
- [ ] Documentation complete

## 📊 Progress Tracking

**Expected Effort:** 1-2 weeks  
**Priority:** Medium (improves developer experience)

### Weekly Goals
- [ ] Week 1: GraphQL parser (4 methods)
- [ ] Week 2: AQL parser (4 methods) + testing

## 🔗 Related

- **Parent Issue:** #XXX (Error Handling Migration - Master Tracking)
- **Query Engine Issue:** (depends on parser for error propagation)
- **Documentation:** ERROR_HANDLING_MIGRATION_STATUS.md

## 💡 Notes

- **User Experience:** Good error messages are critical for developer productivity
- **Error Recovery:** Consider implementing error recovery to find multiple errors
- **Performance:** Parsing is on hot path - monitor performance carefully
- **Internationalization:** Consider i18n for error messages in future
