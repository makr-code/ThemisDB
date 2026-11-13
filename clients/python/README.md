---
Author: "ThemisDB Contributors"
Created: "2026-11-13"
Last Updated: "2026-11-13"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Python SDK

Official Python client for ThemisDB - A high-performance multi-model database.

## Features

- ✅ **Type Hints** - Full type annotations (PEP 484)
- ✅ **Transaction Support** - BEGIN/COMMIT/ROLLBACK with isolation levels
- ✅ **LLM Integration** - Native support for LLM interactions (v1.4.0+) 🆕
- ✅ **Context Manager** - Pythonic `with` statement support
- ✅ **Multi-Model** - Relational, Graph, Vector operations
- ✅ **Query Support** - AQL (Advanced Query Language)
- ✅ **Topology-Aware** - Automatic shard routing
- ✅ **Batch Operations** - Efficient bulk operations
- ✅ **Vector Search** - Similarity search
- ✅ **Retry Logic** - Automatic retries

## Installation

```bash
pip install themisdb-client
```

Or for development:

```bash
pip install -e .[dev]
```

## Quick Start

```python
from themis import ThemisClient

client = ThemisClient(endpoints=["http://localhost:8080"])

# Basic CRUD
client.put("relational", "users", "user1", {"name": "Alice"})
user = client.get("relational", "users", "user1")
print(user)

# Transactions (NEW!)
tx = client.begin_transaction()
try:
    tx.put("relational", "accounts", "acc1", {"balance": 1000})
    tx.put("relational", "accounts", "acc2", {"balance": 500})
    tx.commit()
except Exception:
    tx.rollback()
    raise
```

## Transaction Support (NEW!)

ThemisDB now supports ACID transactions with BEGIN/COMMIT/ROLLBACK semantics.

### Basic Transaction Usage

```python
from themis import ThemisClient

client = ThemisClient(endpoints=["http://localhost:8080"])

# Begin a transaction
tx = client.begin_transaction()

try:
    # Perform operations within the transaction
    tx.put("relational", "accounts", "acc1", {"balance": 1000})
    tx.put("relational", "accounts", "acc2", {"balance": 500})
    
    # Read within transaction
    acc1 = tx.get("relational", "accounts", "acc1")
    print(acc1)  # {"balance": 1000}
    
    # Commit the transaction
    tx.commit()
except Exception as error:
    # Rollback on error
    tx.rollback()
    raise
```

### Context Manager (Pythonic Way)

The recommended way to use transactions in Python is with the `with` statement:

```python
# Automatically commits on success, rolls back on exception
with client.begin_transaction() as tx:
    tx.put("relational", "accounts", "acc1", {"balance": 1000})
    tx.put("relational", "accounts", "acc2", {"balance": 500})
    
    acc1 = tx.get("relational", "accounts", "acc1")
    print(acc1)  # {"balance": 1000}
    
# Transaction is automatically committed here
```

If an exception occurs, the transaction is automatically rolled back:

```python
try:
    with client.begin_transaction() as tx:
        tx.put("relational", "users", "user1", {"name": "Alice"})
        raise ValueError("Something went wrong")
except ValueError:
    pass
# Transaction was automatically rolled back
```

### Isolation Levels

ThemisDB supports three isolation levels:

- `READ_COMMITTED` (default) – Prevents dirty reads. Non-repeatable reads and phantom
  reads are possible.
- `SNAPSHOT` – Provides a consistent snapshot of the database as of transaction start.

  > ⚠️ **Write-skew and phantom-read anomalies are possible at SNAPSHOT isolation.**
  > Two concurrent SNAPSHOT transactions that each read the same data and write
  > disjoint keys can both commit even when their combined effect violates an
  > application invariant (e.g. double-booking, over-withdrawal). Use `SERIALIZABLE`
  > when strict correctness is required.

- `SERIALIZABLE` – Full serializability via SSI / predicate locking. Prevents write
  skew and phantom reads. May abort more transactions and has higher latency than
  SNAPSHOT.

```python
# Use SNAPSHOT isolation for repeatable reads
# WARNING: write skew and phantom reads are possible at this level
tx = client.begin_transaction(isolation_level="SNAPSHOT")

# Use SERIALIZABLE to prevent write skew and phantom reads
tx = client.begin_transaction(isolation_level="SERIALIZABLE")

try:
    user1 = tx.get("relational", "users", "user1")
    user2 = tx.get("relational", "users", "user2")
    
    # These reads are from the same snapshot
    # even if other transactions modify the data
    
    tx.commit()
except Exception:
    tx.rollback()
```

### Query Within Transactions

```python
with client.begin_transaction() as tx:
    # Execute AQL query within transaction
    result = tx.query("FOR user IN users FILTER user.active == true RETURN user")
    
    # Update based on query results
    for user in result.items:
        user["last_seen"] = "2025-11-20T12:00:00Z"
        tx.put("relational", "users", user["id"], user)
    
# Automatically committed
```

### Complex Example: Money Transfer

```python
def transfer_money(client, from_account: str, to_account: str, amount: float):
    """Transfer money between accounts using a transaction."""
    with client.begin_transaction(isolation_level="SNAPSHOT") as tx:
        # Read both accounts
        from_acc = tx.get("relational", "accounts", from_account)
        to_acc = tx.get("relational", "accounts", to_account)
        
        if not from_acc or not to_acc:
            raise ValueError("Account not found")
        
        if from_acc["balance"] < amount:
            raise ValueError("Insufficient funds")
        
        # Update balances
        from_acc["balance"] -= amount
        to_acc["balance"] += amount
        
        tx.put("relational", "accounts", from_account, from_acc)
        tx.put("relational", "accounts", to_account, to_acc)
        
    # Transaction automatically committed

# Usage
transfer_money(client, "alice", "bob", 100.0)
```

## LLM Integration (v1.4.0+) 🆕

ThemisDB v1.8.0-rc1 introduces native LLM integration with support for various models and features like prefix caching, response caching, multi-GPU, and more.

### Basic LLM Interaction

```python
from themis import ThemisClient

client = ThemisClient(endpoints=["http://localhost:8080"])

# Create an LLM interaction
result = client.llm_interaction(
    model="gpt-4o",
    messages=[
        {"role": "user", "content": "Explain MVCC in databases"}
    ]
)

print(f"Interaction ID: {result.id}")
print(f"Success: {result.success}")
```

### LLM with Reasoning Steps

```python
# Create interaction with chain-of-thought reasoning
result = client.llm_interaction(
    model="llama-3.1",
    messages=[
        {"role": "system", "content": "You are a database expert"},
        {"role": "user", "content": "How does ThemisDB handle transactions?"}
    ],
    reasoning_steps=[
        {
            "type": "chain_of_thought",
            "content": [
                "MVCC allows parallel reading/writing",
                "Each transaction gets a snapshot",
                "Commit checks for conflicts"
            ]
        }
    ],
    metadata={"use_case": "documentation", "version": "1.4.0"}
)
```

### Retrieve LLM Interactions

```python
# Get a specific interaction
interaction = client.get_llm_interaction("interaction_id_123")

if interaction:
    print(f"Model: {interaction.model}")
    print(f"Created: {interaction.created_at}")
    for msg in interaction.messages:
        print(f"{msg.role}: {msg.content}")
    
    if interaction.reasoning_steps:
        for step in interaction.reasoning_steps:
            print(f"Reasoning ({step.type}): {step.content}")
```

### List LLM Interactions

```python
# List all interactions
interactions = client.list_llm_interactions(limit=50, offset=0)

for interaction in interactions:
    print(f"{interaction.id}: {interaction.model} - {interaction.created_at}")

# Filter by model
gpt4_interactions = client.list_llm_interactions(model="gpt-4o", limit=10)
```

### LLM with Vision Support

```python
# Vision model interaction (requires v1.4.0+ with vision support)
result = client.llm_interaction(
    model="gpt-4-vision",
    messages=[
        {
            "role": "user",
            "content": "What's in this image?",
            "image_url": "https://example.com/image.jpg"  # Or base64 encoded
        }
    ]
)
```

## API Reference

### ThemisClient

#### Constructor

```python
ThemisClient(
    endpoints: list[str],
    *,
    namespace: str = "default",
    timeout: float = 30.0,
    max_retries: int = 3,
    metadata_endpoint: str | None = None,
    metadata_path: str = "/_admin/cluster/topology",
    max_workers: int | None = None,
    transport: httpx.BaseTransport | None = None
)
```

#### Methods

- `get(model, collection, uuid)` - Retrieve an entity
- `put(model, collection, uuid, data)` - Create/update an entity
- `delete(model, collection, uuid)` - Delete an entity
- `batch_get(model, collection, uuids)` - Batch retrieve
- `batch_put(model, collection, items)` - Batch create/update
- `batch_delete(model, collection, uuids)` - Batch delete
- `query(aql, *, params=None)` - Execute AQL query
- `vector_search(embedding, top_k=10, metadata_filter=None, *, use_cursor=False, cursor=None)` - Synchronous vector similarity search
- `graph_traverse(start_node, max_depth=3)` - Graph traversal
- `health(endpoint=None)` - Health check
- `begin_transaction(*, isolation_level="READ_COMMITTED")` - Start transaction
- `llm_interaction(model, messages, *, reasoning_steps=None, metadata=None)` - **NEW:** Create LLM interaction
- `get_llm_interaction(interaction_id)` - **NEW:** Retrieve LLM interaction
- `list_llm_interactions(*, model=None, limit=100, offset=0)` - **NEW:** List LLM interactions

#### Synchronous vector search contract (#8089)

`ThemisClient.vector_search()` sends `POST /vector/search` with `vector` and
`k=top_k`. `metadata_filter=None` omits the HTTP `filter` field; `{}` sends an
empty object. Both are no-ops. The server does **not** implement caller metadata
filtering here: any nonempty `filter` object or nonobject value (including JSON
`null`, arrays, strings, numbers, and booleans) is rejected with HTTP 400.
Caller-supplied metadata filters are **not authorization policy**; do not use
them to enforce tenant isolation or access control.

For a multi-endpoint client:

- Hits from successful endpoints are merged and limited to global `top_k`,
  even when only one endpoint succeeds.
- Prefer descending `score` when every hit has a non-null score (`0` is valid).
  Otherwise use ascending `distance` when every hit has a non-null distance.
  Scores/distances are expected to be numeric. Ties retain endpoint order, then
  response order. Mixed hits without a common ranking field raise `ValueError`
  with `Vector search hits lack a common non-null score or distance`, rather than
  comparing scores and distances as interchangeable.
- HTTP/request failures are reported through `partial=True` and an `errors`
  list of `endpoint`, `status`, and `message` records. Endpoint origins omit
  credentials, paths, and query strings; request failures have `status=None`.
  Messages are `HTTP <status>` or `Request failure (<exception class>)`; server
  response bodies are intentionally not exposed.
  Only when at least two endpoints succeed does the SDK return merged `results`
  and original successful payloads in `partials`. With one successful response,
  even if other shards fail, its response keys and `results` or `items` format
  are preserved while hits are ranked/capped and `count`/`batch_size` updated
  when present.
  Failures add `partial=True` and `errors`; the SDK does not add `partials` for
  one success. A partial response is not a complete search
  result. If all endpoints fail, the client raises `httpx.HTTPError` with the
  structured list in its `errors` attribute, not a successful empty result.
  An HTTP 200 with no hits is still a valid empty success.

A successful single-endpoint call retains the server response format and cursor
fields while ranking/capping hits and updating `count`/`batch_size` when present. Pass
`use_cursor=True` and the returned `next_cursor` as `cursor` to continue; no global
multi-endpoint cursor is promised. If a shard exceeds the requested k, client
truncation may skip hits when following its preserved cursor.

**Compatibility:** previously ignored nonempty filters now error; formerly
silent partial success is explicit, and all-shard failure is no longer an empty
success. Correct score/distance ranking can change result order and global
top-k membership. These SDK changes apply only to the synchronous REST
`ThemisClient`; `AsyncThemisClient` is out of scope and is not claimed to have
changed.

Sources: [`ThemisClient.vector_search()`](themis/__init__.py) and
[`VectorApiHandler::handleSearch()`](../../src/server/vector_api_handler.cpp).

### Transaction

#### Properties

- `transaction_id: str` - Unique transaction identifier
- `is_active: bool` - Whether the transaction is active

#### Methods

- `get(model, collection, uuid)` - Retrieve within transaction
- `put(model, collection, uuid, data)` - Update within transaction
- `delete(model, collection, uuid)` - Delete within transaction
- `query(aql, *, params=None)` - Query within transaction
- `commit()` - Commit the transaction
- `rollback()` - Rollback the transaction

#### Context Manager

```python
with client.begin_transaction() as tx:
    # Operations here
    pass
# Automatically commits on success, rolls back on exception
```

## Development

```bash
# Install with dev dependencies
pip install -e .[dev]

# Run tests
pytest tests/

# Run tests with coverage
pytest --cov=themis tests/
```

## License

Apache-2.0

