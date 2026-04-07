# CHIMERA Adapter API Reference

**Version:** 1.0.0  
**License:** Apache-2.0 OR MIT  
**Last Updated:** 2026-04-06

---

## Table of Contents

1. [Core Types](#core-types)
2. [Error Handling](#error-handling)
3. [DatabaseAdapter Interface](#databaseadapter-interface)
4. [Transaction Interface](#transaction-interface)
5. [Configuration Structures](#configuration-structures)
6. [Search Parameters](#search-parameters)
7. [Adapter Implementations](#adapter-implementations)

---

## Core Types

### Value

Variant type representing database values.

```cpp
using Value = std::variant<
    std::monostate,           // NULL value
    bool,                     // Boolean
    int64_t,                  // 64-bit integer
    double,                   // Double precision float
    std::string,              // Text string
    std::vector<uint8_t>,     // Binary data (BLOB)
    std::vector<float>        // Vector for embeddings
>;
```

**Usage:**

```cpp
Value null_val = std::monostate{};
Value bool_val = true;
Value int_val = int64_t(42);
Value float_val = 3.14;
Value str_val = std::string("hello");
Value binary_val = std::vector<uint8_t>{0x01, 0x02};
Value vector_val = std::vector<float>{0.1f, 0.2f, 0.3f};
```

### Document

Map of field names to values.

```cpp
using Document = std::map<std::string, Value>;
```

**Usage:**

```cpp
Document doc;
doc["id"] = std::string("123");
doc["name"] = std::string("Alice");
doc["age"] = int64_t(30);
doc["active"] = true;
doc["embedding"] = std::vector<float>{0.1f, 0.2f};
```

### ResultSet

Collection of documents.

```cpp
using ResultSet = std::vector<Document>;
```

---

## Error Handling

### ErrorCode

Enumeration of error types.

```cpp
enum class ErrorCode {
    OK = 0,                     // Success
    CONNECTION_FAILED,          // Failed to connect
    AUTHENTICATION_FAILED,      // Auth credentials invalid
    OPERATION_FAILED,           // General operation failure
    NOT_FOUND,                  // Resource not found
    ALREADY_EXISTS,             // Resource already exists
    INVALID_ARGUMENT,           // Invalid parameter
    TRANSACTION_CONFLICT,       // Transaction conflict
    TIMEOUT,                    // Operation timed out
    UNSUPPORTED_OPERATION,      // Operation not supported
    FEATURE_NOT_AVAILABLE,      // Feature unavailable
    UNKNOWN_ERROR               // Unknown error
};
```

### Status

Status return type for operations.

```cpp
struct Status {
    ErrorCode code;
    std::string message;
    
    Status();
    Status(ErrorCode c, std::string msg = "");
    
    bool ok() const;
    explicit operator bool() const;
};
```

**Methods:**

- `ok()`: Returns `true` if `code == ErrorCode::OK`
- `operator bool()`: Implicit conversion to bool, same as `ok()`

**Usage:**

```cpp
auto status = adapter->connect(config);
if (!status.ok()) {
    std::cerr << "Error: " << status.message << std::endl;
    return status.code;
}
```

---

## DatabaseAdapter Interface

### Connection Management

#### connect

Connect to the database.

```cpp
virtual Status connect(const ConnectionConfig& config) = 0;
```

**Parameters:**
- `config`: Connection configuration

**Returns:** Status indicating success or failure

**Example:**

```cpp
ConnectionConfig config;
config.host = "localhost";
config.port = 5432;
config.database = "mydb";

auto status = adapter->connect(config);
```

#### disconnect

Disconnect from the database.

```cpp
virtual Status disconnect() = 0;
```

**Returns:** Status indicating success or failure

#### is_connected

Check connection status.

```cpp
virtual bool is_connected() const = 0;
```

**Returns:** `true` if connected, `false` otherwise

#### ping

Verify database connectivity.

```cpp
virtual Status ping() = 0;
```

**Returns:** Status indicating if database is reachable

---

### CRUD Operations

#### insert

Insert a single document.

```cpp
virtual Status insert(
    const std::string& collection,
    const Document& document,
    const QueryOptions& options = {}
) = 0;
```

**Parameters:**
- `collection`: Name of collection/table
- `document`: Document to insert
- `options`: Optional query parameters

**Returns:** Status indicating success or failure

**Example:**

```cpp
Document doc;
doc["id"] = std::string("1");
doc["name"] = std::string("Alice");

adapter->insert("users", doc);
```

#### insert_batch

Insert multiple documents.

```cpp
virtual Status insert_batch(
    const std::string& collection,
    const std::vector<Document>& documents,
    const QueryOptions& options = {}
) = 0;
```

**Parameters:**
- `collection`: Name of collection/table
- `documents`: Vector of documents to insert
- `options`: Optional query parameters

**Returns:** Status indicating success or failure

#### find

Find documents matching a filter.

```cpp
virtual Status find(
    const std::string& collection,
    const Document& filter,
    ResultSet& results,
    const QueryOptions& options = {}
) = 0;
```

**Parameters:**
- `collection`: Name of collection/table
- `filter`: Filter criteria (empty for all documents)
- `results`: Output parameter for results
- `options`: Optional query parameters (limit, offset, projection)

**Returns:** Status indicating success or failure

**Example:**

```cpp
Document filter;
filter["age"] = int64_t(30);

QueryOptions options;
options.limit = 10;
options.projection = {"id", "name"};

ResultSet results;
adapter->find("users", filter, results, options);
```

#### find_by_id

Find a single document by ID.

```cpp
virtual Status find_by_id(
    const std::string& collection,
    const std::string& id,
    Document& result,
    const QueryOptions& options = {}
) = 0;
```

**Parameters:**
- `collection`: Name of collection/table
- `id`: Document identifier
- `result`: Output parameter for the document
- `options`: Optional query parameters

**Returns:** Status (`ErrorCode::NOT_FOUND` if not found)

#### update

Update documents matching a filter.

```cpp
virtual Status update(
    const std::string& collection,
    const Document& filter,
    const Document& update,
    size_t& updated_count,
    const QueryOptions& options = {}
) = 0;
```

**Parameters:**
- `collection`: Name of collection/table
- `filter`: Selection criteria
- `update`: Update operations
- `updated_count`: Output parameter for number updated
- `options`: Optional query parameters

**Returns:** Status indicating success or failure

#### remove

Delete documents matching a filter.

```cpp
virtual Status remove(
    const std::string& collection,
    const Document& filter,
    size_t& deleted_count,
    const QueryOptions& options = {}
) = 0;
```

**Parameters:**
- `collection`: Name of collection/table
- `filter`: Selection criteria
- `deleted_count`: Output parameter for number deleted
- `options`: Optional query parameters

**Returns:** Status indicating success or failure

#### count

Count documents matching a filter.

```cpp
virtual Status count(
    const std::string& collection,
    const Document& filter,
    size_t& count,
    const QueryOptions& options = {}
) = 0;
```

**Parameters:**
- `collection`: Name of collection/table
- `filter`: Selection criteria (empty for total count)
- `count`: Output parameter for count
- `options`: Optional query parameters

**Returns:** Status indicating success or failure

---

### Vector Operations

#### supports_vector_search

Check if vector search is supported.

```cpp
virtual bool supports_vector_search() const = 0;
```

**Returns:** `true` if vector operations available

#### vector_search

Perform k-nearest neighbor search.

```cpp
virtual Status vector_search(
    const std::string& collection,
    const std::string& vector_field,
    const VectorSearchParams& params,
    ResultSet& results
) = 0;
```

**Parameters:**
- `collection`: Name of collection/table
- `vector_field`: Name of field containing vectors
- `params`: Search parameters (vector, k, metric)
- `results`: Output parameter for results

**Returns:** Status indicating success or failure

**Example:**

```cpp
VectorSearchParams params;
params.query_vector = {0.1f, 0.2f, 0.3f};
params.k = 10;
params.metric = "cosine";

ResultSet results;
adapter->vector_search("docs", "embedding", params, results);
```

#### create_vector_index

Create an index for vector search.

```cpp
virtual Status create_vector_index(
    const std::string& collection,
    const std::string& field,
    size_t dimensions,
    const std::string& index_type = "hnsw",
    const std::map<std::string, std::string>& parameters = {}
) = 0;
```

**Parameters:**
- `collection`: Name of collection/table
- `field`: Vector field name
- `dimensions`: Vector dimensionality
- `index_type`: Index algorithm ("hnsw", "ivf", "flat")
- `parameters`: Algorithm-specific parameters

**Returns:** Status indicating success or failure

---

### Transaction Management

#### supports_transactions

Check if transactions are supported.

```cpp
virtual bool supports_transactions() const = 0;
```

**Returns:** `true` if transactions available

#### begin_transaction

Begin a new transaction.

```cpp
virtual Status begin_transaction(
    std::unique_ptr<Transaction>& transaction,
    IsolationLevel level = IsolationLevel::READ_COMMITTED
) = 0;
```

**Parameters:**
- `transaction`: Output parameter for transaction handle
- `level`: Isolation level

**Returns:** Status indicating success or failure

**Example:**

```cpp
std::unique_ptr<Transaction> txn;
adapter->begin_transaction(txn, IsolationLevel::SERIALIZABLE);

QueryOptions options;
options.transaction = txn.get();
adapter->insert("users", doc, options);

txn->commit();
```

---

### Schema Operations

#### create_collection

Create a collection/table.

```cpp
virtual Status create_collection(
    const std::string& collection,
    const std::map<std::string, std::string>& schema = {}
) = 0;
```

**Parameters:**
- `collection`: Name of collection to create
- `schema`: Optional schema definition (database-specific)

**Returns:** Status indicating success or failure

#### drop_collection

Drop a collection/table.

```cpp
virtual Status drop_collection(const std::string& collection) = 0;
```

**Parameters:**
- `collection`: Name of collection to drop

**Returns:** Status indicating success or failure

#### list_collections

List all collections/tables.

```cpp
virtual Status list_collections(std::vector<std::string>& collections) = 0;
```

**Parameters:**
- `collections`: Output parameter for collection names

**Returns:** Status indicating success or failure

#### collection_exists

Check if a collection exists.

```cpp
virtual Status collection_exists(
    const std::string& collection,
    bool& exists
) = 0;
```

**Parameters:**
- `collection`: Collection name to check
- `exists`: Output parameter

**Returns:** Status indicating success or failure

---

### Query Execution

#### execute_query

Execute a native query.

```cpp
virtual Status execute_query(
    const std::string& query,
    ResultSet& results,
    const QueryOptions& options = {}
) = 0;
```

**Parameters:**
- `query`: Native query string (SQL, AQL, Cypher, etc.)
- `results`: Output parameter for results
- `options`: Optional query parameters

**Returns:** Status indicating success or failure

#### execute_query_params

Execute a parameterized query.

```cpp
virtual Status execute_query_params(
    const std::string& query,
    const std::map<std::string, Value>& params,
    ResultSet& results,
    const QueryOptions& options = {}
) = 0;
```

**Parameters:**
- `query`: Query with parameter placeholders
- `params`: Query parameters
- `results`: Output parameter for results
- `options`: Optional query parameters

**Returns:** Status indicating success or failure

---

### Metadata

#### get_adapter_info

Get adapter name and version.

```cpp
virtual std::string get_adapter_info() const = 0;
```

**Returns:** Adapter information string

**Example:** `"PostgreSQLAdapter v1.0.0 - CHIMERA Suite"`

#### get_database_version

Get database system version.

```cpp
virtual std::string get_database_version() const = 0;
```

**Returns:** Database version string

**Example:** `"PostgreSQL 15.2"`

#### get_capabilities

Get supported features.

```cpp
virtual std::vector<std::string> get_capabilities() const = 0;
```

**Returns:** List of capability strings

**Example:** `{"crud", "transactions", "vector_search", "queries"}`

---

## Transaction Interface

### commit

Commit the transaction.

```cpp
virtual Status commit() = 0;
```

**Returns:** Status indicating success or failure

### rollback

Rollback the transaction.

```cpp
virtual Status rollback() = 0;
```

**Returns:** Status indicating success or failure

### is_active

Check if transaction is active.

```cpp
virtual bool is_active() const = 0;
```

**Returns:** `true` if transaction is still active

---

## Configuration Structures

### ConnectionConfig

Connection configuration.

```cpp
struct ConnectionConfig {
    std::string host;
    uint16_t port;
    std::string database;
    std::string username;
    std::string password;
    std::map<std::string, std::string> parameters;
    uint32_t timeout_ms = 30000;
    uint32_t pool_size = 10;
    bool use_tls = false;
};
```

**Fields:**
- `host`: Hostname or IP address
- `port`: Port number
- `database`: Database/keyspace name
- `username`: Authentication username
- `password`: Authentication password
- `parameters`: Additional database-specific parameters
- `timeout_ms`: Connection timeout in milliseconds
- `pool_size`: Connection pool size
- `use_tls`: Enable TLS/SSL encryption

### QueryOptions

Query execution options.

```cpp
struct QueryOptions {
    std::optional<size_t> limit;
    std::optional<size_t> offset;
    std::vector<std::string> projection;
    std::optional<uint32_t> timeout_ms;
    Transaction* transaction = nullptr;
};
```

**Fields:**
- `limit`: Maximum number of results
- `offset`: Number of results to skip
- `projection`: Fields to include (empty = all)
- `timeout_ms`: Query timeout in milliseconds
- `transaction`: Transaction context (optional)

---

## Search Parameters

### VectorSearchParams

Vector similarity search parameters.

```cpp
struct VectorSearchParams {
    std::vector<float> query_vector;
    size_t k;
    std::string metric = "cosine";
    std::map<std::string, Value> filters;
    std::optional<uint32_t> timeout_ms;
};
```

**Fields:**
- `query_vector`: Query vector for similarity search
- `k`: Number of nearest neighbors to return
- `metric`: Distance metric (`"cosine"`, `"euclidean"`, `"dot_product"`)
- `filters`: Optional metadata filters
- `timeout_ms`: Search timeout in milliseconds

### IsolationLevel

Transaction isolation levels.

```cpp
enum class IsolationLevel {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE,
    SNAPSHOT_ISOLATION
};
```

---

## Adapter Implementations

### Factory Functions

```cpp
namespace chimera::adapters {
    // ThemisDB
    std::unique_ptr<DatabaseAdapter> create_themisdb_adapter();
    
    // PostgreSQL
    std::unique_ptr<DatabaseAdapter> create_postgresql_adapter();
    
    // Weaviate
    std::unique_ptr<DatabaseAdapter> create_weaviate_adapter();
}
```

### Usage

```cpp
#include "chimera/postgresql_adapter.hpp"

auto adapter = chimera::adapters::create_postgresql_adapter();
```

---

## Thread Safety

**General Rules:**
- Adapter instances are **not thread-safe** by default
- Create separate adapter instances per thread, OR
- Use external synchronization (mutex)
- Connection pooling is thread-safe within a single adapter

**Example (multi-threaded):**

```cpp
// Option 1: Separate adapters per thread
std::thread t1([&]() {
    auto adapter = create_postgresql_adapter();
    // Use adapter...
});

// Option 2: Shared adapter with mutex
std::mutex adapter_mutex;
std::lock_guard<std::mutex> lock(adapter_mutex);
adapter->insert(/* ... */);
```

---

## Best Practices

1. **Always check Status return values**
2. **Use batch operations for multiple inserts**
3. **Set appropriate timeouts**
4. **Check feature support before using optional features**
5. **Clean up resources (disconnect)**
6. **Use transactions for atomic operations**
7. **Use projections to minimize data transfer**
8. **Handle NOT_FOUND appropriately**
9. **Use connection pooling for concurrent requests**
10. **Follow vendor-neutral naming conventions**

---

## Version History

- **v1.0.0** (2026-01-20): Initial release
  - Core interface definition
  - ThemisDB, PostgreSQL, Weaviate adapters
  - Comprehensive documentation

---

**For implementation guide, see:** [ADAPTER_GUIDE.md](ADAPTER_GUIDE.md)  
**For quick reference, see:** [QUICK_REFERENCE.md](../adapters/chimera/QUICK_REFERENCE.md)  
**For examples, see:** [example_usage.cpp](../adapters/chimera/example_usage.cpp)
