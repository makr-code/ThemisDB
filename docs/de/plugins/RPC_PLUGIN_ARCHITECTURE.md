# RPC Plugin-Architektur für ThemisDB

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔌 Plugins  
**Status:** Design & Implementierung

---

## 📑 Table of Contents

- [Executive Summary](#executive-summary)
- [Architecture](#architecture)
- [Implementation](#implementation)

---

## Executive Summary

Dieses Dokument beschreibt die Architektur eines RPC (Remote Procedure Call) Frameworks für ThemisDB, das als Plugin-System implementiert wird. Das RPC-Framework ermöglicht es ThemisDB, verschiedene RPC-Protokolle (gRPC, Apache Thrift, JSON-RPC, etc.) über ein einheitliches Plugin-Interface zu unterstützen.

**Hauptmerkmale:**
- 🔌 **Plugin-basierte Architektur** - Verschiedene RPC-Protokolle als austauschbare Plugins
- 🚀 **Hohe Performance** - Binäre Protokolle (gRPC, Thrift) für minimale Latenz
- 🔒 **Sichere Integration** - Authentifizierung, Autorisierung, TLS-Verschlüsselung
- 🌐 **Multi-Protokoll Support** - gRPC, Thrift, JSON-RPC, Custom Binary Protocol
- 📊 **Observability** - Metrics, Tracing, Logging für alle RPC-Aufrufe
- 🔄 **Hot-Reload** - Plugins können zur Laufzeit geladen/entladen werden

---

## 1. Motivation & Ziele

### 1.1 Warum ein RPC-Framework?

ThemisDB bietet derzeit folgende Kommunikationsprotokolle:
- ✅ HTTP/REST API (Port 8765)
- ✅ WebSocket API
- ✅ Wire Protocol v1 (binäres Custom Protocol, siehe `wire_protocol_v1.md`)

**Limitierungen:**
- HTTP/REST: Hoher Overhead für einfache Operationen (JSON-Serialisierung, HTTP-Header)
- WebSocket: Gut für Streaming, aber nicht optimal für Request/Response
- Wire Protocol v1: Custom-Lösung, keine Standard-Clients verfügbar

**Ziele des RPC-Frameworks:**
1. **Standard-Kompatibilität** - Unterstützung für gRPC und andere Standard-RPC-Protokolle
2. **Performance** - 5-10x schneller als HTTP/REST für einfache Operationen
3. **Interoperabilität** - Standard-Clients in allen Sprachen (Python, Go, Java, JavaScript, etc.)
4. **Flexibilität** - Verschiedene RPC-Protokolle für verschiedene Use-Cases
5. **Erweiterbarkeit** - Neue RPC-Protokolle können als Plugins hinzugefügt werden

### 1.2 Use Cases

| Use Case | Empfohlenes RPC-Protokoll | Begründung |
|----------|---------------------------|------------|
| **Microservices** | gRPC | Standard, hohe Performance, HTTP/2, Streaming |
| **Legacy-Systeme** | Apache Thrift | Breite Sprachunterstützung, kompakt |
| **Web-Frontends** | JSON-RPC over HTTP | Einfach, JavaScript-kompatibel |
| **IoT/Embedded** | Custom Binary Protocol | Minimaler Overhead, kleine Payload |
| **Inter-Database Replication** | Wire Protocol v1 | Optimiert für ThemisDB-Datenstrukturen |

---

## 2. Architektur-Überblick

### 2.1 Plugin-basiertes Design

```
ThemisDB Server Core
       │
       ├── RPC Plugin Manager
       │       │
       │       ├── gRPC Plugin (Port 50051)
       │       │   └── Protobuf Service Definitions
       │       │
       │       ├── Thrift Plugin (Port 9090)
       │       │   └── Thrift IDL Definitions
       │       │
       │       ├── JSON-RPC Plugin (Port 8081)
       │       │   └── HTTP Transport
       │       │
       │       └── Custom Binary Plugin (Port 8766)
       │           └── Wire Protocol v1 (existing)
       │
       └── ThemisDB Core APIs
           ├── Storage Engine (RocksDB)
           ├── Query Engine (AQL)
           ├── Transaction Manager
           ├── Vector Search (FAISS)
           └── Authentication & Authorization
```

### 2.2 Plugin-Architektur

RPC-Plugins erweitern das bestehende ThemisDB Plugin-System:

```cpp
// include/plugins/rpc_plugin_interface.h

namespace themis {
namespace plugins {
namespace rpc {

/**
 * @brief RPC Plugin Type
 */
enum class RPCProtocol {
    GRPC,           // Google gRPC
    THRIFT,         // Apache Thrift
    JSON_RPC,       // JSON-RPC 2.0
    MSGPACK_RPC,    // MessagePack-RPC
    WIRE_PROTOCOL,  // ThemisDB Wire Protocol v1
    CUSTOM          // Custom implementations
};

/**
 * @brief RPC Server Configuration
 */
struct RPCServerConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 0;
    bool tls_enabled = false;
    std::string tls_cert_path;
    std::string tls_key_path;
    bool auth_required = true;
    size_t max_connections = 1000;
    size_t thread_pool_size = 8;
    std::string namespace_default = "default";
};

/**
 * @brief RPC Server Interface
 * 
 * All RPC plugins must implement this interface
 */
class IRPCServer {
public:
    virtual ~IRPCServer() = default;
    
    /**
     * @brief Get RPC protocol type
     */
    virtual RPCProtocol getProtocol() const = 0;
    
    /**
     * @brief Initialize RPC server
     */
    virtual bool initialize(const RPCServerConfig& config) = 0;
    
    /**
     * @brief Start RPC server (non-blocking)
     */
    virtual bool start() = 0;
    
    /**
     * @brief Stop RPC server gracefully
     */
    virtual void stop() = 0;
    
    /**
     * @brief Check if server is running
     */
    virtual bool isRunning() const = 0;
    
    /**
     * @brief Get server statistics
     */
    virtual RPCServerStats getStats() const = 0;
    
    /**
     * @brief Register a service implementation
     * 
     * For gRPC: Register gRPC service
     * For Thrift: Register Thrift processor
     * For JSON-RPC: Register method handlers
     */
    virtual void registerService(void* service_impl) = 0;
};

/**
 * @brief RPC Plugin Interface
 * 
 * Extends IThemisPlugin for RPC-specific functionality
 */
class IRPCPlugin : public IThemisPlugin {
public:
    /**
     * @brief Create RPC server instance
     */
    virtual std::unique_ptr<IRPCServer> createServer() = 0;
    
    /**
     * @brief Get RPC protocol
     */
    virtual RPCProtocol getProtocol() const = 0;
    
    /**
     * @brief Get default port for this RPC protocol
     */
    virtual uint16_t getDefaultPort() const = 0;
};

/**
 * @brief RPC Server Statistics
 */
struct RPCServerStats {
    uint64_t total_requests = 0;
    uint64_t successful_requests = 0;
    uint64_t failed_requests = 0;
    uint64_t active_connections = 0;
    double avg_latency_ms = 0.0;
    double p95_latency_ms = 0.0;
    double p99_latency_ms = 0.0;
};

} // namespace rpc
} // namespace plugins
} // namespace themis
```

---

## 3. Plugin-Implementierungen

### 3.1 gRPC Plugin

**Datei:** `src/rpc_grpc/grpc_plugin.cpp`

**Service Definition (Protobuf):**

```protobuf
// plugins/rpc/grpc/themis.proto

syntax = "proto3";

package themis.rpc;

// ThemisDB Core Service
service ThemisService {
    // Authentication
    rpc Authenticate(AuthRequest) returns (AuthResponse);
    
    // Basic CRUD Operations
    rpc Get(GetRequest) returns (GetResponse);
    rpc Put(PutRequest) returns (PutResponse);
    rpc Delete(DeleteRequest) returns (DeleteResponse);
    rpc BatchGet(BatchGetRequest) returns (BatchGetResponse);
    rpc BatchPut(BatchPutRequest) returns (BatchPutResponse);
    
    // Query Operations
    rpc QueryAQL(QueryRequest) returns (stream QueryResult);
    
    // Vector Search
    rpc VectorSearch(VectorSearchRequest) returns (VectorSearchResponse);
    
    // Graph Operations
    rpc GraphTraverse(GraphTraverseRequest) returns (GraphTraverseResponse);
    
    // Geo Operations
    rpc GeoQuery(GeoQueryRequest) returns (GeoQueryResponse);
    
    // Time Series
    rpc TimeSeriesQuery(TimeSeriesQueryRequest) returns (TimeSeriesQueryResponse);
    
    // Transaction Management
    rpc BeginTransaction(TransactionBeginRequest) returns (TransactionBeginResponse);
    rpc CommitTransaction(TransactionCommitRequest) returns (TransactionCommitResponse);
    rpc AbortTransaction(TransactionAbortRequest) returns (TransactionAbortResponse);
    
    // Health Check
    rpc HealthCheck(HealthCheckRequest) returns (HealthCheckResponse);
}

message AuthRequest {
    string username = 1;
    string password = 2;  // Hashed
    string namespace = 3;
}

message AuthResponse {
    bool success = 1;
    string token = 2;     // JWT token
    string error = 3;
}

message GetRequest {
    string model = 1;
    string collection = 2;
    string uuid = 3;
    bool decrypt = 4;
    repeated string fields = 5;  // Projection
}

message GetResponse {
    bool found = 1;
    bytes entity = 2;      // Serialized entity (JSON or MessagePack)
    uint64 version = 3;
    uint64 timestamp_ns = 4;
    string error = 5;
}

message PutRequest {
    string model = 1;
    string collection = 2;
    string uuid = 3;
    bytes entity = 4;
    bool encrypt = 5;
    uint64 expected_version = 6;  // CAS
}

message PutResponse {
    bool success = 1;
    uint64 version = 2;
    string error = 3;
}

message QueryRequest {
    string aql = 1;
    map<string, bytes> bind_vars = 2;
    uint32 batch_size = 3;
}

message QueryResult {
    repeated bytes results = 1;
    bool has_more = 2;
    string cursor_id = 3;
    uint64 total_count = 4;
}

message VectorSearchRequest {
    string collection = 1;
    repeated float vector = 2;
    uint32 k = 3;
    string distance_metric = 4;  // cosine, euclidean, dot
    map<string, bytes> filters = 5;
}

message VectorSearchResponse {
    repeated VectorResult results = 1;
    string error = 2;
}

message VectorResult {
    string uuid = 1;
    float distance = 2;
    bytes entity = 3;
}

message HealthCheckRequest {
}

message HealthCheckResponse {
    enum Status {
        SERVING = 0;
        NOT_SERVING = 1;
        UNKNOWN = 2;
    }
    Status status = 1;
    string version = 2;
    map<string, string> metadata = 3;
}
```

**Plugin Implementation:**

```cpp
// src/rpc_grpc/grpc_plugin.cpp

#include "plugins/rpc_plugin_interface.h"
#include <grpcpp/grpcpp.h>
#include "themis.grpc.pb.h"

namespace themis {
namespace plugins {
namespace rpc {

class GRPCServer : public IRPCServer {
private:
    std::unique_ptr<grpc::Server> server_;
    RPCServerConfig config_;
    std::atomic<bool> running_{false};
    RPCServerStats stats_;
    std::mutex stats_mutex_;
    
public:
    RPCProtocol getProtocol() const override {
        return RPCProtocol::GRPC;
    }
    
    bool initialize(const RPCServerConfig& config) override {
        config_ = config;
        return true;
    }
    
    bool start() override {
        grpc::ServerBuilder builder;
        
        // Listen address
        std::string server_address = config_.host + ":" + std::to_string(config_.port);
        
        if (config_.tls_enabled) {
            // TLS credentials
            grpc::SslServerCredentialsOptions ssl_opts;
            ssl_opts.pem_root_certs = "";  // Client cert verification (optional)
            
            grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair;
            // Load from config_.tls_cert_path and config_.tls_key_path
            
            auto creds = grpc::SslServerCredentials(ssl_opts);
            builder.AddListeningPort(server_address, creds);
        } else {
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        }
        
        // Register service (will be set via registerService)
        // builder.RegisterService(&service_impl_);
        
        // Build and start server
        server_ = builder.BuildAndStart();
        
        if (server_) {
            running_ = true;
            THEMIS_INFO("gRPC server listening on {}", server_address);
            return true;
        }
        
        return false;
    }
    
    void stop() override {
        if (server_) {
            server_->Shutdown();
            running_ = false;
            THEMIS_INFO("gRPC server stopped");
        }
    }
    
    bool isRunning() const override {
        return running_;
    }
    
    RPCServerStats getStats() const override {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return stats_;
    }
    
    void registerService(void* service_impl) override {
        // Cast to grpc::Service* and register
        // This will be called from the main server initialization
    }
};

class GRPCPlugin : public IRPCPlugin {
public:
    const char* getName() const override {
        return "grpc";
    }
    
    const char* getVersion() const override {
        return "1.0.0";
    }
    
    PluginType getType() const override {
        return PluginType::CUSTOM;  // Or add RPC_SERVER type
    }
    
    PluginCapabilities getCapabilities() const override {
        PluginCapabilities caps;
        caps.supports_streaming = true;
        caps.thread_safe = true;
        return caps;
    }
    
    bool initialize(const char* config_json) override {
        // Parse config if needed
        return true;
    }
    
    void shutdown() override {
        // Cleanup
    }
    
    void* getInstance() override {
        return this;
    }
    
    std::unique_ptr<IRPCServer> createServer() override {
        return std::make_unique<GRPCServer>();
    }
    
    RPCProtocol getProtocol() const override {
        return RPCProtocol::GRPC;
    }
    
    uint16_t getDefaultPort() const override {
        return 50051;  // Standard gRPC port
    }
};

} // namespace rpc
} // namespace plugins
} // namespace themis

// Export plugin
THEMIS_PLUGIN_IMPL(themis::plugins::rpc::GRPCPlugin)
```

**CMakeLists.txt:**

```cmake
# plugins/rpc/grpc/CMakeLists.txt

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

# Generate Protobuf and gRPC files
set(PROTO_FILES themis.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_FILES})

# Build plugin
add_library(themis_rpc_grpc SHARED
    grpc_plugin.cpp
    grpc_service_impl.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_link_libraries(themis_rpc_grpc PRIVATE
    gRPC::grpc++
    protobuf::libprotobuf
    themis_core
)

set_target_properties(themis_rpc_grpc PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/rpc"
)

install(TARGETS themis_rpc_grpc
    LIBRARY DESTINATION lib/themis/plugins/rpc
)
```

**Plugin Manifest:**

```json
{
  "name": "grpc",
  "version": "1.0.0",
  "type": "rpc_server",
  "description": "gRPC server plugin for ThemisDB",
  "binary": {
    "windows": "themis_rpc_grpc.dll",
    "linux": "themis_rpc_grpc.so",
    "macos": "themis_rpc_grpc.dylib"
  },
  "dependencies": [
    "grpc++ (>=1.50.0)",
    "protobuf (>=3.21.0)"
  ],
  "capabilities": {
    "streaming": true,
    "batching": true,
    "thread_safe": true
  },
  "config_schema": {
    "host": {"type": "string", "default": "0.0.0.0"},
    "port": {"type": "number", "default": 50051},
    "tls_enabled": {"type": "boolean", "default": true},
    "tls_cert_path": {"type": "string"},
    "tls_key_path": {"type": "string"},
    "max_connections": {"type": "number", "default": 1000}
  },
  "auto_load": true,
  "load_priority": 20
}
```

---

### 3.2 Apache Thrift Plugin

**Service Definition (Thrift IDL):**

```thrift
// plugins/rpc/thrift/themis.thrift

namespace cpp themis.rpc
namespace py themis.rpc
namespace java com.themisdb.rpc
namespace go themis.rpc

// Basic Types
struct GetRequest {
    1: string model,
    2: string collection,
    3: string uuid,
    4: bool decrypt,
    5: list<string> fields
}

struct GetResponse {
    1: bool found,
    2: binary entity,
    3: i64 version,
    4: i64 timestamp_ns,
    5: optional string error
}

struct PutRequest {
    1: string model,
    2: string collection,
    3: string uuid,
    4: binary entity,
    5: bool encrypt,
    6: i64 expected_version
}

struct PutResponse {
    1: bool success,
    2: i64 version,
    3: optional string error
}

struct VectorSearchRequest {
    1: string collection,
    2: list<double> vector,
    3: i32 k,
    4: string distance_metric,
    5: map<string, binary> filters
}

struct VectorResult {
    1: string uuid,
    2: double distance,
    3: binary entity
}

struct VectorSearchResponse {
    1: list<VectorResult> results,
    2: optional string error
}

// Service Definition
service ThemisService {
    GetResponse get(1: GetRequest request),
    PutResponse put(1: PutRequest request),
    VectorSearchResponse vectorSearch(1: VectorSearchRequest request),
    // ... more methods
}
```

**Plugin Structure:** Similar to gRPC plugin, but using Apache Thrift library.

---

### 3.3 JSON-RPC Plugin

**Protocol:** JSON-RPC 2.0 over HTTP

**Example Request:**

```json
POST /jsonrpc HTTP/1.1
Content-Type: application/json

{
  "jsonrpc": "2.0",
  "method": "themis.get",
  "params": {
    "model": "documents",
    "collection": "articles",
    "uuid": "doc_123"
  },
  "id": 1
}
```

**Example Response:**

```json
{
  "jsonrpc": "2.0",
  "result": {
    "found": true,
    "entity": {
      "uuid": "doc_123",
      "title": "Article Title",
      "content": "..."
    },
    "version": 42,
    "timestamp_ns": 1702816800000000000
  },
  "id": 1
}
```

**Plugin Implementation:** Uses HTTP server (Boost.Beast or similar) with JSON parsing.

---

## 4. Integration in ThemisDB Core

### 4.1 Server Initialization

```cpp
// src/main_server.cpp

#include "plugins/plugin_manager.h"
#include "plugins/rpc_plugin_interface.h"

int main(int argc, char* argv[]) {
    // Initialize ThemisDB core
    auto& db = themis::ThemisDB::instance();
    db.initialize(config);
    
    // Initialize Plugin Manager
    auto& pm = themis::plugins::PluginManager::instance();
    
    // Scan for RPC plugins
    pm.scanPluginDirectory("./plugins/rpc");
    
    // Auto-load plugins
    pm.autoLoadPlugins();
    
    // Get all RPC plugins
    auto rpc_plugins = pm.getPluginsByType(themis::plugins::PluginType::CUSTOM);
    
    std::vector<std::unique_ptr<themis::plugins::rpc::IRPCServer>> rpc_servers;
    
    for (auto* plugin : rpc_plugins) {
        auto* rpc_plugin = dynamic_cast<themis::plugins::rpc::IRPCPlugin*>(plugin);
        if (rpc_plugin) {
            // Create RPC server
            auto server = rpc_plugin->createServer();
            
            // Configure
            themis::plugins::rpc::RPCServerConfig config;
            config.port = rpc_plugin->getDefaultPort();
            config.tls_enabled = true;
            config.tls_cert_path = "./certs/server.crt";
            config.tls_key_path = "./certs/server.key";
            
            server->initialize(config);
            
            // Register service implementation
            // auto service_impl = new ThemisServiceImpl(&db);
            // server->registerService(service_impl);
            
            // Start server
            if (server->start()) {
                THEMIS_INFO("Started {} server on port {}", 
                    rpc_plugin->getName(), 
                    rpc_plugin->getDefaultPort());
                rpc_servers.push_back(std::move(server));
            }
        }
    }
    
    // Start existing HTTP server
    auto http_server = themis::server::HTTPServer(db);
    http_server.start(8765);
    
    THEMIS_INFO("ThemisDB server started successfully");
    THEMIS_INFO("  HTTP/REST:               http://localhost:8765");
    THEMIS_INFO("  gRPC:                    grpc://localhost:50051");
    THEMIS_INFO("  Thrift:                  thrift://localhost:9090");
    THEMIS_INFO("  JSON-RPC:                http://localhost:8081/jsonrpc");
    
    // Wait for shutdown signal
    waitForShutdownSignal();
    
    // Cleanup
    for (auto& server : rpc_servers) {
        server->stop();
    }
    
    pm.unloadAllPlugins();
    db.shutdown();
    
    return 0;
}
```

### 4.2 Service Implementation

```cpp
// src/server/rpc_service_impl.cpp

#include "themis.grpc.pb.h"
#include "themis/themis_db.h"

namespace themis {
namespace server {

class ThemisServiceImpl final : public themis::rpc::ThemisService::Service {
private:
    ThemisDB* db_;
    
public:
    ThemisServiceImpl(ThemisDB* db) : db_(db) {}
    
    grpc::Status Get(
        grpc::ServerContext* context,
        const themis::rpc::GetRequest* request,
        themis::rpc::GetResponse* response
    ) override {
        try {
            // Authenticate using context metadata
            auto auth_token = context->client_metadata().find("authorization");
            if (auth_token == context->client_metadata().end()) {
                return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Missing auth token");
            }
            
            // Verify token
            auto user = db_->auth().verifyToken(
                std::string(auth_token->second.data(), auth_token->second.size())
            );
            if (!user) {
                return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid token");
            }
            
            // Perform GET operation
            auto entity = db_->storage().get(
                request->model(),
                request->collection(),
                request->uuid()
            );
            
            if (entity) {
                response->set_found(true);
                response->set_entity(entity->serialize());
                response->set_version(entity->version());
                response->set_timestamp_ns(entity->timestamp());
            } else {
                response->set_found(false);
            }
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            response->set_error(e.what());
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status Put(
        grpc::ServerContext* context,
        const themis::rpc::PutRequest* request,
        themis::rpc::PutResponse* response
    ) override {
        try {
            // Authentication (same as GET)
            
            // Perform PUT operation
            auto result = db_->storage().put(
                request->model(),
                request->collection(),
                request->uuid(),
                request->entity(),
                request->encrypt(),
                request->expected_version()
            );
            
            response->set_success(result.success);
            response->set_version(result.version);
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            response->set_error(e.what());
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status VectorSearch(
        grpc::ServerContext* context,
        const themis::rpc::VectorSearchRequest* request,
        themis::rpc::VectorSearchResponse* response
    ) override {
        try {
            // Authentication
            
            // Convert request vector
            std::vector<float> query_vector(
                request->vector().begin(),
                request->vector().end()
            );
            
            // Perform vector search
            auto results = db_->vector_search().search(
                request->collection(),
                query_vector,
                request->k(),
                request->distance_metric()
            );
            
            // Populate response
            for (const auto& result : results) {
                auto* vec_result = response->add_results();
                vec_result->set_uuid(result.uuid);
                vec_result->set_distance(result.distance);
                vec_result->set_entity(result.entity.serialize());
            }
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            response->set_error(e.what());
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status QueryAQL(
        grpc::ServerContext* context,
        const themis::rpc::QueryRequest* request,
        grpc::ServerWriter<themis::rpc::QueryResult>* writer
    ) override {
        try {
            // Authentication
            
            // Execute AQL query with streaming
            auto cursor = db_->query().execute(
                request->aql(),
                request->bind_vars()
            );
            
            while (cursor->hasMore()) {
                themis::rpc::QueryResult result;
                
                auto batch = cursor->nextBatch(request->batch_size());
                for (const auto& entity : batch) {
                    result.add_results(entity.serialize());
                }
                
                result.set_has_more(cursor->hasMore());
                result.set_cursor_id(cursor->id());
                
                writer->Write(result);
            }
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status HealthCheck(
        grpc::ServerContext* context,
        const themis::rpc::HealthCheckRequest* request,
        themis::rpc::HealthCheckResponse* response
    ) override {
        response->set_status(themis::rpc::HealthCheckResponse::SERVING);
        response->set_version(THEMIS_VERSION);
        (*response->mutable_metadata())["uptime"] = std::to_string(db_->uptime());
        return grpc::Status::OK;
    }
};

} // namespace server
} // namespace themis
```

---

## 5. Client-Beispiele

### 5.1 Python gRPC Client

```python
# clients/python/themis_grpc_client.py

import grpc
from themis_pb2 import *
from themis_pb2_grpc import ThemisServiceStub

class ThemisGRPCClient:
    def __init__(self, host='localhost', port=50051, tls=True):
        if tls:
            # Load TLS credentials
            with open('certs/ca.crt', 'rb') as f:
                ca_cert = f.read()
            credentials = grpc.ssl_channel_credentials(ca_cert)
            self.channel = grpc.secure_channel(f'{host}:{port}', credentials)
        else:
            self.channel = grpc.insecure_channel(f'{host}:{port}')
        
        self.stub = ThemisServiceStub(self.channel)
        self.token = None
    
    def authenticate(self, username, password, namespace='default'):
        request = AuthRequest(
            username=username,
            password=password,
            namespace=namespace
        )
        response = self.stub.Authenticate(request)
        if response.success:
            self.token = response.token
            return True
        else:
            raise Exception(f"Authentication failed: {response.error}")
    
    def _metadata(self):
        return [('authorization', f'Bearer {self.token}')]
    
    def get(self, model, collection, uuid, decrypt=False, fields=None):
        request = GetRequest(
            model=model,
            collection=collection,
            uuid=uuid,
            decrypt=decrypt,
            fields=fields or []
        )
        response = self.stub.Get(request, metadata=self._metadata())
        
        if response.found:
            return {
                'entity': response.entity,
                'version': response.version,
                'timestamp_ns': response.timestamp_ns
            }
        else:
            return None
    
    def put(self, model, collection, uuid, entity, encrypt=False, expected_version=0):
        import json
        request = PutRequest(
            model=model,
            collection=collection,
            uuid=uuid,
            entity=json.dumps(entity).encode('utf-8'),
            encrypt=encrypt,
            expected_version=expected_version
        )
        response = self.stub.Put(request, metadata=self._metadata())
        
        if response.success:
            return {'version': response.version}
        else:
            raise Exception(f"Put failed: {response.error}")
    
    def vector_search(self, collection, vector, k=10, distance_metric='cosine'):
        request = VectorSearchRequest(
            collection=collection,
            vector=vector,
            k=k,
            distance_metric=distance_metric
        )
        response = self.stub.VectorSearch(request, metadata=self._metadata())
        
        return [
            {
                'uuid': result.uuid,
                'distance': result.distance,
                'entity': result.entity
            }
            for result in response.results
        ]
    
    def query(self, aql, bind_vars=None, batch_size=100):
        request = QueryRequest(
            aql=aql,
            bind_vars=bind_vars or {},
            batch_size=batch_size
        )
        
        results = []
        for response in self.stub.QueryAQL(request, metadata=self._metadata()):
            results.extend(response.results)
        
        return results
    
    def close(self):
        self.channel.close()

# Usage
client = ThemisGRPCClient(host='localhost', port=50051, tls=True)
client.authenticate('admin', 'secret')

# Get operation
entity = client.get(model='documents', collection='articles', uuid='doc_123')
print(entity)

# Put operation
result = client.put(
    model='documents',
    collection='articles',
    uuid='doc_456',
    entity={'title': 'New Article', 'content': 'Lorem ipsum...'}
)
print(f"Saved with version: {result['version']}")

# Vector search
results = client.vector_search(
    collection='embeddings',
    vector=[0.1, 0.2, 0.3, ...],  # 384-dim vector
    k=10,
    distance_metric='cosine'
)
for r in results:
    print(f"UUID: {r['uuid']}, Distance: {r['distance']}")

# AQL Query
results = client.query("""
    FOR doc IN articles
        FILTER doc.published == true
        SORT doc.created_at DESC
        LIMIT 100
        RETURN doc
""")

client.close()
```

### 5.2 Go gRPC Client

```go
// clients/go/themis_client.go

package themis

import (
    "context"
    "crypto/tls"
    "crypto/x509"
    "io/ioutil"
    
    "google.golang.org/grpc"
    "google.golang.org/grpc/credentials"
    "google.golang.org/grpc/metadata"
    
    pb "github.com/themisdb/themis-go/proto"
)

type ThemisClient struct {
    conn   *grpc.ClientConn
    client pb.ThemisServiceClient
    token  string
}

func NewThemisClient(host string, port int, tlsEnabled bool) (*ThemisClient, error) {
    var opts []grpc.DialOption
    
    if tlsEnabled {
        caCert, err := ioutil.ReadFile("certs/ca.crt")
        if err != nil {
            return nil, err
        }
        certPool := x509.NewCertPool()
        certPool.AppendCertsFromPEM(caCert)
        
        creds := credentials.NewTLS(&tls.Config{
            RootCAs: certPool,
        })
        opts = append(opts, grpc.WithTransportCredentials(creds))
    } else {
        opts = append(opts, grpc.WithInsecure())
    }
    
    conn, err := grpc.Dial(fmt.Sprintf("%s:%d", host, port), opts...)
    if err != nil {
        return nil, err
    }
    
    return &ThemisClient{
        conn:   conn,
        client: pb.NewThemisServiceClient(conn),
    }, nil
}

func (c *ThemisClient) Authenticate(username, password, namespace string) error {
    ctx := context.Background()
    
    req := &pb.AuthRequest{
        Username:  username,
        Password:  password,
        Namespace: namespace,
    }
    
    resp, err := c.client.Authenticate(ctx, req)
    if err != nil {
        return err
    }
    
    if !resp.Success {
        return fmt.Errorf("authentication failed: %s", resp.Error)
    }
    
    c.token = resp.Token
    return nil
}

func (c *ThemisClient) context() context.Context {
    md := metadata.Pairs("authorization", "Bearer "+c.token)
    return metadata.NewOutgoingContext(context.Background(), md)
}

func (c *ThemisClient) Get(model, collection, uuid string) (*pb.GetResponse, error) {
    req := &pb.GetRequest{
        Model:      model,
        Collection: collection,
        Uuid:       uuid,
    }
    
    return c.client.Get(c.context(), req)
}

func (c *ThemisClient) Put(model, collection, uuid string, entity []byte) (*pb.PutResponse, error) {
    req := &pb.PutRequest{
        Model:      model,
        Collection: collection,
        Uuid:       uuid,
        Entity:     entity,
    }
    
    return c.client.Put(c.context(), req)
}

func (c *ThemisClient) VectorSearch(collection string, vector []float32, k int32) ([]*pb.VectorResult, error) {
    req := &pb.VectorSearchRequest{
        Collection:     collection,
        Vector:         vector,
        K:              k,
        DistanceMetric: "cosine",
    }
    
    resp, err := c.client.VectorSearch(c.context(), req)
    if err != nil {
        return nil, err
    }
    
    return resp.Results, nil
}

func (c *ThemisClient) Close() error {
    return c.conn.Close()
}
```

---

## 6. Konfiguration

### 6.1 Server-Konfiguration (config.yaml)

```yaml
# ThemisDB Configuration

rpc:
  # Enable RPC plugins
  enabled: true
  
  # Plugin directory
  plugin_directory: "./plugins/rpc"
  
  # Auto-load plugins
  auto_load: true
  
  # RPC servers configuration
  servers:
    grpc:
      enabled: true
      host: "0.0.0.0"
      port: 50051
      tls:
        enabled: true
        cert_path: "./certs/server.crt"
        key_path: "./certs/server.key"
        ca_cert_path: "./certs/ca.crt"
      max_connections: 1000
      thread_pool_size: 8
      
    thrift:
      enabled: true
      host: "0.0.0.0"
      port: 9090
      tls:
        enabled: true
      max_connections: 500
      thread_pool_size: 4
      
    jsonrpc:
      enabled: true
      host: "0.0.0.0"
      port: 8081
      tls:
        enabled: false
      max_connections: 500

  # Authentication
  auth:
    required: true
    token_expiry: 3600  # seconds
    
  # Rate limiting
  rate_limit:
    enabled: true
    requests_per_second: 1000
    burst_size: 100
```

---

## 7. Security

### 7.1 Authentifizierung

Alle RPC-Plugins müssen Authentifizierung unterstützen:

1. **Initial Authentication** - Username/Password → JWT Token
2. **Request Authentication** - JWT Token in Metadata/Headers
3. **Token Expiry** - Tokens haben eine begrenzte Gültigkeit
4. **Token Refresh** - Clients können Tokens erneuern

### 7.2 Autorisierung

RBAC (Role-Based Access Control) wird auf RPC-Ebene durchgesetzt:

```cpp
// Check permissions before executing operation
if (!user->hasPermission("documents.articles.read")) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "Insufficient permissions");
}
```

### 7.3 TLS/SSL

- **Mutual TLS** - Server und Client authentifizieren sich gegenseitig
- **Certificate Verification** - Zertifikate werden gegen CA geprüft
- **Cipher Suites** - Nur starke Verschlüsselungsalgorithmen

### 7.4 Rate Limiting

- **Per-User Rate Limits** - Verhindert Missbrauch
- **Connection Limits** - Max. Anzahl gleichzeitiger Verbindungen
- **Request Size Limits** - Verhindert DoS-Angriffe

---

## 8. Performance

### 8.1 Benchmarks (Ziele)

| Metrik | HTTP/REST | gRPC | Thrift | Wire Protocol v1 |
|--------|-----------|------|--------|------------------|
| **GET Latency (p50)** | 1.5ms | 0.3ms | 0.4ms | 0.3ms |
| **GET Latency (p99)** | 5ms | 1ms | 1.2ms | 1ms |
| **Throughput (ops/sec)** | 1,000 | 10,000 | 8,000 | 12,000 |
| **Connection Overhead** | 3-5ms | 0.5ms | 0.6ms | 0.5ms |
| **Payload Size (1KB entity)** | 1.5KB | 1.1KB | 1.0KB | 1.0KB |

### 8.2 Optimierungen

1. **Connection Pooling** - Wiederverwendung von Verbindungen
2. **Zero-Copy** - Minimierung von Speicher-Kopien
3. **Async I/O** - Non-blocking Operations
4. **Thread Pool** - Begrenzte Anzahl Worker-Threads
5. **Message Compression** - Optional für große Payloads

---

## 9. Observability

### 9.1 Metrics

OpenTelemetry-Metriken für alle RPC-Operationen:

```cpp
// Metrics to track
- rpc_requests_total{protocol="grpc", method="Get", status="success"}
- rpc_requests_duration_seconds{protocol="grpc", method="Get"}
- rpc_active_connections{protocol="grpc"}
- rpc_bytes_sent_total{protocol="grpc"}
- rpc_bytes_received_total{protocol="grpc"}
```

### 9.2 Tracing

Distributed Tracing mit OpenTelemetry:

```cpp
// Example: Trace RPC request
auto span = tracer->StartSpan("grpc.Get");
span->SetAttribute("model", request->model());
span->SetAttribute("collection", request->collection());
span->SetAttribute("uuid", request->uuid());

// Execute operation
auto result = db_->storage().get(...);

span->End();
```

### 9.3 Logging

Strukturiertes Logging für alle RPC-Operationen:

```json
{
  "timestamp": "2025-12-17T10:30:00Z",
  "level": "INFO",
  "protocol": "grpc",
  "method": "Get",
  "model": "documents",
  "collection": "articles",
  "uuid": "doc_123",
  "duration_ms": 2.3,
  "status": "success",
  "user": "admin"
}
```

---

## 10. Deployment

### 10.1 Docker

```dockerfile
# Dockerfile.rpc

FROM themisdb/themisdb:latest

# Install RPC plugins
COPY plugins/rpc /usr/local/lib/themis/plugins/rpc

# Expose RPC ports
EXPOSE 50051  # gRPC
EXPOSE 9090   # Thrift
EXPOSE 8081   # JSON-RPC

CMD ["themis_server", "--config", "/etc/themis/config.yaml"]
```

### 10.2 Kubernetes

```yaml
# k8s/themis-rpc-deployment.yaml

apiVersion: v1
kind: Service
metadata:
  name: themisdb-grpc
spec:
  type: LoadBalancer
  ports:
    - port: 50051
      targetPort: 50051
      protocol: TCP
      name: grpc
  selector:
    app: themisdb

---

apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb
spec:
  replicas: 3
  selector:
    matchLabels:
      app: themisdb
  template:
    metadata:
      labels:
        app: themisdb
    spec:
      containers:
      - name: themisdb
        image: themisdb/themisdb:1.2.0-rpc
        ports:
        - containerPort: 50051
          name: grpc
        - containerPort: 9090
          name: thrift
        - containerPort: 8081
          name: jsonrpc
        volumeMounts:
        - name: config
          mountPath: /etc/themis
        - name: data
          mountPath: /data
      volumes:
      - name: config
        configMap:
          name: themisdb-config
      - name: data
        persistentVolumeClaim:
          claimName: themisdb-data
```

---

## 11. Testing

### 11.1 Unit Tests

```cpp
// tests/rpc/test_grpc_plugin.cpp

#include <gtest/gtest.h>
#include "plugins/rpc/grpc_plugin.h"

TEST(GRPCPluginTest, PluginLifecycle) {
    auto plugin = std::make_unique<themis::plugins::rpc::GRPCPlugin>();
    
    EXPECT_STREQ(plugin->getName(), "grpc");
    EXPECT_EQ(plugin->getProtocol(), themis::plugins::rpc::RPCProtocol::GRPC);
    EXPECT_EQ(plugin->getDefaultPort(), 50051);
    
    EXPECT_TRUE(plugin->initialize("{}"));
    
    auto server = plugin->createServer();
    EXPECT_NE(server, nullptr);
    
    plugin->shutdown();
}

TEST(GRPCPluginTest, ServerStartStop) {
    auto plugin = std::make_unique<themis::plugins::rpc::GRPCPlugin>();
    plugin->initialize("{}");
    
    auto server = plugin->createServer();
    
    themis::plugins::rpc::RPCServerConfig config;
    config.host = "127.0.0.1";
    config.port = 50052;  // Different port for testing
    
    EXPECT_TRUE(server->initialize(config));
    EXPECT_TRUE(server->start());
    EXPECT_TRUE(server->isRunning());
    
    server->stop();
    EXPECT_FALSE(server->isRunning());
}
```

### 11.2 Integration Tests

```python
# tests/integration/test_grpc_client.py

import pytest
from themis_grpc_client import ThemisGRPCClient

@pytest.fixture
def client():
    client = ThemisGRPCClient(host='localhost', port=50051, tls=False)
    client.authenticate('admin', 'secret')
    yield client
    client.close()

def test_get_operation(client):
    # Put test data
    result = client.put(
        model='documents',
        collection='test',
        uuid='test_doc_1',
        entity={'title': 'Test', 'content': 'Test content'}
    )
    assert result['version'] > 0
    
    # Get data
    entity = client.get(
        model='documents',
        collection='test',
        uuid='test_doc_1'
    )
    assert entity is not None
    assert entity['entity'] is not None

def test_vector_search(client):
    # Index test vectors
    for i in range(10):
        client.put(
            model='vectors',
            collection='embeddings',
            uuid=f'vec_{i}',
            entity={
                'vector': [0.1 * i] * 384,
                'text': f'Document {i}'
            }
        )
    
    # Search
    results = client.vector_search(
        collection='embeddings',
        vector=[0.5] * 384,
        k=5
    )
    assert len(results) == 5
    assert all(r['distance'] >= 0 for r in results)

def test_query(client):
    results = client.query("""
        FOR doc IN test
            FILTER doc.title == 'Test'
            RETURN doc
    """)
    assert len(results) > 0
```

---

## 12. Roadmap

### Phase 1: Foundation (Week 1-2) ✅
- [x] RPC Plugin Interface Definition
- [x] Plugin Manager Integration
- [x] Architecture Documentation

### Phase 2: gRPC Implementation (Week 3-4)
- [ ] Protobuf Service Definitions
- [ ] gRPC Server Plugin
- [ ] gRPC Service Implementation
- [ ] Python gRPC Client
- [ ] Go gRPC Client

### Phase 3: Thrift Implementation (Week 5-6)
- [ ] Thrift IDL Definitions
- [ ] Thrift Server Plugin
- [ ] Java Thrift Client
- [ ] Python Thrift Client

### Phase 4: JSON-RPC Implementation (Week 7)
- [ ] JSON-RPC Server Plugin
- [ ] JavaScript Client
- [ ] Python Client

### Phase 5: Testing & Optimization (Week 8-9)
- [ ] Unit Tests
- [ ] Integration Tests
- [ ] Performance Benchmarks
- [ ] Load Testing
- [ ] Security Audit

### Phase 6: Documentation & Release (Week 10)
- [ ] API Documentation
- [ ] Client Libraries
- [ ] Example Applications
- [ ] Production Deployment Guide

---

## 13. Best Practices

### 13.1 Plugin Development

1. **Follow Plugin Interface** - Implementieren Sie alle erforderlichen Methoden
2. **Error Handling** - Geben Sie aussagekräftige Fehlermeldungen zurück
3. **Resource Management** - Cleanup in `shutdown()`
4. **Thread Safety** - Alle Methoden müssen thread-safe sein
5. **Logging** - Verwenden Sie strukturiertes Logging

### 13.2 Service Implementation

1. **Authentication First** - Immer Authentifizierung prüfen
2. **Authorization Check** - RBAC vor jeder Operation
3. **Input Validation** - Validieren Sie alle Eingaben
4. **Error Handling** - Catch alle Exceptions
5. **Metrics & Tracing** - Instrumentieren Sie alle Operationen

### 13.3 Client Development

1. **Connection Pooling** - Wiederverwenden Sie Verbindungen
2. **Retry Logic** - Implementieren Sie exponentielles Backoff
3. **Timeout Configuration** - Setzen Sie angemessene Timeouts
4. **TLS Verification** - Verifizieren Sie Server-Zertifikate
5. **Token Management** - Erneuern Sie Tokens vor Ablauf

---

## 14. Zusammenfassung

Das RPC Plugin-Framework für ThemisDB bietet:

✅ **Flexibilität** - Verschiedene RPC-Protokolle je nach Anforderung  
✅ **Performance** - 5-10x schneller als HTTP/REST  
✅ **Standard-Kompatibilität** - gRPC, Thrift, JSON-RPC  
✅ **Sicherheit** - TLS, Authentifizierung, Autorisierung  
✅ **Erweiterbarkeit** - Neue Protokolle als Plugins  
✅ **Observability** - Metrics, Tracing, Logging  

**Nächste Schritte:**
1. Protobuf Service Definitions erstellen
2. gRPC Plugin implementieren
3. Service Implementation
4. Client Libraries
5. Testing & Benchmarking

---

**Weitere Dokumentation:**
- [Plugin System Overview](README.md)
- [Wire Protocol v1](../architecture/wire_protocol_v1.md)
- [Plugin Migration Guide](PLUGIN_MIGRATION.md)
- [Security Architecture](../architecture/SECURITY_ARCHITECTURE.md)
- [Performance Benchmarks](../README.md)
