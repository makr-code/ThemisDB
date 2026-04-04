# AQL Auto-Complete API (LSP-Compatible)

**Category:** 🛠️ Developer Tools  
**Version:** v1.5.0  
**Status:** ✅ Available  
**Issue:** #1359

---

## Overview

ThemisDB provides a built-in AQL auto-complete engine that delivers
**Language Server Protocol (LSP)-compatible** completion items.  Editor
extensions (VS Code, Neovim, JetBrains, etc.) can query this engine to offer
interactive completions while users write AQL queries.

The engine is **purely rule-based** and requires no LLM connection.

---

## Quick Start

```cpp
#include "aql/aql_autocomplete.h"

using namespace themis::aql;

AQLAutoComplete ac;

CompletionContext ctx;
ctx.query_text     = "FOR u IN users FILT";
ctx.cursor_offset  = ctx.query_text.size();   // cursor at end
ctx.schema_context = "collection: users(id, name, age, email)";

auto items = ac.complete(ctx);
for (const auto& item : items) {
    std::cout << item.label << "  [" << item.detail << "]\n";
}
// → FILTER  [Filter results by a boolean expression]
```

---

## API Reference

### `CompletionContext`

| Field | Type | Description |
|-------|------|-------------|
| `query_text` | `string` | Full AQL text typed so far |
| `cursor_offset` | `size_t` | 0-based byte offset of cursor (`npos` = end of text) |
| `schema_context` | `string` | Optional: collection/field descriptions (see below) |
| `trigger_character` | `string` | Optional: character that triggered completion (e.g. `.`) |

### `CompletionItem`

| Field | Type | Description |
|-------|------|-------------|
| `label` | `string` | Text shown in the completion list |
| `kind` | `CompletionItemKind` | LSP-compatible kind enum (see below) |
| `detail` | `string` | Short one-line description / signature |
| `documentation` | `string` | Longer Markdown documentation (optional) |
| `insert_text` | `string` | Text to insert; may contain `${N:placeholder}` snippets |
| `prefix_start` | `size_t` | 0-based offset where the prefix to replace starts |
| `sort_order` | `int` | Sort key (lower = higher in list) |

### `CompletionItemKind` values

Values are **identical to the LSP specification** and can be forwarded
directly in LSP `CompletionList` responses.

| Name | Value | Used for |
|------|-------|----------|
| `Keyword` | 14 | AQL clause and modifier keywords |
| `Function` | 3 | Built-in AQL functions |
| `Variable` | 6 | Variables declared by `FOR`, `LET`, `COLLECT` |
| `Field` | 5 | Document fields (schema-aware, after `.`) |
| `Snippet` | 15 | Multi-placeholder snippets |

### `AQLAutoComplete` methods

| Method | Description |
|--------|-------------|
| `complete(ctx)` | Compute completion items for cursor context |
| `allKeywords()` | List all known AQL keywords (useful for syntax highlighting) |
| `allFunctions()` | List all known built-in function names |

---

## Schema Context Format

Pass a `schema_context` string so that attribute completions (after `.`) are
aware of collection fields.

```
collection: users(id, name, age, email), orders(id, user_id, total, status)
```

- `collection:` prefix is optional
- Multiple collections are comma-separated
- Field lists are enclosed in parentheses

When a variable is bound to a collection via `FOR var IN collection`, the
engine returns only that collection's fields after `var.`.  If the binding
cannot be resolved, all fields from all collections are returned as a union.

---

## What Gets Completed

### Keywords

All core AQL clause keywords are offered with **snippet insert text**
containing `${N:placeholder}` markers for tab-stop navigation:

| Keyword | Insert Text |
|---------|-------------|
| `FOR` | `FOR ${1:var} IN ${2:collection}` |
| `FILTER` | `FILTER ${1:condition}` |
| `SORT` | `SORT ${1:expr} ${2:ASC}` |
| `LIMIT` | `LIMIT ${1:count}` |
| `RETURN` | `RETURN ${1:expr}` |
| `LET` | `LET ${1:var} = ${2:expr}` |
| `COLLECT` | `COLLECT ${1:var} = ${2:expr}` |

Modifier keywords (`AND`, `OR`, `NOT`, `ASC`, `DESC`, `DISTINCT`, etc.) and
LLM-extension keywords (`LLM`, `INFER`, `RAG`, `EMBED`, `MODEL`, `LORA`) are
also included.

### Built-in Functions (40+)

Functions are completed with their parameter signature in `detail` and a
snippet insert text:

```
COUNT(${1})   →  detail: "COUNT(expr)"
SIMILARITY(${1})  →  detail: "SIMILARITY(vec1, vec2)"
ST_DISTANCE(${1}) →  detail: "ST_DISTANCE(geo1, geo2)"
```

Categories: aggregate, string, math, array, type conversion, JSON,
date/time, vector/similarity, geospatial, fulltext.

### Variables

Variables introduced by `FOR`, `LET`, and `COLLECT` that appear **before**
the cursor are included as `Variable`-kind completions and are ranked first.

```aql
FOR user IN users
COLLECT dept = user.department INTO deptGroup
RETURN use  -- completes: 'user', 'dept', 'deptGroup'
```

### Attributes (after `.`)

When the cursor is immediately after a dot (e.g. `u.`), the engine resolves
the variable to its collection and returns `Field`-kind items:

```aql
FOR u IN users RETURN u.na  -- completes: 'name'
```

---

## LSP Integration Example

```json
// textDocument/completion response
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "isIncomplete": false,
    "items": [
      {
        "label": "FILTER",
        "kind": 14,
        "detail": "Filter results by a boolean expression",
        "insertText": "FILTER ${1:condition}",
        "insertTextFormat": 2
      }
    ]
  }
}
```

Set `insertTextFormat: 2` (Snippet) in LSP responses when `insert_text`
contains `${N:...}` placeholders.

---

## Sort Order Policy

1. Declared variables (lowest sort_order, ranked first)
2. Clause keywords (`FOR`, `FILTER`, `RETURN`, …)
3. Modifier keywords (`AND`, `OR`, `ASC`, …)
4. LLM-extension keywords (`LLM`, `INFER`, `RAG`, …)
5. Built-in functions
6. Within each group: alphabetical by label

---

## Related

- [AQL Syntax Guide](AQL_SYNTAX_GUIDE.md)
- [AQL Functions Reference](aql_functions_reference.md)
- [AQL Query Templates](../../../docs/en/aql/aql_query_templates.md)
- [AQL Query Validator](aql_query_validator) (for validation before execution)
