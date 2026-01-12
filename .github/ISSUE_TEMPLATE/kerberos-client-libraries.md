---
name: Create Kerberos Client Library Support (C++/Python)
about: Implement client libraries with native Kerberos authentication support
title: 'Create Kerberos Client Library Support (C++/Python)'
labels: type:enhancement, area:security, area:api, priority:P2, effort:large
assignees: ''
---

## 📋 Summary

Implement client libraries for C++ and Python with native Kerberos/GSSAPI authentication support, enabling seamless SSO integration for ThemisDB clients.

**Parent Feature:** Issue #[parent-issue-number] - Kerberos/GSSAPI Authentication Support

## 🔍 Problem Statement

### Current State
- ✅ Server-side Kerberos authentication implemented
- ✅ HTTP/REST API supports Kerberos
- ❌ No C++ client library with Kerberos support
- ❌ No Python client library with Kerberos support
- ❌ Users must manually implement GSSAPI token handling

### Customer Need
Application developers require:
1. **Simple client libraries** with transparent Kerberos auth
2. **Automatic ticket acquisition** and renewal
3. **Cross-platform support** (Linux, Windows, macOS)
4. **Pythonic interface** for Python developers
5. **Idiomatic C++ API** for native applications

### Business Impact
**Without Client Libraries:**
- Complex manual GSSAPI integration
- Higher barrier to adoption
- Inconsistent client implementations
- Poor developer experience

**With Client Libraries:**
- ✅ Simple, one-line Kerberos authentication
- ✅ Automatic credential management
- ✅ Faster application development
- ✅ Better developer experience

## 🎯 Requirements

### Functional Requirements

#### FR-1: C++ Client Library
- [ ] `KerberosClient` class inheriting from `ThemisDBClient`
- [ ] Automatic GSSAPI context creation
- [ ] Ticket acquisition from credential cache
- [ ] Automatic token renewal on expiration
- [ ] Error handling and retry logic
- [ ] Support for both HTTP and gRPC

#### FR-2: Python Client Library
- [ ] Python package: `themisdb-kerberos`
- [ ] Integration with `gssapi` Python library
- [ ] Context manager support (`with` statement)
- [ ] Async/await support
- [ ] Type hints for IDE support
- [ ] Cross-platform (uses `gssapi` or `winkerberos`)

#### FR-3: Configuration
- [ ] Service principal configuration
- [ ] Credential cache path override
- [ ] Keytab file support (for service accounts)
- [ ] Fallback authentication options
- [ ] Connection pooling with authenticated sessions

#### FR-4: CLI Tool Enhancement
- [ ] Add `--auth kerberos` flag
- [ ] Automatic principal detection
- [ ] Interactive principal selection
- [ ] Ticket status display

### Non-Functional Requirements

#### NFR-1: Ease of Use
- [ ] Zero-configuration for basic use
- [ ] Auto-detection of credentials
- [ ] Sensible defaults
- [ ] Clear error messages

#### NFR-2: Performance
- [ ] Connection pooling
- [ ] Ticket caching
- [ ] Lazy initialization
- [ ] Minimal memory overhead

#### NFR-3: Platform Support
- [ ] Linux (MIT Kerberos, GSSAPI)
- [ ] Windows (SSPI, `winkerberos`)
- [ ] macOS (Heimdal Kerberos)

## 🛠️ Technical Design

### C++ Client Library

```cpp
// File: clients/cpp/kerberos_client.h
namespace themisdb {
namespace client {

class KerberosClient : public ThemisDBClient {
public:
    struct Config {
        std::string host;
        uint16_t port = 9000;
        std::string service_principal;  // e.g., "themisdb/hostname@REALM"
        std::string keytab_path;        // Optional, for service accounts
        std::chrono::seconds timeout{30};
    };
    
    explicit KerberosClient(const Config& config);
    ~KerberosClient() override;
    
    // Connect with Kerberos authentication
    Status connect() override;
    
    // Execute query (authenticated)
    Status execute(const std::string& query, ResultSet& results) override;
    
private:
    Config config_;
    gss_ctx_id_t context_;
    gss_cred_id_t creds_;
    
    Status acquireCredentials();
    Status initSecurityContext();
    Status renewTicket();
};

} // namespace client
} // namespace themisdb
```

### C++ Usage Example

```cpp
#include <themisdb/kerberos_client.h>

// Simple usage
themisdb::client::KerberosClient::Config config;
config.host = "db.example.com";
config.service_principal = "themisdb/db.example.com@EXAMPLE.COM";

themisdb::client::KerberosClient client(config);
auto status = client.connect();

if (status.ok()) {
    ResultSet results;
    client.execute("SELECT * FROM users", results);
}
```

### Python Client Library

```python
# File: clients/python/themisdb_kerberos/__init__.py
from typing import Optional, Dict, Any
import gssapi

class KerberosClient:
    """ThemisDB client with Kerberos authentication"""
    
    def __init__(
        self,
        host: str,
        port: int = 9000,
        service_principal: Optional[str] = None,
        keytab: Optional[str] = None,
        **kwargs
    ):
        self.host = host
        self.port = port
        self.service_principal = service_principal or f"themisdb/{host}"
        self.keytab = keytab
        self._context: Optional[gssapi.SecurityContext] = None
        self._client = None
    
    def __enter__(self):
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
    
    def connect(self) -> None:
        """Establish authenticated connection"""
        # Acquire credentials
        if self.keytab:
            store = {'keytab': self.keytab}
            creds = gssapi.Credentials(
                usage='initiate',
                store=store
            )
        else:
            creds = None  # Use default credential cache
        
        # Create security context
        service_name = gssapi.Name(
            self.service_principal,
            gssapi.NameType.hostbased_service
        )
        
        self._context = gssapi.SecurityContext(
            name=service_name,
            creds=creds,
            usage='initiate'
        )
        
        # Get authentication token
        token = self._context.step()
        
        # Connect to ThemisDB with token
        from themisdb import Client
        self._client = Client()
        self._client.connect(
            f"{self.host}:{self.port}",
            auth_token=token.decode('latin-1')
        )
    
    def execute(self, query: str) -> Dict[str, Any]:
        """Execute authenticated query"""
        if not self._client:
            raise RuntimeError("Not connected")
        return self._client.execute(query)
    
    def close(self) -> None:
        """Close connection"""
        if self._client:
            self._client.close()
            self._client = None
```

### Python Usage Example

```python
from themisdb_kerberos import KerberosClient

# Simple usage with context manager
with KerberosClient("db.example.com") as client:
    results = client.execute("SELECT * FROM users")
    print(results)

# Service account with keytab
client = KerberosClient(
    host="db.example.com",
    service_principal="themisdb/db.example.com@EXAMPLE.COM",
    keytab="/path/to/service.keytab"
)
client.connect()
results = client.execute("SELECT COUNT(*) FROM users")
client.close()
```

### CLI Tool Enhancement

```bash
# Auto-detect and use Kerberos credentials
themisdb-cli --host db.example.com --auth kerberos

# Specify service principal
themisdb-cli --host db.example.com \
             --auth kerberos \
             --service-principal "themisdb/db.example.com@EXAMPLE.COM"

# Use keytab for service account
themisdb-cli --host db.example.com \
             --auth kerberos \
             --keytab /etc/themisdb/service.keytab

# Show ticket status
themisdb-cli --auth kerberos --show-ticket
```

## 📝 Implementation Plan

### Phase 1: C++ Client Library (Week 1-2)
- [ ] **Task 1.1**: Create `KerberosClient` class
- [ ] **Task 1.2**: Implement credential acquisition
- [ ] **Task 1.3**: Implement security context initialization
- [ ] **Task 1.4**: Add ticket renewal logic
- [ ] **Task 1.5**: Integration with HTTP/gRPC clients
- [ ] **Task 1.6**: Unit and integration tests
- [ ] **Task 1.7**: Documentation and examples

### Phase 2: Python Client Library (Week 3)
- [ ] **Task 2.1**: Create Python package structure
- [ ] **Task 2.2**: Implement `KerberosClient` class
- [ ] **Task 2.3**: Add Windows support (`winkerberos`)
- [ ] **Task 2.4**: Add async/await support
- [ ] **Task 2.5**: Unit and integration tests
- [ ] **Task 2.6**: Documentation and examples
- [ ] **Task 2.7**: Publish to PyPI

### Phase 3: CLI Tool Enhancement (Week 4)
- [ ] **Task 3.1**: Add `--auth kerberos` option
- [ ] **Task 3.2**: Implement auto-detection
- [ ] **Task 3.3**: Add ticket status display
- [ ] **Task 3.4**: Update help and documentation
- [ ] **Task 3.5**: Integration tests

## ✅ Acceptance Criteria

### Functional Acceptance
- [ ] C++ client connects with Kerberos
- [ ] Python client connects with Kerberos
- [ ] Automatic credential discovery works
- [ ] Ticket renewal functions correctly
- [ ] CLI tool supports Kerberos auth
- [ ] Cross-platform support verified

### Technical Acceptance
- [ ] Unit test coverage >80%
- [ ] Integration tests pass
- [ ] No memory leaks (C++)
- [ ] Type hints for Python
- [ ] API documentation complete

### User Experience Acceptance
- [ ] Simple one-line authentication
- [ ] Clear error messages
- [ ] Comprehensive examples
- [ ] Quick start guide available

## 🧪 Testing Strategy

### Unit Tests
- Credential acquisition
- Context initialization
- Ticket renewal
- Error handling
- Configuration parsing

### Integration Tests
- End-to-end authentication with KDC
- HTTP and gRPC protocols
- Service account authentication
- Ticket expiration scenarios
- Cross-platform compatibility

## 📚 References

- [GSSAPI Programming Guide](https://docs.oracle.com/cd/E88353_01/html/E37851/gssapi-2.html)
- [Python gssapi Library](https://github.com/pythongssapi/python-gssapi)
- [Kerberos Server Implementation](../../docs/en/security/KERBEROS_AUTHENTICATION.md)

## 🔗 Related Issues

- Parent: Issue #[parent-issue-number] - Kerberos/GSSAPI Authentication Support
- Related: Issue #[grpc-interceptor-issue] - gRPC Kerberos Interceptor

## 💬 Notes

**Dependencies:**
- C++: `libkrb5-dev`, `libgssapi-krb5-2`
- Python: `gssapi`, `winkerberos` (Windows)

**Distribution:**
- C++: Install with package managers
- Python: Publish to PyPI as `themisdb-kerberos`

**Estimated Effort:** 4 weeks (1 developer)

---

**Created:** 2026-01-12 (Future Enhancement from Kerberos Implementation)  
**Status:** 📋 Planned  
**Priority:** MEDIUM  
**Labels:** `type:enhancement`, `area:security`, `area:api`, `priority:P2`, `effort:large`
