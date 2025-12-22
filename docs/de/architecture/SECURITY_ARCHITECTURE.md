# ThemisDB Server Architecture - Security & Performance Design

**Version**: v1.3.0  
**Date**: 4. Dezember 2025  
**Status**: Production Architecture  
**Kategorie**: 🧩 Architecture

---

## 📑 Inhaltsverzeichnis

- [Executive Summary](#executive-summary)
- [Architektur-Übersicht](#architektur-übersicht)
- [Warum KEINE separaten Prozesse?](#warum-keine-separaten-prozesse)
- [Isolation & Sicherheit](#isolation--sicherheit)

---

## Executive Summary

ThemisDB nutzt eine **Multi-Threaded, Process-Isolated Server-Architektur** mit getrennten Thread Pools für HTTP und Wire Protocol, um maximale Sicherheit und Performance zu gewährleisten.

---

## Architektur-Übersicht

```
┌──────────────────────────────────────────────────────────────────┐
│                    themis_server.exe (1 Prozess)                  │
├──────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌─────────────────────────────┐  ┌──────────────────────────┐  │
│  │   HTTP REST Server          │  │  Wire Protocol Server     │  │
│  │   Port: 8765                │  │  Port: 8766               │  │
│  ├─────────────────────────────┤  ├──────────────────────────┤  │
│  │ Dedicated IO Context        │  │ Dedicated IO Context      │  │
│  │ Thread Pool: 8-16 threads   │  │ Thread Pool: 4-8 threads  │  │
│  │ Max Connections: 10k        │  │ Max Connections: 1k       │  │
│  │ Rate Limit: 10k req/min     │  │ Rate Limit: 1k req/sec    │  │
│  └─────────────────────────────┘  └──────────────────────────┘  │
│           │                                  │                    │
│           └──────────────┬───────────────────┘                    │
│                          ▼                                        │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │          Shared Core Components (Thread-Safe)              │  │
│  ├────────────────────────────────────────────────────────────┤  │
│  │  • RocksDB Storage Engine (concurrent_hash_map)            │  │
│  │  • Secondary Indexes (TBB concurrent structures)           │  │
│  │  • Vector Index HNSW (read-optimized locks)                │  │
│  │  • Graph Index (concurrent adjacency lists)                │  │
│  │  • Transaction Manager (MVCC, lock-free where possible)    │  │
│  └────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────┘
```

---

## Warum KEINE separaten Prozesse?

### ❌ Multi-Process Nachteile:

1. **Shared Memory Overhead**:
   - IPC (Inter-Process Communication) notwendig
   - RocksDB kann nicht direkt geshared werden
   - 2x Memory für Block Cache (HTTP Prozess + Wire Prozess)
   - Cache Coherence Problem

2. **Performance**:
   - IPC Latenz: ~100-500µs pro Request
   - Shared Memory Locks langsam
   - Context Switches zwischen Prozessen teurer

3. **Complexity**:
   - Process Management (supervisord, systemd)
   - Crash Recovery komplexer
   - Deployment komplizierter

### ✅ Multi-Threaded Vorteile:

1. **Zero-Copy Shared State**:
   - Beide Server greifen direkt auf RocksDB zu
   - Shared Block Cache (256MB-2GB)
   - Keine IPC Overhead

2. **Performance**:
   - Direct Memory Access
   - Cache-freundlich
   - Weniger Context Switches

3. **Einfachheit**:
   - Ein Binary
   - Ein Log File
   - Einfaches Deployment

---

## Isolation & Sicherheit

### 1. Thread Pool Isolation ✅

**HTTP Server**:
```cpp
// Eigener IO Context
net::io_context http_ioc_;
std::vector<std::thread> http_threads_;  // 8-16 threads

// Läuft komplett getrennt
http_ioc_.run();  // Blockiert nie Wire Protocol
```

**Wire Protocol Server**:
```cpp
// SEPARATER IO Context
std::unique_ptr<net::io_context> wire_io_context_;
std::vector<std::thread> wire_io_threads_;  // 4-8 threads

// Separate worker threads
std::unique_ptr<boost::asio::thread_pool> wire_worker_pool_;

wire_io_context_->run();  // Unabhängig vom HTTP Server
```

**Ergebnis**: 
- HTTP Request blockiert NIEMALS Wire Protocol
- Wire Protocol Request blockiert NIEMALS HTTP
- Bei DDoS auf Port 8765 läuft Port 8766 weiter!

---

### 2. Connection Limits ✅

**Per-IP Connection Limits**:
```cpp
struct Config {
    uint32_t max_connections = 1000;           // Global
    uint32_t max_connections_per_ip = 10;     // Per IP
};

// Tracking
std::unordered_map<std::string, uint32_t> connections_per_ip_;

bool checkConnectionLimit(const std::string& remote_ip) {
    std::lock_guard lock(connections_mutex_);
    if (connections_per_ip_[remote_ip] >= config_.max_connections_per_ip) {
        stats_.rejected_connections++;
        return false;  // Reject!
    }
    return true;
}
```

**Schutz gegen**:
- Connection Exhaustion Attacks
- Slowloris Attacks
- Fork Bomb ähnliche Szenarien

---

### 3. Rate Limiting ✅

**Per-IP Request Limits**:
```cpp
struct RateLimitState {
    uint64_t window_start_ms = 0;
    uint32_t request_count_second = 0;   // Rolling 1-second window
    uint32_t request_count_minute = 0;   // Rolling 1-minute window
};

std::unordered_map<std::string, RateLimitState> rate_limits_;

bool checkRateLimit(const std::string& remote_ip) {
    auto now_ms = getCurrentTimeMs();
    auto& state = rate_limits_[remote_ip];
    
    // Reset windows if expired
    if (now_ms - state.window_start_ms > 1000) {
        state.request_count_second = 0;
        state.window_start_ms = now_ms;
    }
    
    // Check limits
    if (state.request_count_second >= config_.max_requests_per_second) {
        return false;  // Rate limited!
    }
    
    state.request_count_second++;
    state.request_count_minute++;
    return true;
}
```

**Schutz gegen**:
- Request Flooding
- Credential Stuffing
- Brute Force Attacks

---

### 4. Request Size Limits ✅

```cpp
struct Config {
    uint32_t max_frame_size_mb = 64;  // Wire Protocol
    size_t max_request_size_mb = 10;  // HTTP
};

// Validate before allocating buffer
if (payload_size > config_.max_frame_size_mb * 1024 * 1024) {
    sendError(ERROR_FRAME_TOO_LARGE, "Frame exceeds 64MB limit");
    close();
    return;
}

// Safe allocation
payload_buffer_.resize(payload_size);
```

**Schutz gegen**:
- Memory Exhaustion
- OOM Attacks
- Buffer Overflow

---

### 5. Timeout Protection ✅

```cpp
struct Config {
    uint32_t connection_timeout_sec = 300;   // 5 minutes idle
    uint32_t auth_timeout_sec = 10;          // Must auth within 10s
    uint32_t request_timeout_sec = 30;       // Request processing
};

void startTimeout(std::chrono::seconds timeout) {
    timeout_timer_ = std::make_unique<net::steady_timer>(*io_context_);
    timeout_timer_->expires_after(timeout);
    timeout_timer_->async_wait([self = shared_from_this()](auto ec) {
        if (!ec) {
            THEMIS_WARN("Session {} timeout - closing", self->session_id_);
            self->close();
        }
    });
}
```

**Schutz gegen**:
- Slowloris (langsame HTTP Requests)
- Slow Read Attacks
- Hung Connections

---

### 6. Authentication Required ✅

**Wire Protocol**:
```cpp
// State machine
enum class SessionState {
    UNAUTHENTICATED,
    HELLO_RECEIVED,
    AUTHENTICATED
};

void handleMessage() {
    if (state_ != SessionState::AUTHENTICATED && 
        opcode != OpCode::HELLO && 
        opcode != OpCode::AUTH_REQUEST) {
        sendError(ERROR_NOT_AUTHENTICATED, "Authentication required");
        return;
    }
    
    // Process message...
}
```

**HTTP Server**:
```cpp
// Middleware
if (requiresAuth(request.target()) && !isAuthenticated(request)) {
    return makeError(401, "Unauthorized");
}
```

**Mechanismen**:
- SCRAM-SHA-256 (Wire Protocol)
- JWT/OAuth2 (HTTP REST)
- Optional: mTLS (Client Certificates)

---

### 7. TLS/mTLS Support ✅

```cpp
struct Config {
    bool enable_tls = false;
    std::string tls_cert_path = "/etc/themis/server.crt";
    std::string tls_key_path = "/etc/themis/server.key";
    std::string tls_ca_cert_path = "/etc/themis/ca.crt";
    bool tls_require_client_cert = false;  // mTLS
};

// SSL Context
boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv13);
ssl_ctx.load_cert_chain(config.tls_cert_path, config.tls_key_path);

if (config.tls_require_client_cert) {
    ssl_ctx.load_verify_file(config.tls_ca_cert_path);
    ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer | 
                           boost::asio::ssl::verify_fail_if_no_peer_cert);
}
```

**Schutz gegen**:
- Man-in-the-Middle
- Eavesdropping
- Packet Sniffing

---

## Performance Optimierungen

### 1. Lock-Free Data Structures

```cpp
// TBB concurrent_hash_map (2.4x faster als std::unordered_map + mutex)
tbb::concurrent_hash_map<std::string, TenantConfig> tenants_;

// Atomic counters (keine Locks)
std::atomic<uint64_t> request_count_{0};
std::atomic<bool> running_{false};
```

### 2. Zero-Copy I/O

```cpp
// Boost.Beast scatter-gather I/O
boost::asio::async_write(
    socket_,
    std::vector<boost::asio::const_buffer>{
        boost::asio::buffer(&header, sizeof(header)),
        boost::asio::buffer(payload),
        boost::asio::buffer(&checksum, sizeof(checksum))
    },
    // ...
);
```

### 3. Connection Pooling

Clients halten persistente Verbindungen:
```python
# Python Client - Connection Pool
with ThemisDBConnectionPool(max_size=10) as pool:
    conn = pool.acquire()
    result = conn.get("key")
    pool.release(conn)  # Reuse!
```

### 4. Thread Affinity (Optional)

```cpp
// Pin I/O threads to specific CPU cores (NUMA-aware)
#ifdef __linux__
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(core_id, &cpuset);
pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
#endif
```

---

## Monitoring & Observability

### Metrics Export

```cpp
struct Stats {
    // Connection metrics
    uint64_t total_connections = 0;
    uint64_t active_connections = 0;
    uint64_t rejected_connections = 0;
    
    // Request metrics
    uint64_t total_requests = 0;
    uint64_t total_errors = 0;
    uint64_t auth_failures = 0;
    
    // Bandwidth
    uint64_t bytes_received = 0;
    uint64_t bytes_sent = 0;
};

// Prometheus format
GET /metrics
themis_wire_active_connections 42
themis_wire_total_requests_total 1234567
themis_wire_auth_failures_total 12
themis_wire_bytes_received_total 98765432
```

### Logging

```cpp
THEMIS_INFO("Wire Protocol: Accepted connection from {} (session_id={})", 
            remote_ip, session_id);
THEMIS_WARN("Wire Protocol: Rate limit exceeded for IP {}", remote_ip);
THEMIS_ERROR("Wire Protocol: Authentication failed for user {} from {}", 
             username, remote_ip);
```

---

## Attack Surface Mitigation

### 1. DDoS Protection

✅ Connection limits per IP  
✅ Rate limiting (per-second + per-minute)  
✅ Request size limits  
✅ Timeout on all operations  
✅ Automatic connection cleanup  

### 2. Authentication Bypass

✅ State machine enforcement  
✅ Authentication required for all ops (except HELLO)  
✅ Timeout on auth handshake (10s)  
✅ Audit logging of auth failures  

### 3. Memory Exhaustion

✅ Max frame size: 64MB  
✅ Max connections: 1000  
✅ Payload validation before allocation  
✅ Automatic buffer cleanup on disconnect  

### 4. Data Exfiltration

✅ TLS encryption (optional)  
✅ mTLS client verification (optional)  
✅ Authorization checks on all operations  
✅ Audit logging of all data access  

---

## Deployment Best Practices

### 1. Firewall Rules

```bash
# HTTP REST (public)
iptables -A INPUT -p tcp --dport 8765 -j ACCEPT

# Wire Protocol (internal only!)
iptables -A INPUT -p tcp --dport 8766 -s 10.0.0.0/8 -j ACCEPT
iptables -A INPUT -p tcp --dport 8766 -j DROP
```

### 2. Resource Limits

```yaml
# systemd service file
[Service]
LimitNOFILE=65536          # Max file descriptors
LimitNPROC=4096            # Max processes/threads
MemoryMax=32G              # Max memory
CPUQuota=800%              # Max 8 cores
```

### 3. TLS Configuration

```yaml
# config.yaml
wire_protocol:
  enable_tls: true
  tls_cert_path: /etc/themis/certs/server.crt
  tls_key_path: /etc/themis/certs/server.key
  tls_ca_cert_path: /etc/themis/certs/ca.crt
  tls_require_client_cert: true  # Enforce mTLS
  tls_min_version: "TLSv1.3"
```

### 4. Monitoring

```bash
# Prometheus scraping
curl http://localhost:9090/metrics | grep themis_wire

# Connection count alert
expr: themis_wire_active_connections > 900
alert: WireProtocolHighLoad

# Auth failure alert
expr: rate(themis_wire_auth_failures_total[5m]) > 10
alert: WireProtocolBruteForce
```

---

## Fazit

**✅ Sicherheit**: 
- Multi-Layer Defense (Limits + Auth + TLS + Rate Limiting)
- Attack Surface minimal
- Defense-in-Depth Prinzip

**✅ Performance**: 
- Thread Pool Isolation verhindert gegenseitige Blockierung
- Lock-Free Strukturen wo möglich
- Zero-Copy I/O
- Shared State ohne IPC Overhead

**✅ Best of Both Worlds**: 
- Single-Process Simplicity
- Multi-Threaded Isolation
- Production-Ready Security

---

**Nächste Schritte**:
1. Wire Protocol Server implementieren (src/network/wire_protocol_server.cpp)
2. Integration Tests (Security + Performance)
3. Penetration Testing
4. Load Testing (1M concurrent connections)
5. Production Deployment
