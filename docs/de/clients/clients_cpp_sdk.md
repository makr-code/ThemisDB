# ThemisDB C++ Client SDK

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Clients  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Installation](#installation)
- [Basic Usage](#basic-usage)
- [Authentication](#authentication)
- [CRUD Operations](#crud-operations)
- [Query Execution](#query-execution)
- [Performance Optimization](#performance-optimization)
- [Error Handling](#error-handling)
- [Advanced Features](#advanced-features)

---

## Installation

### vcpkg

```bash
vcpkg install themisdb-client
```

### CMake

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(MyApp)

find_package(themisdb-client CONFIG REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE themisdb::client)
```

### Manual Build

```bash
git clone https://github.com/themisdb/cpp-client.git
cd cpp-client
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

---

## Basic Usage

### Connection

```cpp
#include <themisdb/client.h>

int main() {
    // Simple connection
    auto client = themisdb::Client::create("http://localhost:8765");
    
    // With options
    themisdb::ClientOptions options;
    options.connection_timeout = std::chrono::seconds(10);
    options.request_timeout = std::chrono::seconds(30);
    options.max_retries = 3;
    
    auto client = themisdb::Client::create("http://localhost:8765", options);
    
    return 0;
}
```

### Connection Pooling

```cpp
#include <themisdb/client.h>

int main() {
    themisdb::ConnectionPoolOptions pool_options;
    pool_options.min_connections = 5;
    pool_options.max_connections = 50;
    pool_options.connection_timeout = std::chrono::seconds(30);
    
    auto pool = themisdb::ConnectionPool::create(
        "http://localhost:8765",
        pool_options
    );
    
    // Get connection from pool
    auto client = pool->acquire();
    
    // Use client...
    auto result = client->query("FOR doc IN users RETURN doc");
    
    // Connection automatically returned to pool when 'client' goes out of scope
    
    return 0;
}
```

---

## Authentication

### Basic Authentication

```cpp
#include <themisdb/client.h>

int main() {
    themisdb::ClientOptions options;
    options.username = "admin";
    options.password = "secret";
    
    auto client = themisdb::Client::create("http://localhost:8765", options);
    
    // All requests now use Basic Auth
    auto result = client->query("FOR doc IN users RETURN doc");
    
    return 0;
}
```

### Token-based Authentication

```cpp
#include <themisdb/client.h>

int main() {
    // 1. Login to get token
    auto client = themisdb::Client::create("http://localhost:8765");
    
    auto login_result = client->login("admin", "secret");
    std::string token = login_result.token;
    std::string refresh_token = login_result.refresh_token;
    
    // 2. Use token for subsequent requests
    themisdb::ClientOptions options;
    options.auth_token = token;
    
    auto authenticated_client = themisdb::Client::create(
        "http://localhost:8765",
        options
    );
    
    // 3. Refresh token when expired
    auto new_token = authenticated_client->refresh_token(refresh_token);
    
    return 0;
}
```

### SSL/TLS

```cpp
#include <themisdb/client.h>

int main() {
    themisdb::ClientOptions options;
    options.use_ssl = true;
    options.ssl_verify = true;
    options.ssl_cert_path = "/path/to/cert.pem";
    options.ssl_key_path = "/path/to/key.pem";
    options.ssl_ca_path = "/path/to/ca.pem";
    
    auto client = themisdb::Client::create("https://localhost:8765", options);
    
    return 0;
}
```

### mTLS (Mutual TLS)

```cpp
#include <themisdb/client.h>

int main() {
    themisdb::ClientOptions options;
    options.use_ssl = true;
    options.ssl_verify = true;
    
    // Client certificate for mutual authentication
    options.ssl_client_cert_path = "/path/to/client-cert.pem";
    options.ssl_client_key_path = "/path/to/client-key.pem";
    
    // CA certificate to verify server
    options.ssl_ca_path = "/path/to/ca-bundle.pem";
    
    auto client = themisdb::Client::create("https://localhost:8765", options);
    
    return 0;
}
```

---

## CRUD Operations

### Create (Insert)

```cpp
#include <themisdb/client.h>
#include <nlohmann/json.hpp>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    // Single document
    nlohmann::json doc = {
        {"name", "Alice"},
        {"age", 30},
        {"email", "alice@example.com"}
    };
    
    auto result = client->insert("users", "user-123", doc);
    std::cout << "Inserted: " << result.key << std::endl;
    
    // Batch insert
    std::vector<themisdb::Document> docs = {
        {"user-124", {{"name", "Bob"}, {"age", 25}}},
        {"user-125", {{"name", "Charlie"}, {"age", 35}}}
    };
    
    auto batch_result = client->batch_insert("users", docs);
    std::cout << "Inserted " << batch_result.success_count << " documents" << std::endl;
    
    return 0;
}
```

### Read (Get)

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    // Get single document
    auto doc = client->get("users", "user-123");
    if (doc.has_value()) {
        std::cout << "Name: " << doc->at("name") << std::endl;
    }
    
    // Batch get
    std::vector<std::string> keys = {"user-123", "user-124", "user-125"};
    auto batch_result = client->batch_get("users", keys);
    
    for (const auto& doc : batch_result.found) {
        std::cout << "Found: " << doc.key << std::endl;
    }
    
    for (const auto& key : batch_result.missing) {
        std::cout << "Missing: " << key << std::endl;
    }
    
    return 0;
}
```

### Update

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    // Partial update
    nlohmann::json updates = {
        {"age", 31},
        {"last_login", "2026-01-24T14:30:00Z"}
    };
    
    auto result = client->update("users", "user-123", updates);
    
    // Full replace
    nlohmann::json new_doc = {
        {"name", "Alice Smith"},
        {"age", 31},
        {"email", "alice.smith@example.com"}
    };
    
    auto replace_result = client->replace("users", "user-123", new_doc);
    
    return 0;
}
```

### Delete

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    // Delete single document
    client->remove("users", "user-123");
    
    // Batch delete
    std::vector<std::string> keys = {"user-124", "user-125"};
    auto batch_result = client->batch_remove("users", keys);
    
    std::cout << "Deleted " << batch_result.deleted_count << " documents" << std::endl;
    
    return 0;
}
```

---

## Query Execution

### Basic Query

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    auto result = client->query(R"(
        FOR doc IN users
          FILTER doc.age > 25
          SORT doc.name ASC
          LIMIT 10
          RETURN doc
    )");
    
    for (const auto& doc : result.entities) {
        std::cout << "Name: " << doc["name"] << std::endl;
    }
    
    return 0;
}
```

### Parameterized Query

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    nlohmann::json bind_vars = {
        {"min_age", 25},
        {"limit", 10}
    };
    
    auto result = client->query(R"(
        FOR doc IN users
          FILTER doc.age > @min_age
          LIMIT @limit
          RETURN doc
    )", bind_vars);
    
    std::cout << "Found " << result.entities.size() << " users" << std::endl;
    
    return 0;
}
```

### Cursor-based Pagination

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    themisdb::QueryOptions options;
    options.batch_size = 100;
    options.use_cursor = true;
    
    auto cursor = client->query_cursor(R"(
        FOR doc IN large_collection
          RETURN doc
    )", options);
    
    while (cursor->has_more()) {
        auto batch = cursor->next();
        
        for (const auto& doc : batch) {
            // Process document
            process(doc);
        }
    }
    
    return 0;
}
```

---

## Performance Optimization

### Connection Reuse

```cpp
#include <themisdb/client.h>

// ✅ Good: Reuse client instance
class MyService {
private:
    themisdb::ClientPtr client_;
    
public:
    MyService() {
        client_ = themisdb::Client::create("http://localhost:8765");
    }
    
    void process_users() {
        auto result = client_->query("FOR doc IN users RETURN doc");
        // Process result...
    }
};

// ❌ Bad: Create new client for each request
void process_users_bad() {
    auto client = themisdb::Client::create("http://localhost:8765");
    auto result = client->query("FOR doc IN users RETURN doc");
}
```

### Batch Operations

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    // ❌ Bad: Many individual requests
    for (const auto& key : keys) {
        auto doc = client->get("users", key);
        // Process...
    }
    
    // ✅ Good: Single batch request
    auto batch_result = client->batch_get("users", keys);
    for (const auto& doc : batch_result.found) {
        // Process...
    }
    
    return 0;
}
```

### Async Operations

```cpp
#include <themisdb/client.h>
#include <future>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    // Async query
    auto future = client->query_async(R"(
        FOR doc IN large_collection
          RETURN doc
    )");
    
    // Do other work while query executes...
    do_other_work();
    
    // Wait for result
    auto result = future.get();
    
    // Parallel queries
    std::vector<std::future<themisdb::QueryResult>> futures;
    
    for (const auto& collection : collections) {
        futures.push_back(client->query_async(
            "FOR doc IN " + collection + " RETURN doc"
        ));
    }
    
    // Gather results
    for (auto& future : futures) {
        auto result = future.get();
        process(result);
    }
    
    return 0;
}
```

### Prepared Statements

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    // Prepare statement once
    auto stmt = client->prepare(R"(
        FOR doc IN users
          FILTER doc.age > @min_age
          RETURN doc
    )");
    
    // Execute multiple times with different parameters
    auto result1 = stmt->execute({{"min_age", 25}});
    auto result2 = stmt->execute({{"min_age", 30}});
    auto result3 = stmt->execute({{"min_age", 35}});
    
    return 0;
}
```

---

## Error Handling

### Exception Handling

```cpp
#include <themisdb/client.h>
#include <themisdb/exceptions.h>

int main() {
    try {
        auto client = themisdb::Client::create("http://localhost:8765");
        auto result = client->query("FOR doc IN users RETURN doc");
        
    } catch (const themisdb::ConnectionError& e) {
        std::cerr << "Connection failed: " << e.what() << std::endl;
        // Retry logic...
        
    } catch (const themisdb::QueryError& e) {
        std::cerr << "Query error: " << e.what() << std::endl;
        std::cerr << "Error code: " << e.code() << std::endl;
        
    } catch (const themisdb::AuthenticationError& e) {
        std::cerr << "Authentication failed: " << e.what() << std::endl;
        // Re-authenticate...
        
    } catch (const themisdb::ClientError& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
    
    return 0;
}
```

### Retry Logic

```cpp
#include <themisdb/client.h>

template<typename Func>
auto retry_with_backoff(Func func, int max_attempts = 3) {
    int attempt = 0;
    while (true) {
        try {
            return func();
        } catch (const themisdb::ConnectionError& e) {
            if (++attempt >= max_attempts) {
                throw;
            }
            
            auto delay = std::chrono::milliseconds(100 * std::pow(2, attempt));
            std::this_thread::sleep_for(delay);
        }
    }
}

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    auto result = retry_with_backoff([&]() {
        return client->query("FOR doc IN users RETURN doc");
    });
    
    return 0;
}
```

---

## Advanced Features

### Vector Search

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    std::vector<float> query_vector = {0.1, 0.2, 0.3, /* ... */};
    
    themisdb::VectorSearchOptions options;
    options.top_k = 10;
    options.metric = themisdb::VectorMetric::COSINE;
    
    auto results = client->vector_search("embeddings", query_vector, options);
    
    for (const auto& result : results) {
        std::cout << "Key: " << result.key 
                  << ", Score: " << result.score << std::endl;
    }
    
    return 0;
}
```

### Transactions

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    auto tx = client->begin_transaction();
    
    try {
        // Multiple operations in transaction
        tx->insert("users", "user-123", {{"name", "Alice"}});
        tx->insert("orders", "order-456", {{"user_id", "user-123"}});
        
        // Commit transaction
        tx->commit();
        
    } catch (const std::exception& e) {
        // Rollback on error
        tx->rollback();
        throw;
    }
    
    return 0;
}
```

### Graph Traversal

```cpp
#include <themisdb/client.h>

int main() {
    auto client = themisdb::Client::create("http://localhost:8765");
    
    auto result = client->query(R"(
        FOR v, e, p IN 1..3 OUTBOUND @start_vertex edges
          RETURN {vertex: v, edge: e, path: p}
    )", {{"start_vertex", "users/alice"}});
    
    for (const auto& item : result.entities) {
        std::cout << "Vertex: " << item["vertex"]["_key"] << std::endl;
    }
    
    return 0;
}
```

---

## Siehe auch

- [C# Client SDK](clients_csharp_sdk.md)
- [REST API Documentation](clients_rest_api.md)
- [Python SDK](clients_python_sdk.md)
- [Performance Best Practices](../performance/PERFORMANCE_CLIENT_OPTIMIZATION.md)
