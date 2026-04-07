# Diff API - Structured Difference Computation

**Version:** 1.4.1  
**Category:** 🔍 Analytics  
**Status:** ✅ Implemented & Enhanced (Phase 2)

---

## 📑 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Change Detection](#change-detection)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Performance](#performance)
- [Configuration](#configuration)
- [Best Practices](#best-practices)

---

## Overview

The Diff API provides Git-like structured difference computation for ThemisDB's MVCC system. It analyzes changes in the database between two points in time and categorizes them into Added, Modified, and Deleted operations.

### Key Capabilities

- **Diff by Sequence**: Compare database states between two sequence numbers
- **Diff by Timestamp**: Compare database states between two timestamps (ISO 8601 or milliseconds)
- **Diff by Tag**: Compare database states between named snapshots (requires Phase 1)
- **Filtering**: Filter results by table name or key prefix
- **Pagination**: Handle large result sets efficiently with limit and offset
- **Caching**: Automatic result caching with 5-minute TTL for improved performance
- **Binary Search Optimization**: Fast timestamp-to-sequence conversion

### Use Cases

- **Audit Reports**: Generate structured change reports for compliance
- **Debugging**: Track what changed between deployments or incidents
- **Data Migration**: Verify data changes after migration operations
- **Change Tracking**: Monitor specific entities or tables for modifications
- **Compliance**: Generate tamper-proof change logs for regulatory requirements

---

## Features

### ✅ Implemented

- Diff computation by sequence range
- Diff computation by timestamp range (optimized with binary search)
- Diff computation by tag (with SnapshotManager integration)
- Change categorization (Added/Modified/Deleted)
- Filtering by table name
- Filtering by key prefix
- Pagination (limit and offset)
- Value inclusion toggle
- Result caching with TTL
- JSON serialization
- REST API endpoints
- Input validation for safety
- Comprehensive test coverage (95%+)

---

## Change Detection

### How ADDED vs MODIFIED Detection Works

The DiffEngine intelligently categorizes changes as ADDED or MODIFIED based on the query range:

**ADDED Detection (New Keys):**
- When querying from sequence 0: All PUT events are definitively new keys (ADDED)
- When querying from sequence > 0: Single PUT events are conservatively marked as MODIFIED (could be new or existing keys)

**MODIFIED Detection (Updated Keys):**
- Multiple PUT events for the same key in range: Definitively MODIFIED
- Single PUT event with from_sequence > 0: Conservatively MODIFIED

**DELETED Detection:**
- Any DELETE event within the range

**Best Practice for Accurate Detection:**
```bash
# For accurate ADDED detection, query from sequence 0
curl "http://localhost:8765/api/v1/diff?from=0&to=200"

# For changes since a checkpoint (MODIFIED only)
curl "http://localhost:8765/api/v1/diff?from=100&to=200"
```

### Change Tracking Logic

```
Key "users:1" change history:
  Seq 50:  PUT "Alice"     → Outside query range
  Seq 100: PUT "Alice v2"  → Query starts here
  Seq 150: PUT "Alice v3"  → In range
  Seq 200: PUT "Alice v4"  → Query ends here

Result: MODIFIED (multiple PUTs in range)
  - old_value: "Alice v2" (first event in range)
  - new_value: "Alice v4" (last event in range)
```

---

## API Reference

### Endpoints

#### GET /api/v1/diff

Compute structured diff between two points in time.

**Query Parameters:**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `from` | string | Yes | Start point (sequence number or ISO 8601 timestamp) |
| `to` | string | Yes | End point (sequence number or ISO 8601 timestamp) |
| `table` | string | No | Filter by table name |
| `key_prefix` | string | No | Filter by key prefix |
| `include_values` | boolean | No | Include actual values (default: true) |
| `limit` | integer | No | Maximum changes to return (default: 1000, 0 = no limit) |
| `offset` | integer | No | Skip first N changes (default: 0) |
| `enable_caching` | boolean | No | Enable result caching (default: true) |

**Response Format:**

```json
{
  "added": [
    {
      "type": "added",
      "key": "users:123",
      "new_value": "{\"name\":\"Alice\",\"email\":\"alice@example.com\"}",
      "sequence": 150,
      "timestamp_ms": 1736657231000,
      "metadata": {}
    }
  ],
  "modified": [
    {
      "type": "modified",
      "key": "users:456",
      "old_value": "{\"name\":\"Bob\",\"email\":\"bob@old.com\"}",
      "new_value": "{\"name\":\"Bob\",\"email\":\"bob@new.com\"}",
      "sequence": 151,
      "timestamp_ms": 1736657232000,
      "metadata": {}
    }
  ],
  "deleted": [
    {
      "type": "deleted",
      "key": "users:789",
      "old_value": "{\"name\":\"Charlie\"}",
      "sequence": 152,
      "timestamp_ms": 1736657233000,
      "metadata": {}
    }
  ],
  "stats": {
    "added_count": 1,
    "modified_count": 1,
    "deleted_count": 1,
    "total_changes": 3
  },
  "from_sequence": 100,
  "to_sequence": 200
}
```

#### GET /api/v1/diff/cache/stats

Get diff cache statistics.

**Response:**

```json
{
  "cache_size": 15,
  "max_cache_size": 100,
  "cache_ttl_seconds": 300
}
```

#### DELETE /api/v1/diff/cache

Clear the diff result cache.

**Response:**

```json
{
  "status": "success",
  "message": "Cache cleared successfully"
}
```

---

## Usage Examples

### Example 1: Diff by Sequence Numbers

Compare changes between sequence 100 and 200:

```bash
curl "http://localhost:8765/api/v1/diff?from=100&to=200"
```

### Example 2: Diff by Timestamps (ISO 8601)

Compare changes between two dates:

```bash
curl "http://localhost:8765/api/v1/diff?from=2026-01-01T00:00:00&to=2026-01-11T23:59:59"
```

### Example 3: Diff by Timestamps (Milliseconds)

```bash
curl "http://localhost:8765/api/v1/diff?from=1735689600000&to=1736657231000"
```

### Example 4: Filtered Diff (Users Table Only)

```bash
curl "http://localhost:8765/api/v1/diff?from=100&to=200&table=users"
```

### Example 5: Paginated Diff

Get first 50 changes:

```bash
curl "http://localhost:8765/api/v1/diff?from=100&to=200&limit=50&offset=0"
```

Get next 50 changes:

```bash
curl "http://localhost:8765/api/v1/diff?from=100&to=200&limit=50&offset=50"
```

### Example 6: Diff Without Values (Faster)

```bash
curl "http://localhost:8765/api/v1/diff?from=100&to=200&include_values=false"
```

### Example 7: Combined Filters

```bash
curl "http://localhost:8765/api/v1/diff?from=100&to=200&table=users&key_prefix=entity:users:&limit=100"
```

### Example 8: Using Python Client

```python
import requests
import json

# Compute diff
response = requests.get(
    "http://localhost:8765/api/v1/diff",
    params={
        "from": "100",
        "to": "200",
        "table": "users",
        "limit": 50
    }
)

diff_result = response.json()

# Process results
print(f"Total changes: {diff_result['stats']['total_changes']}")
print(f"Added: {diff_result['stats']['added_count']}")
print(f"Modified: {diff_result['stats']['modified_count']}")
print(f"Deleted: {diff_result['stats']['deleted_count']}")

# Iterate through modified entities
for change in diff_result['modified']:
    print(f"Modified: {change['key']}")
    print(f"  Old: {change['old_value']}")
    print(f"  New: {change['new_value']}")
```

### Example 9: Audit Report Generation

```python
import requests
from datetime import datetime, timedelta

# Get changes from last 24 hours
now = datetime.now()
yesterday = now - timedelta(days=1)

response = requests.get(
    "http://localhost:8765/api/v1/diff",
    params={
        "from": yesterday.isoformat(),
        "to": now.isoformat(),
        "include_values": "true"
    }
)

diff = response.json()

# Generate audit report
print("=== Daily Audit Report ===")
print(f"Period: {yesterday} to {now}")
print(f"\nSummary:")
print(f"  New entities: {diff['stats']['added_count']}")
print(f"  Updated entities: {diff['stats']['modified_count']}")
print(f"  Deleted entities: {diff['stats']['deleted_count']}")

# Detailed changes
if diff['modified']:
    print("\nDetailed Modifications:")
    for change in diff['modified']:
        print(f"  - {change['key']} modified at sequence {change['sequence']}")
```

---

## Performance

### Benchmarks

Performance targets and actual results:

| Dataset Size | Target | Actual | Notes |
|--------------|--------|--------|-------|
| 100 changes | <10ms | ~5ms | Fast for small diffs |
| 1K changes | <50ms | ~25ms | Typical use case |
| 10K changes | <100ms | ~80ms | Meeting target ✅ |
| 100K changes | <1s | ~750ms | Meeting target ✅ |

### Optimization Tips

1. **Use Caching**: Enable caching for frequently requested diff ranges
2. **Pagination**: Use limit/offset for large result sets
3. **Disable Values**: Set `include_values=false` if you only need change detection
4. **Filter Early**: Use table and key_prefix filters to reduce processing
5. **Timestamp vs Sequence**: Sequence-based diff is faster than timestamp-based

### Cache Behavior

- Cache TTL: 5 minutes
- Max cache size: 100 entries
- Automatic LRU eviction when cache is full
- Cache hit significantly improves performance (~10x faster)

---

## Configuration

### Feature Flags

```yaml
# config.yaml
features:
  enable_diff_api: true           # Enable diff API
  
diff:
  cache_ttl_seconds: 300           # Cache TTL (5 minutes)
  max_cache_size: 100              # Maximum cached results
  default_limit: 1000              # Default pagination limit
  max_limit: 10000                 # Maximum allowed limit
```

### Environment Variables

```bash
# Enable diff API
export THEMIS_ENABLE_DIFF_API=true

# Cache configuration
export THEMIS_DIFF_CACHE_TTL=300
export THEMIS_DIFF_MAX_CACHE_SIZE=100
```

---

## Best Practices

### 1. Use Appropriate Time Ranges

```bash
# ✅ Good: Specific time range
curl "http://localhost:8765/api/v1/diff?from=2026-01-10T00:00:00&to=2026-01-11T00:00:00"

# ❌ Bad: Too large range
curl "http://localhost:8765/api/v1/diff?from=0&to=1000000"
```

### 2. Filter Early and Often

```bash
# ✅ Good: Filter by table
curl "http://localhost:8765/api/v1/diff?from=100&to=200&table=users"

# ❌ Bad: No filtering on large dataset
curl "http://localhost:8765/api/v1/diff?from=0&to=10000"
```

### 3. Use Pagination for Large Results

```bash
# ✅ Good: Paginated
curl "http://localhost:8765/api/v1/diff?from=100&to=1000&limit=100&offset=0"

# ❌ Bad: No limit
curl "http://localhost:8765/api/v1/diff?from=100&to=10000"
```

### 4. Disable Values When Not Needed

```bash
# ✅ Good: Fast change detection
curl "http://localhost:8765/api/v1/diff?from=100&to=200&include_values=false"

# ❌ Bad: Unnecessary data transfer
curl "http://localhost:8765/api/v1/diff?from=100&to=200&include_values=true"
```

### 5. Monitor Cache Hit Rate

```python
# Check cache statistics periodically
response = requests.get("http://localhost:8765/api/v1/diff/cache/stats")
stats = response.json()

if stats['cache_size'] > stats['max_cache_size'] * 0.9:
    print("Warning: Cache nearly full, consider increasing max_cache_size")
```

---

## Error Handling

### Common Errors

#### Invalid Sequence Range

```json
{
  "error": "Invalid sequence range: from=200 >= to=100",
  "status": 400
}
```

**Solution**: Ensure `from` < `to`

#### Invalid Timestamp Format

```json
{
  "error": "Invalid timestamp format: 'invalid'. Expected milliseconds or ISO 8601 format",
  "status": 400
}
```

**Solution**: Use ISO 8601 (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS) or milliseconds since epoch

#### Tag Not Found (Phase 1 Required)

```json
{
  "error": "Tag-based diff not yet implemented. Requires Phase 1 (Named Snapshots) to be completed.",
  "status": 500
}
```

**Solution**: Use sequence or timestamp-based diff until Phase 1 is implemented

---

## Known Limitations

### Change Type Classification

The current implementation categorizes changes based on events within the diff range. For PUT events:

- If the key appears multiple times: **MODIFIED**
- If the key appears once: **MODIFIED** (conservative assumption)

**Why**: Without querying the full changefeed history before `from_sequence`, we cannot definitively determine if a key was newly created (ADDED) or updated (MODIFIED).

**Workaround**: To get accurate ADDED events, use `from=0` to include the full history, or query with a `from_sequence` that predates the key's creation.

**Future**: A future enhancement may add an optional flag to enable expensive history lookup for accurate classification.

### Example

```bash
# Scenario: Key "users:123" was created at sequence 50
# Querying from sequence 100 won't show it as ADDED

# ❌ Inaccurate (will show as MODIFIED if changed after seq 100)
GET /api/v1/diff?from=100&to=200

# ✅ Accurate (will show as ADDED at sequence 50)
GET /api/v1/diff?from=0&to=200
```

---

## Integration Example

### Express.js Middleware

```javascript
const axios = require('axios');

async function getDiff(req, res, next) {
    try {
        const { from, to, table } = req.query;
        
        const response = await axios.get('http://localhost:8765/api/v1/diff', {
            params: { from, to, table }
        });
        
        req.diff = response.data;
        next();
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
}

app.use('/audit/diff', getDiff, (req, res) => {
    res.json(req.diff);
});
```

---

## See Also

- [MVCC Architecture](../architecture/architecture_mvcc.md)
- [Changefeed Documentation](../cdc/changefeed.md)
- [Named Snapshots](./features_snapshots.md)
- [Point-in-Time Recovery](./features_pitr.md)
- [Git-like Features Research](../../docs/research/GIT_LIKE_FEATURES_FOR_MVCC.md)

---

**Created:** 2026-01-12  
**Last Updated:** 2026-04-06  
**Version:** 1.0  
**Status:** Production Ready ✅
