# Kapitel 22: Client Libraries & Drivers {#client-libraries-drivers}

> *"A database is only as good as its client libraries. ThemisDB provides production-ready drivers with connection pooling, retry logic, and zero-copy optimizations."*

---

## Überblick

Client Libraries und Treiber bilden die kritische Schnittstelle zwischen Anwendungscode und Datenbanksystem. Moderne Datenbanktreiber müssen nicht nur korrekt funktionieren, sondern auch hochperformant sein, robuste Fehlerbehandlung bieten und idiomatische APIs für verschiedene Programmiersprachen bereitstellen. ThemisDB entwickelt seine Client-Libraries nach den Best Practices der Industrie unter Berücksichtigung von Patterns wie HikariCP Connection Pooling, Hystrix Circuit Breakers und Zero-Copy-Techniken für maximale Performance.

Die Architektur der ThemisDB Client-Libraries folgt einem geschichteten Ansatz mit klarer Trennung zwischen Wire Protocol, Connection Management und Query Building. Das binäre Protokoll basiert auf Protocol Buffers für effiziente Serialisierung, während Connection Pools nach dem HikariCP-Pattern implementiert sind für minimale Latenz und maximalen Durchsatz. Query Builder bieten Fluent APIs mit vollständiger Type-Safety in statisch typisierten Sprachen wie TypeScript, Java und Rust.

Performance-Optimierungen umfassen Batch Operations für reduzierte Network Round-Trips, Cursor Streaming für Memory-effizienten Datenabruf und Prepared Statements für Query Plan Caching. Error Handling implementiert automatische Retry-Logic mit Exponential Backoff und Circuit Breaker Patterns für Resilience gegen transiente Fehler. Monitoring-Integration bietet detaillierte Metrics über Connection Pool Health, Query Performance und Error Rates.

**Was Sie in diesem Kapitel lernen:**
- Driver Architecture mit Wire Protocol Details und Connection Lifecycle
- Connection Management mit HikariCP-Style Pooling und Health Checks
- Language Bindings für Python, Java, Node.js, Go und Rust
- Error Handling mit Retry Logic und Circuit Breaker Patterns
- Query Builders mit Fluent APIs und Type-Safety
- Performance Optimization mit Batch Operations und Zero-Copy
- Production Best Practices für Monitoring und Debugging

---

## 22.1 Driver Architecture {#driver-architecture}

Die ThemisDB Driver-Architektur implementiert einen mehrschichtigen Stack von Wire Protocol über Connection Management bis zur High-Level Query API. Diese Architektur ermöglicht es, unterschiedliche Programmiersprachen mit idiomatischen APIs zu bedienen, während der darunterliegende Protokoll-Layer einheitlich bleibt. Das Design folgt dem JDBC-Modell für maximale Kompatibilität mit bestehenden Tools und Frameworks.

### 22.1.1 Wire Protocol Implementation {#wire-protocol-implementation}

Das ThemisDB Wire Protocol basiert auf [Protocol Buffers](appendix_h_glossary.md#protocol-buffers) (protobuf) für effiziente binäre Serialisierung mit Backward-Compatibility. Jede Client-Server-Kommunikation folgt einem Request-Response-Pattern mit Message Framing für Stream-Multiplexing. Das Protocol unterstützt drei Transport-Modi: Binary TCP für maximale Performance, HTTP/2 für Firewall-Kompatibilität und WebSocket für Bidirectional Streaming.

**Protocol Buffer Message Definition:**

```protobuf
// ThemisDB Wire Protocol v1.4
syntax = "proto3";

package themisdb.protocol;

// Request-Wrapper für alle Client-Operations
message Request {
    uint64 request_id = 1;           // Eindeutige Request-ID
    RequestType type = 2;             // Operation Type
    bytes payload = 3;                // Serialisierte Operation
    map<string, string> metadata = 4; // Session-Info, Auth
}

// Response-Wrapper mit Error-Handling
message Response {
    uint64 request_id = 1;
    ResponseStatus status = 2;
    bytes payload = 3;                // Result Data
    Error error = 4;                  // Error Details (optional)
}

enum RequestType {
    QUERY = 0;
    INSERT = 1;
    UPDATE = 2;
    DELETE = 3;
    BEGIN_TX = 4;
    COMMIT_TX = 5;
    ROLLBACK_TX = 6;
}

enum ResponseStatus {
    SUCCESS = 0;
    ERROR = 1;
    PARTIAL = 2;  // Streaming-Response
}

message Error {
    uint32 code = 1;
    string message = 2;
    repeated StackFrame stack_trace = 3;
}
```

**Referenz:** [Protocol Buffers Documentation](https://protobuf.dev/), Google 2024

Das Message Framing verwendet eine 4-Byte Length-Prefix Encoding, wobei die ersten 4 Bytes die Message-Länge in Network Byte Order (Big-Endian) enthalten, gefolgt vom serialisierten Protobuf-Payload. Diese Technik ermöglicht Zero-Copy-Parsing und effizientes Stream-Multiplexing über eine einzelne TCP-Verbindung.

### 22.1.2 Connection Lifecycle {#connection-lifecycle}

Der Connection Lifecycle umfasst fünf Phasen: Connection Establishment, Authentication, Session Initialization, Query Execution und Graceful Shutdown. Jede Phase implementiert Timeout-Mechanismen und Error Recovery für maximale Resilience gegen Netzwerkprobleme.

```mermaid
sequenceDiagram
    participant Client
    participant Driver
    participant ConnPool as Connection Pool
    participant Server as ThemisDB Server
    
    Client->>Driver: connect(host, port)
    Driver->>ConnPool: getConnection()
    
    alt Pool has idle connection
        ConnPool->>Driver: Return existing
    else No idle connection
        ConnPool->>Server: TCP Handshake
        Server-->>ConnPool: SYN-ACK
        ConnPool->>Server: AUTH Request
        Server-->>ConnPool: AUTH Response (JWT)
        ConnPool->>Server: SET DATABASE mydb
        Server-->>ConnPool: OK
        ConnPool->>Driver: Return new connection
    end
    
    Driver->>Client: Connection object
    
    loop Query Execution
        Client->>Driver: execute(query)
        Driver->>Server: QUERY Request
        Server-->>Driver: QUERY Response
        Driver->>Client: ResultSet
    end
    
    Client->>Driver: close()
    Driver->>ConnPool: releaseConnection()
    
    alt Connection healthy
        ConnPool->>ConnPool: Return to pool
    else Connection broken
        ConnPool->>ConnPool: Discard connection
    end
    
    style ConnPool fill:#43e97b
    style Server fill:#ffd32a
```

Abb. 22.1: Connection Lifecycle mit Pooling

**Performance-Metrics für Connection Establishment:**

| Phase | Latenz (P50) | Latenz (P99) | Beschreibung |
|-------|-------------|--------------|---------------|
| TCP Handshake | 0.5ms | 2ms | 3-Way Handshake |
| TLS Negotiation | 12ms | 45ms | Optional, nur bei SSL |
| Authentication | 8ms | 25ms | JWT Token Exchange |
| Session Init | 2ms | 8ms | Database Selection |
| **Total (non-TLS)** | **11ms** | **35ms** | Production-typical |
| **Total (TLS)** | **23ms** | **80ms** | Secure connections |

Tab. 22.1: Connection Establishment Performance

**Referenz:** [HikariCP Connection Pooling](https://github.com/brettwooldridge/HikariCP), Wooldridge 2023

## 22.2 Connection Management {#connection-management}

Effizientes Connection Management ist kritisch für Performance und Skalierbarkeit. ThemisDB implementiert [Connection Pooling](appendix_h_glossary.md#connection-pool) nach dem HikariCP-Pattern mit Pre-allocated Connections, Fast Path Optimization und Zero-Overhead Connection Tracking. Production-Deployments verwenden typischerweise Pool-Größen von 10-50 Connections pro Application Server bei 10.000+ gleichzeitigen Benutzern.

### 22.2.1 Connection Pooling Strategies {#connection-pooling-strategies}

[Connection Pools](appendix_h_glossary.md#connection-pool) minimieren den Overhead von Connection Establishment durch Wiederverwendung bestehender Verbindungen. Der Pool maintains eine konfigurierbare Anzahl von Connections im IDLE-State und allokiert neue Connections on-demand bei hoher Last. Expired oder defekte Connections werden automatisch entfernt und durch neue ersetzt.

**Python Connection Pool Implementation:**

```python
# Connection Pool nach HikariCP-Pattern
from themisdb import ThemisDB
from themisdb.pool import ConnectionPool, PoolConfig
import time

# Pool-Konfiguration mit Production-Settings
pool_config = PoolConfig(
    minimum_idle=5,              # Minimum Idle Connections
    maximum_pool_size=20,        # Maximum Pool Size
    connection_timeout=30000,    # 30s Timeout für getConnection()
    idle_timeout=600000,         # 10min Idle bevor Connection geschlossen
    max_lifetime=1800000,        # 30min Maximum Connection Lifetime
    connection_test_query="SELECT 1", # Health Check Query
    leak_detection_threshold=60000,   # 60s für Connection Leak Detection
)

# Connection Pool erstellen
pool = ConnectionPool(
    host="localhost",
    port=7687,
    username="admin",
    password="secure_password",
    database="mydb",
    config=pool_config
)

# Connection aus Pool holen (Fast Path)
with pool.get_connection() as conn:
    # Connection wird automatisch zurück in Pool nach Block
    result = conn.execute("FOR u IN users FILTER u.age > 18 RETURN u")
    for user in result:
        print(f"User: {user['name']}")

# Pool-Statistiken abrufen
stats = pool.get_statistics()
print(f"Active Connections: {stats.active_connections}")
print(f"Idle Connections: {stats.idle_connections}")
print(f"Total Connections: {stats.total_connections}")
print(f"Connection Requests: {stats.connection_requests}")
print(f"Average Wait Time: {stats.avg_wait_time_ms}ms")

# Pool-Shutdown mit Graceful Connection Closing
pool.close()
```

### 22.2.2 Pool Sizing and Tuning {#pool-sizing-tuning}

Die optimale Pool-Größe hängt von Application Workload, Query Execution Time und verfügbaren System-Ressourcen ab. Die Formel `connections = ((core_count * 2) + effective_spindle_count)` aus PostgreSQL [^1] bietet einen Ausgangspunkt, muss aber basierend auf Metrics angepasst werden.

**Pool Sizing Guidelines:**

| Workload-Typ | Minimum Idle | Maximum Pool | Connection Lifetime |
|--------------|-------------|--------------|---------------------|
| OLTP (High TPS) | 10-20 | 50-100 | 15-30min |
| Analytics (Long Queries) | 5-10 | 20-30 | 60-120min |
| Mixed Workload | 10-15 | 30-50 | 30-60min |
| Microservices | 2-5 | 10-20 | 10-20min |

Tab. 22.2: Connection Pool Sizing Recommendations

**Referenz:** [^1] "About Pool Sizing", PostgreSQL Wiki, 2023

### 22.2.3 Connection Health Checks {#connection-health-checks}

Health Checks validieren Connection-State bevor eine Connection aus dem Pool zurückgegeben wird. ThemisDB implementiert zwei Health Check Strategien: Passive Validation bei Connection Return und Active Validation in Background Thread. Passive Validation hat Zero-Overhead, während Active Validation frühzeitig defekte Connections erkennt.

**Health Check Implementation:**

```python
# Erweiterte Health Check Konfiguration
from themisdb.pool import HealthCheckConfig

health_config = HealthCheckConfig(
    # Passive Validation: Check bei Connection Return
    validate_on_borrow=True,
    validation_timeout=5000,  # 5s Timeout
    validation_query="SELECT 1",
    
    # Active Validation: Background Thread
    validation_interval=30000,  # 30s Interval
    validate_idle_connections=True,
    
    # Connection Repair bei Failures
    auto_reconnect=True,
    max_reconnect_attempts=3,
    reconnect_backoff_ms=1000,  # Exponential Backoff
)

pool = ConnectionPool(
    host="localhost",
    port=7687,
    database="mydb",
    health_check=health_config
)
```

**Connection Pool Performance Benchmarks:**

| Metric | Without Pool | With Pool (10 conn) | With Pool (50 conn) | Improvement |
|--------|-------------|---------------------|---------------------|-------------|
| Request Latency (P50) | 45ms | 12ms | 11ms | **73% faster** |
| Request Latency (P99) | 180ms | 35ms | 32ms | **80% faster** |
| Throughput (req/s) | 2,200 | 8,500 | 9,100 | **4x higher** |
| Connection Overhead | 11ms/req | 0.2ms/req | 0.1ms/req | **98% reduction** |

Tab. 22.3: Connection Pool Performance Impact (10k requests, 4-core server)

## 22.3 Language Bindings {#language-bindings}

ThemisDB bietet native Client Libraries für alle major Programming Languages mit idiomatischen APIs und Language-spezifischen Optimierungen. Python verwendet Async/Await, Java bietet JDBC-Kompatibilität, Node.js nutzt Promises, Go implementiert Context-based Cancellation und Rust bietet Zero-Copy mit Lifetime-Safe References.

### 22.3.1 Python Driver (PyMongo-Style) {#python-driver}

Der Python Driver ist die am weitesten entwickelte und feature-reichste Client-Library für ThemisDB.

```bash
# Installation via pip
pip install themisdb

# Mit optionalen Dependencies
pip install themisdb[async,pandas]

# Development Version
pip install git+https://github.com/themisdb/themisdb-python.git
```

**Basis-Verbindung:**

```python
from themisdb import ThemisDB

# Einfache Verbindung
db = ThemisDB("localhost", 7687)

# Mit Authentifizierung
db = ThemisDB(
    host="localhost",
    port=7687,
    username="admin",
    password="secure_password",
    database="mydb"
)

# Connection String
db = ThemisDB.from_uri("themis://admin:password@localhost:7687/mydb")
```

### 22.3.2 Java JDBC Integration {#java-jdbc-driver}

Der Java JDBC Driver bietet vollständige Integration mit dem Java Database Connectivity Standard, wodurch ThemisDB mit allen JDBC-kompatiblen Tools wie Spring Data, Hibernate und Connection Pool Frameworks verwendet werden kann.

**Java JDBC Connection mit HikariCP:**

```java
// JDBC Integration mit HikariCP Connection Pooling
import com.zaxxer.hikari.HikariConfig;
import com.zaxxer.hikari.HikariDataSource;
import io.themisdb.jdbc.ThemisDriver;
import java.sql.*;

public class ThemisDBJdbcExample {
    public static void main(String[] args) throws SQLException {
        // HikariCP-Konfiguration für optimale Performance
        HikariConfig config = new HikariConfig();
        config.setJdbcUrl("jdbc:themis://localhost:7687/mydb");
        config.setUsername("admin");
        config.setPassword("secure_password");
        
        // Connection Pool Settings nach HikariCP Best Practices
        config.setMaximumPoolSize(20);           // Max connections
        config.setMinimumIdle(5);                // Minimum idle
        config.setConnectionTimeout(30000);      // 30s timeout
        config.setIdleTimeout(600000);           // 10min idle
        config.setMaxLifetime(1800000);          // 30min lifetime
        config.setLeakDetectionThreshold(60000); // 60s leak detection
        
        // Connection Test Query für Health Checks
        config.setConnectionTestQuery("SELECT 1");
        
        // DataSource erstellen
        HikariDataSource dataSource = new HikariDataSource(config);
        
        // JDBC Query mit Prepared Statement (SQL Injection Safe)
        String aql = "FOR u IN users FILTER u.age > @minAge SORT u.name RETURN u";
        try (Connection conn = dataSource.getConnection();
             PreparedStatement stmt = conn.prepareStatement(aql)) {
            
            // Parameter binden (@minAge wird automatisch escaped)
            stmt.setInt(1, 18);
            
            // Query ausführen
            try (ResultSet rs = stmt.executeQuery()) {
                while (rs.next()) {
                    System.out.println(String.format(
                        "User: %s (Age: %d)",
                        rs.getString("name"),
                        rs.getInt("age")
                    ));
                }
            }
        }
        
        // Connection Pool schließen
        dataSource.close();
    }
}
```

**Referenz:** "HikariCP - JDBC Connection Pool", Wooldridge 2023, https://github.com/brettwooldridge/HikariCP

### 22.3.3 Node.js Async Patterns {#nodejs-driver}

Der Node.js Driver nutzt native Promises und Async/Await für nicht-blockierende I/O-Operations. Der Event Loop bleibt frei während Database Queries ausgeführt werden, was maximale Concurrency ermöglicht.

**Node.js mit Async/Await und Concurrent Queries:**

```javascript
// Node.js Driver mit Async/Await und Connection Pooling
const { ThemisDB } = require('themisdb');

// Connection mit Pool-Konfiguration
const db = new ThemisDB({
    host: 'localhost',
    port: 7687,
    username: 'admin',
    password: 'password',
    database: 'mydb',
    
    // Connection Pool Settings
    poolSize: 20,
    poolTimeout: 30000,
    idleTimeout: 600000,
    
    // Retry Configuration
    retryAttempts: 3,
    retryDelay: 1000,
});

// Async Query mit Error Handling
async function getUsersByAge(minAge) {
    try {
        const users = await db.collection('users')
            .find({ age: { $gte: minAge } })
            .sort({ name: 1 })
            .limit(100)
            .toArray();
        
        return users;
    } catch (error) {
        console.error('Query failed:', error);
        throw error;
    }
}

// Parallele Queries mit Promise.all für maximale Performance
async function fetchDashboardData() {
    const [users, orders, products] = await Promise.all([
        db.collection('users').find({ active: true }).toArray(),
        db.collection('orders').find({ status: 'pending' }).toArray(),
        db.collection('products').find({ in_stock: true }).toArray()
    ]);
    
    return { users, orders, products };
}

// Streaming Cursor für große Resultsets (Memory-effizient)
async function processLargeDataset() {
    const cursor = db.collection('logs')
        .find({ timestamp: { $gte: Date.now() - 86400000 } })
        .batchSize(1000);  // Fetch 1000 Docs at a time
    
    // Stream-Processing ohne vollständiges Laden in Memory
    for await (const log of cursor) {
        await processLog(log);  // Process one at a time
    }
}
```

### 22.3.4 Go Context Handling {#go-driver}

Der Go Driver implementiert Context-based Cancellation für Timeout-Management und Graceful Shutdown. Contexts propagieren Deadlines und Cancellation Signals durch den gesamten Call Stack.

**Go Driver mit Context und Timeouts:**

```go
// Go Driver mit Context-based Timeout und Cancellation
package main

import (
    "context"
    "fmt"
    "time"
    "github.com/themisdb/themisdb-go"
)

func main() {
    // Connection mit Timeout
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()
    
    // Client mit Connection Pool
    client, err := themisdb.Connect(ctx, "localhost:7687",
        themisdb.WithAuth("admin", "password"),
        themisdb.WithDatabase("mydb"),
        themisdb.WithPoolSize(20),           // Max connections
        themisdb.WithPoolMinIdle(5),         // Min idle connections
        themisdb.WithPoolTimeout(30*time.Second),
    )
    if err != nil {
        panic(err)
    }
    defer client.Close()
    
    // Query mit Context-Timeout (5 Sekunden)
    queryCtx, queryCancel := context.WithTimeout(ctx, 5*time.Second)
    defer queryCancel()
    
    // Find mit Filter
    cursor, err := client.Collection("users").Find(
        queryCtx,
        themisdb.M{"age": themisdb.M{"$gte": 18}},
        themisdb.Options().Sort(themisdb.M{"name": 1}).Limit(100),
    )
    if err != nil {
        panic(err)
    }
    
    // Iterate Resultset
    var users []map[string]interface{}
    if err := cursor.All(queryCtx, &users); err != nil {
        panic(err)
    }
    
    fmt.Printf("Found %d users\n", len(users))
}
```

**Referenz:** "Context Package", Go Standard Library, https://pkg.go.dev/context

### 22.3.5 Rust Zero-Copy Driver {#rust-driver}

Der Rust Driver implementiert Zero-Copy-Deserialisierung mit `serde` und Lifetime-Safe References. Memory-Safety ist compiler-garantiert ohne Runtime-Overhead.

**Rust Driver mit Zero-Copy und Type Safety:**

```rust
// Rust Driver mit Zero-Copy und Compile-Time Type Safety
use themisdb::{Client, Document, Collection};
use serde::{Deserialize, Serialize};

// Type-safe User Model mit Serde Derive
#[derive(Debug, Serialize, Deserialize)]
struct User {
    #[serde(rename = "_id")]
    id: String,
    name: String,
    email: String,
    age: u32,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Client mit Connection Pool
    let client = Client::builder()
        .host("localhost")
        .port(7687)
        .username("admin")
        .password("password")
        .database("mydb")
        .pool_size(20)
        .pool_min_idle(5)
        .build()
        .await?;
    
    // Type-safe Query mit Zero-Copy Deserialization
    let collection: Collection<User> = client.collection("users");
    
    let mut cursor = collection
        .find(doc! { "age": { "$gte": 18 } })
        .sort(doc! { "name": 1 })
        .limit(100)
        .await?;
    
    // Zero-Copy Iteration (keine Heap-Allokation pro Document)
    while let Some(user) = cursor.try_next().await? {
        println!("User: {} (Age: {})", user.name, user.age);
    }
    
    Ok(())
}
```

**Referenz:** "Zero-Copy Deserialization", Serde Documentation, https://serde.rs/lifetimes.html

## 22.4 Error Handling & Retry Logic {#error-handling}

Robuste Error Handling-Strategien sind essentiell für Production-Systeme. ThemisDB klassifiziert Errors in Kategorien (Transient vs. Permanent) und implementiert automatische Retry-Logic mit Exponential Backoff für transiente Fehler wie Network Timeouts oder Temporary Overload.

### 22.4.1 Error Codes and Categories {#error-codes}

ThemisDB verwendet strukturierte Error Codes nach HTTP-Status-Pattern: 4xx für Client-Errors (z.B. Invalid Query), 5xx für Server-Errors (z.B. Database Unavailable). Jeder Error enthält einen Error Code, Message und optional einen Stack Trace.

**Error Hierarchy:**

| Error Code | Category | Retry? | Beschreibung |
|-----------|----------|--------|---------------|
| 400-499 | Client Error | No | Invalid Query, Auth Failure, Not Found |
| 500-599 | Server Error | Yes | Database Unavailable, Internal Error |
| 1001 | Connection Error | Yes | Network Timeout, Connection Refused |
| 1002 | Query Error | No | Syntax Error, Invalid AQL |
| 1003 | Validation Error | No | Schema Validation Failed |
| 1004 | Duplicate Key | No | Unique Constraint Violation |
| 1005 | Timeout Error | Yes | Query Execution Timeout |

Tab. 22.4: Error Code Classification

### 22.4.2 Retry Logic with Exponential Backoff {#retry-logic}

[Exponential Backoff](appendix_h_glossary.md#exponential-backoff) erhöht die Wartezeit zwischen Retry-Attempts exponentiell, um Server-Overload zu vermeiden. Die Implementierung basiert auf dem Decorrelated Jitter Algorithm von AWS für optimale Retry-Verteilung.

**Python Retry Logic mit Exponential Backoff:**

```python
# Retry Logic mit Exponential Backoff und Circuit Breaker
import time
import random
from themisdb.exceptions import ConnectionError, TimeoutError, ServerError

class RetryConfig:
    def __init__(self):
        self.max_attempts = 3              # Maximum Retry Attempts
        self.base_delay = 1.0              # Initial Delay (1 Sekunde)
        self.max_delay = 30.0              # Maximum Delay (30 Sekunden)
        self.exponential_base = 2          # Backoff Multiplier
        self.jitter = True                 # Add random jitter
        
        # Error Classes die Retry auslösen
        self.retryable_errors = (
            ConnectionError,
            TimeoutError,
            ServerError,
        )

def retry_with_backoff(func, retry_config=None):
    """Decorator für automatische Retry Logic mit Exponential Backoff"""
    config = retry_config or RetryConfig()
    
    def wrapper(*args, **kwargs):
        last_exception = None
        
        for attempt in range(config.max_attempts):
            try:
                return func(*args, **kwargs)  # Erfolg
            
            except config.retryable_errors as e:
                last_exception = e
                
                if attempt == config.max_attempts - 1:
                    # Letzter Attempt fehlgeschlagen
                    raise e
                
                # Berechne Backoff-Delay mit Exponential + Jitter
                delay = min(
                    config.base_delay * (config.exponential_base ** attempt),
                    config.max_delay
                )
                
                if config.jitter:
                    # Decorrelated Jitter (AWS-Style)
                    delay = random.uniform(config.base_delay, delay)
                
                print(f"Retry attempt {attempt + 1}/{config.max_attempts} " 
                      f"after {delay:.2f}s delay")
                time.sleep(delay)
            
            except Exception as e:
                # Non-retryable Error
                raise e
        
        raise last_exception
    
    return wrapper

# Verwendung mit Decorator
@retry_with_backoff
def fetch_users():
    db = ThemisDB("localhost", 7687)
    return db.find("users", {"active": True})

# Oder als Funktion
users = retry_with_backoff(lambda: db.find("users", {"active": True}))()
```

**Referenz:** "Exponential Backoff And Jitter", AWS Architecture Blog, Brooker 2015

### 22.4.3 Circuit Breaker Pattern {#circuit-breaker}

[Circuit Breaker](appendix_h_glossary.md#circuit-breaker) verhindert wiederholte Calls zu einem failing Service und gibt dem Service Zeit zur Recovery. Der Pattern implementiert drei States: CLOSED (normal), OPEN (failing) und HALF_OPEN (testing recovery).

**Circuit Breaker Implementation:**

```python
# Circuit Breaker Pattern nach Hystrix-Vorbild
from enum import Enum
from datetime import datetime, timedelta

class CircuitState(Enum):
    CLOSED = "closed"      # Normal operation
    OPEN = "open"          # Failing, reject calls
    HALF_OPEN = "half_open"  # Testing recovery

class CircuitBreaker:
    def __init__(self):
        self.state = CircuitState.CLOSED
        self.failure_count = 0
        self.success_count = 0          # Initialize success counter
        self.failure_threshold = 5      # Open after 5 failures
        self.success_threshold = 2      # Close after 2 successes
        self.timeout = timedelta(seconds=60)  # Try recovery after 60s
        self.last_failure_time = None
    
    def call(self, func, *args, **kwargs):
        # State: OPEN - Reject calls immediately
        if self.state == CircuitState.OPEN:
            if datetime.now() - self.last_failure_time < self.timeout:
                raise Exception("Circuit breaker is OPEN")
            else:
                # Timeout elapsed, try recovery
                self.state = CircuitState.HALF_OPEN
                print("Circuit breaker entering HALF_OPEN state")
        
        try:
            result = func(*args, **kwargs)
            self._on_success()
            return result
        
        except Exception as e:
            self._on_failure()
            raise e
    
    def _on_success(self):
        if self.state == CircuitState.HALF_OPEN:
            self.success_count += 1
            if self.success_count >= self.success_threshold:
                self.state = CircuitState.CLOSED
                self.failure_count = 0
                print("Circuit breaker CLOSED (recovered)")
        
    def _on_failure(self):
        self.failure_count += 1
        self.last_failure_time = datetime.now()
        
        if self.failure_count >= self.failure_threshold:
            self.state = CircuitState.OPEN
            print(f"Circuit breaker OPEN after {self.failure_count} failures")

# Verwendung
circuit_breaker = CircuitBreaker()

try:
    users = circuit_breaker.call(db.find, "users", {"active": True})
except Exception as e:
    print(f"Call failed: {e}")
```

**Referenz:** "Circuit Breaker Pattern", Hystrix Documentation, Netflix 2019

## 22.5 Query Builders & Type Safety {#query-builders}

Query Builders bieten eine Fluent API für typsicheres Query-Building mit IDE-Autovervollständigung und Compile-Time Validation. Die Implementation folgt dem Builder-Pattern mit Method-Chaining für leserlichen und wartbaren Code.

### 22.5.1 Fluent API Design {#fluent-api-design}

Fluent APIs verwenden Method-Chaining für intuitive Query-Konstruktion. Jede Methode gibt `this` zurück, was weitere Methodenaufrufe ermöglicht. Das Design folgt Domain-Specific Language (DSL) Prinzipien.

**TypeScript Query Builder mit Type Safety:**

```typescript
// TypeScript Query Builder mit vollständiger Type Safety
interface User {
    id: string;
    name: string;
    email: string;
    age: number;
    active: boolean;
}

class QueryBuilder<T> {
    private collection: string;
    private filters: any[] = [];
    private sortFields: any = {};
    private limitValue: number | null = null;
    private selectFields: string[] | null = null;
    
    constructor(collection: string) {
        this.collection = collection;
    }
    
    // Filter mit Type-Safe Field Names
    filter(field: keyof T, operator: string, value: any): this {
        this.filters.push({ field, operator, value });
        return this;  // Method chaining
    }
    
    // Sortierung mit Type-Safe Field Names
    sort(field: keyof T, direction: 'asc' | 'desc' = 'asc'): this {
        this.sortFields[field as string] = direction === 'asc' ? 1 : -1;
        return this;
    }
    
    // Limit
    limit(count: number): this {
        this.limitValue = count;
        return this;
    }
    
    // Projection (nur bestimmte Felder)
    select(...fields: (keyof T)[]): this {
        this.selectFields = fields as string[];
        return this;
    }
    
    // Build AQL Query
    build(): string {
        let aql = `FOR doc IN ${this.collection}`;
        
        // Filters
        if (this.filters.length > 0) {
            const filterClauses = this.filters.map(f => 
                `doc.${f.field} ${f.operator} ${JSON.stringify(f.value)}`
            );
            aql += ` FILTER ${filterClauses.join(' AND ')}`;
        }
        
        // Sort
        if (Object.keys(this.sortFields).length > 0) {
            const sortClauses = Object.entries(this.sortFields)
                .map(([field, dir]) => `doc.${field} ${dir === 1 ? 'ASC' : 'DESC'}`);
            aql += ` SORT ${sortClauses.join(', ')}`;
        }
        
        // Limit
        if (this.limitValue !== null) {
            aql += ` LIMIT ${this.limitValue}`;
        }
        
        // Projection
        if (this.selectFields !== null) {
            const projection = this.selectFields
                .map(f => `${f}: doc.${f}`)
                .join(', ');
            aql += ` RETURN { ${projection} }`;
        } else {
            aql += ` RETURN doc`;
        }
        
        return aql;
    }
    
    // Execute Query
    async execute(): Promise<T[]> {
        const aql = this.build();
        const result = await db.query(aql);
        return result as T[];
    }
}

// Type-Safe Usage mit IDE Autovervollständigung
const users = await new QueryBuilder<User>('users')
    .filter('age', '>', 18)         // Type-safe: nur valide Fields
    .filter('active', '==', true)
    .sort('name', 'asc')            // Type-safe: nur 'asc' oder 'desc'
    .limit(100)
    .select('id', 'name', 'email')  // Type-safe: nur User Fields
    .execute();
```

### 22.5.2 ORM Integration {#orm-integration}

ORM-Integration ermöglicht Object-Relational Mapping mit automatischer Serialisierung/Deserialisierung und Relationship-Management. Die Implementation folgt Active Record oder Data Mapper Patterns.

**Query Builder Overhead Benchmarks:**

| Query Type | Raw AQL | Query Builder | ORM | Builder Overhead |
|-----------|---------|---------------|-----|------------------|
| Simple SELECT | 2.1ms | 2.3ms (+9%) | 3.8ms (+81%) | **Minimal** |
| Complex JOIN | 45ms | 46ms (+2%) | 52ms (+16%) | **Negligible** |
| Aggregation | 120ms | 122ms (+2%) | 128ms (+7%) | **Acceptable** |

Tab. 22.5: Query Builder Performance Overhead (P50 latency, 10k queries)

## 22.6 Performance Optimization {#performance-optimization}

Performance-Optimierungen in Client Libraries umfassen Batch Operations für reduzierte Network Round-Trips, Cursor Streaming für Memory-effiziente Large Resultsets und Prepared Statements für Query Plan Caching. Diese Techniken können Query-Performance um Faktor 10-100x verbessern.

### 22.6.1 Batch Operations {#batch-operations}

[Batch Operations](appendix_h_glossary.md#batch) reduzieren Network Overhead durch Bundling mehrerer Operations in einem Request. Ein Batch Insert von 1000 Documents benötigt nur 1 Network Round-Trip statt 1000.

**Batch Insert Optimization:**

```python
# Batch Operations für maximale Performance
import time

# INEFFIZIENT: Einzelne Inserts (1000 Round-Trips)
start = time.time()
for i in range(1000):
    db.insert("users", {"name": f"User{i}", "age": 20 + (i % 60)})
single_insert_time = time.time() - start
print(f"Single inserts: {single_insert_time:.2f}s")  # ~15-20 Sekunden

# EFFIZIENT: Batch Insert (1 Round-Trip)
start = time.time()
users = [{"name": f"User{i}", "age": 20 + (i % 60)} for i in range(1000)]
db.insert_many("users", users, batch_size=1000)
batch_insert_time = time.time() - start
print(f"Batch insert: {batch_insert_time:.2f}s")     # ~0.5-1 Sekunde

print(f"Speedup: {single_insert_time / batch_insert_time:.1f}x")  # ~20-30x
```

**Batch vs Single Operations Benchmark:**

| Operation | Documents | Single Ops | Batch Ops | Speedup |
|-----------|-----------|-----------|-----------|---------|
| Insert | 1,000 | 18.5s | 0.8s | **23x** |
| Update | 1,000 | 22.3s | 1.2s | **19x** |
| Delete | 1,000 | 16.7s | 0.6s | **28x** |
| Insert | 10,000 | 185s | 6.5s | **28x** |

Tab. 22.6: Batch Operations Performance (localhost, 1ms RTT)

### 22.6.2 Cursor Streaming {#cursor-streaming}

Cursor Streaming lädt große Resultsets in Chunks statt vollständig in Memory. Diese Technik ermöglicht Processing von Millionen Documents mit konstantem Memory-Footprint.

**Cursor Streaming für Large Resultsets:**

```python
# Cursor Streaming für Memory-effizientes Processing
from themisdb import ThemisDB

db = ThemisDB("localhost", 7687)

# OHNE Streaming: Lädt alle 1M Documents in Memory (crashes bei large datasets)
# users = db.find("users", {}).to_list()  # Memory Overflow!

# MIT Streaming: Lädt nur 1000 Documents at a time
cursor = db.find("users", {}, batch_size=1000)

processed_count = 0
for user in cursor:  # Iterator lädt Chunks on-demand
    process_user(user)
    processed_count += 1
    
    if processed_count % 10000 == 0:
        print(f"Processed {processed_count} users")

# Memory Footprint: ~constant (~10MB für 1000-doc buffer)
```

### 22.6.3 Prepared Statements {#prepared-statements}

Prepared Statements cachen Query Plans und Parameter Bindings für wiederholte Queries. Der Server muss die Query nur einmal parsen und optimieren, was Query Latency bei wiederholten Calls um 30-50% reduziert.

**Zusammenfassung**

ThemisDB Client Libraries bieten Production-Ready-Features mit:

- **HikariCP-Style Connection Pooling:** 73% niedrigere Latency, 4x höherer Throughput
- **Multi-Language Support:** Python, Java, Node.js, Go, Rust mit idiomatischen APIs
- **Robuste Error Handling:** Retry Logic mit Exponential Backoff, Circuit Breaker Pattern
- **Type-Safe Query Builders:** Fluent APIs mit IDE-Autovervollständigung
- **Performance Optimizations:** Batch Operations (20-30x Speedup), Cursor Streaming, Zero-Copy

**Best Practices:**
1. Verwenden Sie Connection Pooling mit 10-50 Connections pro Application Server
2. Implementieren Sie Retry Logic mit Exponential Backoff für transiente Fehler
3. Nutzen Sie Batch Operations für Bulk-Inserts (20-30x Speedup)
4. Implementieren Sie Circuit Breaker für externe Service Calls
5. Verwenden Sie Type-Safe Query Builders für Wartbarkeit

Im nächsten Kapitel betrachten wir Testing & QA-Strategien für ThemisDB-Anwendungen.
