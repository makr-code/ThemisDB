# CHIMERA Adapter Quick Reference

## Include Headers

```cpp
#include "chimera/database_adapter.hpp"         // Base interface
#include "chimera/themisdb_adapter.hpp"         // ThemisDB
#include "chimera/postgresql_adapter.hpp"       // PostgreSQL
#include "chimera/weaviate_adapter.hpp"         // Weaviate
#include "chimera/qdrant_adapter.hpp"           // Qdrant
```

## Create Adapter

```cpp
using namespace chimera;
using namespace chimera::adapters;

// ThemisDB
auto adapter = create_themisdb_adapter();

// PostgreSQL
auto adapter = create_postgresql_adapter();

// Weaviate
auto adapter = create_weaviate_adapter();

// Qdrant
auto adapter = create_qdrant_adapter();
```

## Connection

```cpp
ConnectionConfig config;
config.host = "localhost";
config.port = 5432;
config.database = "mydb";
config.username = "user";
config.password = "pass";

adapter->connect(config);
adapter->ping();
adapter->disconnect();
```

## CRUD Operations

### Insert

```cpp
Document doc;
doc["id"] = std::string("1");
doc["name"] = std::string("Alice");
doc["age"] = int64_t(30);

adapter->insert("users", doc);
```

### Batch Insert

```cpp
std::vector<Document> docs = { /* ... */ };
adapter->insert_batch("users", docs);
```

### Find

```cpp
Document filter;
filter["age"] = int64_t(30);

ResultSet results;
adapter->find("users", filter, results);
```

### Find by ID

```cpp
Document result;
adapter->find_by_id("users", "1", result);
```

### Update

```cpp
Document filter;
filter["id"] = std::string("1");

Document update;
update["age"] = int64_t(31);

size_t count;
adapter->update("users", filter, update, count);
```

### Delete

```cpp
Document filter;
filter["id"] = std::string("1");

size_t count;
adapter->remove("users", filter, count);
```

### Count

```cpp
size_t count;
adapter->count("users", {}, count);
```

## Query Options

```cpp
QueryOptions options;
options.limit = 10;
options.offset = 5;
options.projection = {"id", "name"};
options.timeout_ms = 5000;

adapter->find("users", {}, results, options);
```

## Vector Search

```cpp
if (adapter->supports_vector_search()) {
    VectorSearchParams params;
    params.query_vector = {0.1f, 0.2f, 0.3f};
    params.k = 10;
    params.metric = "cosine";  // or "euclidean", "dot_product"
    
    ResultSet results;
    adapter->vector_search("docs", "embedding", params, results);
}
```

## Transactions

```cpp
if (adapter->supports_transactions()) {
    std::unique_ptr<Transaction> txn;
    adapter->begin_transaction(txn, IsolationLevel::READ_COMMITTED);
    
    QueryOptions options;
    options.transaction = txn.get();
    
    adapter->insert("users", doc, options);
    
    txn->commit();  // or txn->rollback();
}
```

## Native Queries

```cpp
// Simple query
std::string query = "SELECT * FROM users WHERE age > 25";
ResultSet results;
adapter->execute_query(query, results);

// Parameterized query
std::string query = "SELECT * FROM users WHERE age > $1";
std::map<std::string, Value> params;
params["1"] = int64_t(25);
adapter->execute_query_params(query, params, results);
```

## Schema Operations

```cpp
// Create collection
adapter->create_collection("users");

// Drop collection
adapter->drop_collection("users");

// List collections
std::vector<std::string> collections;
adapter->list_collections(collections);

// Check if exists
bool exists;
adapter->collection_exists("users", exists);
```

## Error Handling

```cpp
auto status = adapter->connect(config);
if (!status.ok()) {
    std::cerr << "Error: " << status.message << std::endl;
    
    switch (status.code) {
        case ErrorCode::CONNECTION_FAILED:
            // Handle connection error
            break;
        case ErrorCode::AUTHENTICATION_FAILED:
            // Handle auth error
            break;
        case ErrorCode::TIMEOUT:
            // Handle timeout
            break;
        // ...
    }
}
```

## Value Types

```cpp
Document doc;

// NULL
doc["field1"] = std::monostate{};

// Boolean
doc["field2"] = true;

// Integer
doc["field3"] = int64_t(42);

// Float
doc["field4"] = 3.14;

// String
doc["field5"] = std::string("text");

// Binary
doc["field6"] = std::vector<uint8_t>{0x01, 0x02, 0x03};

// Vector (embeddings)
doc["field7"] = std::vector<float>{0.1f, 0.2f, 0.3f};
```

## Accessing Values

```cpp
// Check if field exists
if (doc.find("name") != doc.end()) {
    // Access value with std::get
    auto name = std::get<std::string>(doc["name"]);
    auto age = std::get<int64_t>(doc["age"]);
}

// Visit pattern for unknown types
std::visit([](auto&& value) {
    using T = std::decay_t<decltype(value)>;
    if constexpr (std::is_same_v<T, std::string>) {
        std::cout << "String: " << value << std::endl;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        std::cout << "Integer: " << value << std::endl;
    }
    // ...
}, doc["field"]);
```

## Metadata

```cpp
// Get adapter info
std::string info = adapter->get_adapter_info();
// e.g., "PostgreSQLAdapter v1.0.0"

// Get database version
std::string version = adapter->get_database_version();
// e.g., "PostgreSQL 15.2"

// Get capabilities
auto caps = adapter->get_capabilities();
// e.g., {"crud", "transactions", "vector_search"}

// Check specific capability
bool has_vectors = adapter->supports_vector_search();
bool has_txn = adapter->supports_transactions();
```

## Isolation Levels

```cpp
IsolationLevel::READ_UNCOMMITTED
IsolationLevel::READ_COMMITTED
IsolationLevel::REPEATABLE_READ
IsolationLevel::SERIALIZABLE
IsolationLevel::SNAPSHOT_ISOLATION
```

## Error Codes

```cpp
ErrorCode::OK
ErrorCode::CONNECTION_FAILED
ErrorCode::AUTHENTICATION_FAILED
ErrorCode::OPERATION_FAILED
ErrorCode::NOT_FOUND
ErrorCode::ALREADY_EXISTS
ErrorCode::INVALID_ARGUMENT
ErrorCode::TRANSACTION_CONFLICT
ErrorCode::TIMEOUT
ErrorCode::UNSUPPORTED_OPERATION
ErrorCode::FEATURE_NOT_AVAILABLE
ErrorCode::UNKNOWN_ERROR
```

## Best Practices

1. **Always check Status**: `if (!status.ok()) { /* handle error */ }`
2. **Use connection pooling**: Set `config.pool_size` appropriately
3. **Set timeouts**: Use `config.timeout_ms` and `options.timeout_ms`
4. **Batch operations**: Use `insert_batch()` for multiple inserts
5. **Clean up**: Call `disconnect()` when done
6. **Check capabilities**: Use `supports_*()` before optional features
7. **Handle NOT_FOUND**: Check for `ErrorCode::NOT_FOUND` when expected
8. **Use transactions**: When atomicity is required
9. **Project fields**: Use `options.projection` to limit data transfer
10. **Type safety**: Use `std::get<T>()` with proper type

## Configuration Files

```toml
# adapter_config.toml
[database]
host = "localhost"
port = 5432
database = "mydb"
username = "user"
password = "${DB_PASSWORD}"
pool_size = 20
timeout_ms = 30000
use_tls = true
```

## Building

```cmake
# CMakeLists.txt
find_package(chimera_adapters REQUIRED)

target_link_libraries(myapp
    chimera_postgresql_adapter
)
```

```bash
cmake -B build -DBUILD_CHIMERA_ADAPTERS=ON
cmake --build build
```

## Testing

```cpp
#include "chimera/adapter_test_utils.hpp"

CHIMERA_ADAPTER_TEST_SUITE(MyAdapter, MyAdapterTest)

void MyAdapterTest::SetUp() {
    adapter = create_my_adapter();
    auto status = adapter->connect(get_test_config());
    ASSERT_TRUE(status.ok());
}
```

---

**Full documentation:** `docs/chimera/ADAPTER_GUIDE.md`  
**Examples:** `adapters/chimera/example_usage.cpp`  
**API Reference:** `include/chimera/database_adapter.hpp`
