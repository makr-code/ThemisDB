# JSON Path Query Functions in ThemisDB

## Overview

This document describes the new JSON path query functions added to ThemisDB, enabling powerful JSON document manipulation directly in AQL queries.

## Functions

### JSON_EXTRACT(document, path)

Extract a value from a JSON document using a JSONPath expression.

**Syntax:**
```aql
JSON_EXTRACT(document, path)
```

**Parameters:**
- `document`: JSON object or array
- `path`: JSONPath string (e.g., `"$.field.nested[0]"`)

**Returns:** The value at the specified path, or `null` if not found

**Examples:**
```aql
-- Extract a simple field
FOR doc IN users
  RETURN JSON_EXTRACT(doc, "$.profile.name")

-- Extract from nested arrays
FOR order IN orders
  RETURN JSON_EXTRACT(order, "$.items[0].product.name")

-- Extract in filter
FOR doc IN products
  FILTER JSON_EXTRACT(doc, "$.price") > 100
  RETURN doc
```

### JSON_SET(document, path, value)

Set a value in a JSON document at the specified path.

**Syntax:**
```aql
JSON_SET(document, path, value)
```

**Parameters:**
- `document`: JSON object or array
- `path`: JSONPath string
- `value`: Any value to set

**Returns:** Modified JSON document

**Examples:**
```aql
-- Add a new field
FOR doc IN users
  LET updated = JSON_SET(doc, "$.lastLogin", DATE_NOW())
  RETURN updated

-- Update nested field
FOR doc IN products
  LET discounted = JSON_SET(doc, "$.pricing.discount", 0.15)
  RETURN discounted

-- Create nested structure
FOR doc IN documents
  LET enhanced = JSON_SET(doc, "$.metadata.tags[0]", "important")
  RETURN enhanced
```

### JSON_REMOVE(document, path)

Remove a value from a JSON document at the specified path.

**Syntax:**
```aql
JSON_REMOVE(document, path)
```

**Parameters:**
- `document`: JSON object or array
- `path`: JSONPath string

**Returns:** Modified JSON document

**Examples:**
```aql
-- Remove a field
FOR doc IN users
  LET cleaned = JSON_REMOVE(doc, "$.internal.tempData")
  RETURN cleaned

-- Remove array element
FOR doc IN lists
  LET trimmed = JSON_REMOVE(doc, "$.items[0]")
  RETURN trimmed
```

### JSON_TYPE(document, path)

Get the type of a value at the specified path.

**Syntax:**
```aql
JSON_TYPE(document, path)
```

**Returns:** String: `"null"`, `"boolean"`, `"integer"`, `"number"`, `"string"`, `"array"`, or `"object"`

**Examples:**
```aql
-- Filter by type
FOR doc IN data
  FILTER JSON_TYPE(doc, "$.value") == "array"
  RETURN doc

-- Type checking in conditions
FOR doc IN mixed
  LET type = JSON_TYPE(doc, "$.field")
  RETURN {doc: doc, type: type}
```

### JSON_CONTAINS(document, value)

Check if a JSON document contains a specific value (recursive search).

**Syntax:**
```aql
JSON_CONTAINS(document, value)
```

**Returns:** Boolean

**Examples:**
```aql
-- Find documents containing a value
FOR doc IN logs
  FILTER JSON_CONTAINS(doc, "ERROR")
  RETURN doc

-- Search for specific ID
FOR doc IN records
  FILTER JSON_CONTAINS(doc, 12345)
  RETURN doc
```

### JSON_DEPTH(document)

Get the maximum depth of a JSON structure.

**Syntax:**
```aql
JSON_DEPTH(document)
```

**Returns:** Integer (0 for primitives, 1+ for nested structures)

**Examples:**
```aql
-- Find deeply nested documents
FOR doc IN data
  FILTER JSON_DEPTH(doc) > 5
  RETURN doc

-- Get depth statistics
FOR doc IN collection
  COLLECT depth = JSON_DEPTH(doc) WITH COUNT INTO count
  RETURN {depth: depth, count: count}
```

### JSON_PARSE(json_string)

Parse a JSON string into a JSON object.

**Syntax:**
```aql
JSON_PARSE(json_string)
```

**Examples:**
```aql
-- Parse stored JSON strings
FOR doc IN raw_data
  LET parsed = JSON_PARSE(doc.json_field)
  RETURN parsed

-- Parse and extract
FOR doc IN strings
  LET obj = JSON_PARSE(doc.data)
  LET value = JSON_EXTRACT(obj, "$.field")
  RETURN value
```

### JSON_STRINGIFY(value)

Convert a value to a JSON string.

**Syntax:**
```aql
JSON_STRINGIFY(value)
```

**Examples:**
```aql
-- Convert objects to strings
FOR doc IN data
  RETURN JSON_STRINGIFY(doc)

-- Serialize for storage
FOR doc IN temp
  LET serialized = JSON_STRINGIFY(doc.complex_field)
  RETURN {id: doc.id, data: serialized}
```

## JSONPath Syntax

ThemisDB supports a subset of JSONPath syntax:

- `$` - Root element
- `.field` - Object field access
- `[index]` - Array index access (0-based)
- Combinations like `$.a.b[0].c`

**Supported Patterns:**
```
$.field              # Simple field
$.nested.field       # Nested fields
$.array[0]           # Array element
$.field[0].nested    # Mixed access
$.a.b.c.d.e         # Deep nesting
```

**Not yet supported:**
- Wildcards (`$.*`, `$.field[*]`)
- Recursive descent (`$..field`)
- Filter expressions (`$[?(@.price < 10)]`)
- Array slicing (`$[0:5]`)
- Negative indices (`$[-1]`)

## Integration with Existing Functions

JSON path functions work seamlessly with existing document functions:

```aql
-- Combine with MERGE
FOR doc IN users
  LET extracted = JSON_EXTRACT(doc, "$.profile")
  LET merged = MERGE(extracted, {lastAccess: DATE_NOW()})
  RETURN merged

-- Combine with KEEP/UNSET
FOR doc IN data
  LET nested = JSON_EXTRACT(doc, "$.complex")
  LET kept = KEEP(nested, ["id", "name", "status"])
  RETURN kept

-- Combine with HAS
FOR doc IN records
  FILTER HAS(JSON_EXTRACT(doc, "$.metadata"), "version")
  RETURN doc
```

## Use Cases

### 1. Dynamic Field Access

```aql
-- Query with user-specified field path
LET fieldPath = "$.user.preferences.theme"
FOR doc IN settings
  RETURN {
    id: doc.id,
    value: JSON_EXTRACT(doc, fieldPath)
  }
```

### 2. Data Migration

```aql
-- Restructure documents
FOR doc IN old_schema
  LET migrated = JSON_SET(doc, "$.v2.userData", JSON_EXTRACT(doc, "$.user"))
  LET cleaned = JSON_REMOVE(migrated, "$.user")
  RETURN cleaned
```

### 3. Complex Filtering

```aql
-- Multi-level filtering
FOR doc IN products
  FILTER JSON_TYPE(doc, "$.metadata") == "object"
  FILTER JSON_CONTAINS(JSON_EXTRACT(doc, "$.tags"), "featured")
  FILTER JSON_EXTRACT(doc, "$.pricing.amount") > 50
  RETURN doc
```

### 4. Document Analysis

```aql
-- Analyze document structure
FOR doc IN collection
  RETURN {
    id: doc.id,
    depth: JSON_DEPTH(doc),
    hasAddress: JSON_EXTRACT(doc, "$.address") != null,
    addressType: JSON_TYPE(doc, "$.address")
  }
```

### 5. Conditional Updates

```aql
-- Update based on nested conditions
FOR doc IN users
  LET shouldUpdate = JSON_EXTRACT(doc, "$.subscription.status") == "expired"
  LET updated = shouldUpdate ? 
    JSON_SET(doc, "$.subscription.renewalRequired", true) : doc
  RETURN updated
```

## Performance Considerations

1. **Index Usage**: JSON path functions do not automatically use indexes. For frequently queried paths, consider:
   - Creating a computed field with the extracted value
   - Adding an index on that computed field

2. **Deep Nesting**: Extracting from deeply nested structures has a cost proportional to the depth. Consider denormalizing frequently accessed data.

3. **Caching**: Extracted values in `LET` clauses are computed once per document:
   ```aql
   FOR doc IN large_collection
     LET price = JSON_EXTRACT(doc, "$.nested.deep.price")
     FILTER price > 100
     RETURN {id: doc.id, price: price}  -- Reuses cached value
   ```

## Error Handling

- Invalid JSON paths throw a runtime error
- Missing paths return `null` (not an error)
- Type mismatches (e.g., field access on array) return `null`
- JSON_PARSE throws an error on invalid JSON strings

## Migration from Existing Code

If you were using field access notation:

```aql
-- Old way (limited)
FOR doc IN users
  FILTER doc.profile.name == "Alice"
  RETURN doc

-- New way (flexible)
FOR doc IN users
  FILTER JSON_EXTRACT(doc, "$.profile.name") == "Alice"
  RETURN doc
```

## See Also

- [Document Functions Reference](./document_functions.md)
- [Array Functions Reference](./array_functions.md)
- [AQL Syntax Guide](../aql_syntax.md)
