---
name: End-to-End Integration Testing with Real KDC for Kerberos
about: Implement comprehensive integration tests with MIT Kerberos and Active Directory
title: 'End-to-End Integration Testing with Real KDC for Kerberos'
labels: type:test, area:security, priority:P2, effort:medium
assignees: ''
---

## 📋 Summary

Implement comprehensive end-to-end integration tests for Kerberos/GSSAPI authentication using real Kerberos Key Distribution Centers (MIT Kerberos and Active Directory).

**Parent Feature:** Issue #[parent-issue-number] - Kerberos/GSSAPI Authentication Support

## 🔍 Problem Statement

### Current State
- ✅ Kerberos authentication implemented
- ✅ Unit tests for authentication logic
- ❌ No integration tests with real KDC
- ❌ No Active Directory integration tests
- ❌ Manual testing required for validation

### Customer Need
Development and QA teams require:
1. **Automated testing** with real Kerberos infrastructure
2. **CI/CD integration** for continuous validation
3. **Cross-platform testing** (MIT Kerberos, AD, Heimdal)
4. **Regression testing** to prevent authentication breaks

### Business Impact
**Without Integration Tests:**
- High risk of authentication regressions
- Manual testing bottleneck
- Slow release cycles
- Poor confidence in Kerberos feature

**With Integration Tests:**
- ✅ Automated validation with real KDC
- ✅ Fast feedback in CI/CD
- ✅ High confidence in releases
- ✅ Reduced manual QA effort

## 🎯 Requirements

### Functional Requirements

#### FR-1: Test Infrastructure
- [ ] Docker-based MIT Kerberos KDC
- [ ] Docker-based Active Directory (Samba4 AD)
- [ ] Docker-based Heimdal KDC (BSD compatibility)
- [ ] Automated KDC setup and teardown
- [ ] Pre-configured test realms and principals

#### FR-2: Test Scenarios
- [ ] Basic authentication (user → server)
- [ ] Service-to-service authentication
- [ ] Ticket expiration and renewal
- [ ] Multiple realm support
- [ ] Principal-to-role mapping
- [ ] Fallback authentication
- [ ] Cross-realm authentication
- [ ] Ticket cache management

#### FR-3: Platform Coverage
- [ ] Linux with MIT Kerberos
- [ ] Windows with Active Directory
- [ ] macOS with Heimdal
- [ ] Container environments

#### FR-4: CI/CD Integration
- [ ] GitHub Actions workflow
- [ ] GitLab CI configuration
- [ ] Jenkins pipeline
- [ ] Automated test reports

### Non-Functional Requirements

#### NFR-1: Test Speed
- [ ] Complete test suite <15 minutes
- [ ] Parallel test execution
- [ ] Fast container startup (<2 minutes)

#### NFR-2: Reliability
- [ ] 100% pass rate on clean environment
- [ ] No flaky tests
- [ ] Proper cleanup after failures
- [ ] Deterministic test order

#### NFR-3: Maintainability
- [ ] Self-contained test environment
- [ ] Easy local execution
- [ ] Clear test documentation
- [ ] Minimal external dependencies

## 🛠️ Technical Design

### Test Infrastructure Architecture

```
┌─────────────────────────────────────────────────────┐
│                 Docker Compose                       │
├─────────────────────────────────────────────────────┤
│                                                       │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────┐ │
│  │ MIT Kerberos │  │   Samba4 AD  │  │  Heimdal  │ │
│  │     KDC      │  │     KDC      │  │    KDC    │ │
│  │              │  │              │  │           │ │
│  │ TEST.REALM   │  │  AD.CORP.COM │  │ BSD.LOCAL │ │
│  └──────┬───────┘  └──────┬───────┘  └─────┬─────┘ │
│         │                 │                │       │
│         └─────────────────┴────────────────┘       │
│                          │                          │
│                          ▼                          │
│              ┌────────────────────┐                 │
│              │   ThemisDB Server  │                 │
│              │   (Test Instance)  │                 │
│              └────────────────────┘                 │
│                          ▲                          │
│                          │                          │
│              ┌────────────────────┐                 │
│              │    Test Runner     │                 │
│              │   (pytest/gtest)   │                 │
│              └────────────────────┘                 │
└─────────────────────────────────────────────────────┘
```

### Docker Compose Configuration

```yaml
# File: tests/integration/kerberos/docker-compose.yml
version: '3.8'

services:
  # MIT Kerberos KDC
  mit-kdc:
    image: gcavalcante8808/krb5-server:latest
    hostname: kdc.test.realm
    environment:
      - KRB5_REALM=TEST.REALM
      - KRB5_KDC=kdc.test.realm
      - KRB5_ADMINSERVER=kdc.test.realm
    volumes:
      - ./krb5-config:/etc/krb5kdc
    ports:
      - "88:88/udp"
      - "749:749"
  
  # Active Directory (Samba4)
  samba-ad:
    image: nowsci/samba-domain:latest
    hostname: dc.ad.corp.com
    environment:
      - DOMAIN=AD
      - DOMAINPASS=Admin123!
      - DNSFORWARDER=8.8.8.8
      - HOSTIP=172.20.0.3
    ports:
      - "53:53/udp"
      - "88:88/udp"
      - "135:135"
      - "389:389"
  
  # ThemisDB test instance
  themisdb:
    build:
      context: ../../../
      dockerfile: tests/integration/kerberos/Dockerfile
    depends_on:
      - mit-kdc
      - samba-ad
    environment:
      - THEMIS_KERBEROS_ENABLED=true
      - KRB5_CONFIG=/etc/krb5.conf
    volumes:
      - ./config:/etc/themisdb
      - ./keytabs:/etc/keytabs
    ports:
      - "9000:9000"
```

### Test Implementation

```cpp
// File: tests/integration/kerberos/test_kerberos_e2e.cpp
#include <gtest/gtest.h>
#include "auth/gssapi_authenticator.h"
#include "server/auth_middleware.h"

class KerberosE2ETest : public ::testing::Test {
protected:
    void SetUp() override {
        // Wait for KDC to be ready
        waitForKDC("kdc.test.realm", 88);
        
        // Configure Kerberos
        setenv("KRB5_CONFIG", "/tmp/test_krb5.conf", 1);
        
        // Initialize authenticator
        themis::auth::KerberosConfig config;
        config.service_principal = "themisdb/localhost@TEST.REALM";
        config.keytab_file = "/tmp/test.keytab";
        
        authenticator_ = std::make_unique<themis::auth::GSSAPIAuthenticator>();
        ASSERT_TRUE(authenticator_->initialize(config));
    }
    
    void TearDown() override {
        authenticator_.reset();
    }
    
    std::unique_ptr<themis::auth::GSSAPIAuthenticator> authenticator_;
};

TEST_F(KerberosE2ETest, BasicAuthentication) {
    // Get ticket for test user
    std::string ticket = acquireTicket("testuser@TEST.REALM", "password");
    
    // Authenticate
    auto result = authenticator_->authenticateToken(ticket);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.principal_name, "testuser@TEST.REALM");
}

TEST_F(KerberosE2ETest, ExpiredTicket) {
    // Get expired ticket
    std::string ticket = getExpiredTicket();
    
    // Should fail
    auto result = authenticator_->authenticateToken(ticket);
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("expired"), std::string::npos);
}

TEST_F(KerberosE2ETest, PrincipalToRoleMapping) {
    // Test admin user
    std::string admin_ticket = acquireTicket("admin@TEST.REALM", "admin123");
    auto result = authenticator_->authenticateToken(admin_ticket);
    
    EXPECT_TRUE(result.success);
    EXPECT_NE(std::find(result.roles.begin(), result.roles.end(), "admin"),
              result.roles.end());
}
```

### Python Test Implementation

```python
# File: tests/integration/kerberos/test_kerberos_e2e.py
import pytest
import subprocess
import time
from themisdb_kerberos import KerberosClient

@pytest.fixture(scope="module")
def kdc_setup():
    """Setup KDC using docker-compose"""
    subprocess.run(["docker-compose", "up", "-d"], check=True)
    time.sleep(30)  # Wait for KDC to start
    
    # Create test principal
    subprocess.run([
        "docker-compose", "exec", "mit-kdc",
        "kadmin.local", "-q",
        "addprinc -pw password testuser@TEST.REALM"
    ], check=True)
    
    yield
    
    # Cleanup
    subprocess.run(["docker-compose", "down", "-v"], check=True)

def test_basic_authentication(kdc_setup):
    """Test basic Kerberos authentication"""
    # Acquire ticket
    subprocess.run([
        "kinit", "testuser@TEST.REALM"
    ], input=b"password\n", check=True)
    
    # Connect with Kerberos
    with KerberosClient("localhost", port=9000) as client:
        result = client.execute("SELECT 1")
        assert result is not None

def test_service_account_authentication(kdc_setup):
    """Test service account with keytab"""
    client = KerberosClient(
        host="localhost",
        port=9000,
        keytab="/tmp/service.keytab"
    )
    
    client.connect()
    result = client.execute("SELECT 1")
    assert result is not None
    client.close()

def test_ticket_renewal(kdc_setup):
    """Test automatic ticket renewal"""
    with KerberosClient("localhost", port=9000) as client:
        # Execute query
        client.execute("SELECT 1")
        
        # Wait for ticket to expire (if short lifetime)
        time.sleep(600)
        
        # Should still work due to renewal
        result = client.execute("SELECT 1")
        assert result is not None
```

### CI/CD Integration (GitHub Actions)

```yaml
# File: .github/workflows/kerberos-integration-tests.yml
name: Kerberos Integration Tests

on:
  push:
    paths:
      - 'src/auth/**'
      - 'tests/integration/kerberos/**'
  pull_request:
    paths:
      - 'src/auth/**'

jobs:
  integration-tests:
    runs-on: ubuntu-latest
    
    services:
      mit-kdc:
        image: gcavalcante8808/krb5-server:latest
        options: --hostname kdc.test.realm
        env:
          KRB5_REALM: TEST.REALM
        ports:
          - 88:88/udp
          - 749:749
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Install Kerberos client
        run: |
          sudo apt-get update
          sudo apt-get install -y krb5-user libkrb5-dev
      
      - name: Setup test environment
        run: |
          cd tests/integration/kerberos
          ./setup_test_kdc.sh
      
      - name: Build ThemisDB
        run: |
          cmake -DTHEMIS_ENABLE_KERBEROS=ON ..
          make
      
      - name: Run integration tests
        run: |
          ctest -R kerberos_e2e -V
      
      - name: Upload test results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: test-results
          path: test-results.xml
```

## 📝 Implementation Plan

### Phase 1: Test Infrastructure (Week 1)
- [ ] **Task 1.1**: Create Docker Compose setup
- [ ] **Task 1.2**: Configure MIT Kerberos KDC
- [ ] **Task 1.3**: Configure Samba4 AD
- [ ] **Task 1.4**: Create test realms and principals
- [ ] **Task 1.5**: Verify KDC connectivity

### Phase 2: Test Implementation (Week 2-3)
- [ ] **Task 2.1**: Implement C++ integration tests
- [ ] **Task 2.2**: Implement Python integration tests
- [ ] **Task 2.3**: Add test scenarios (basic, expiration, renewal)
- [ ] **Task 2.4**: Add cross-realm tests
- [ ] **Task 2.5**: Add performance tests

### Phase 3: CI/CD Integration (Week 4)
- [ ] **Task 3.1**: Create GitHub Actions workflow
- [ ] **Task 3.2**: Create GitLab CI configuration
- [ ] **Task 3.3**: Add test reporting
- [ ] **Task 3.4**: Document local test execution
- [ ] **Task 3.5**: Create troubleshooting guide

## ✅ Acceptance Criteria

### Functional Acceptance
- [ ] All test scenarios pass with MIT Kerberos
- [ ] All test scenarios pass with Active Directory
- [ ] Tests run successfully in CI/CD
- [ ] Local test execution works
- [ ] Test coverage >80% of Kerberos code

### Technical Acceptance
- [ ] Complete test suite runs <15 minutes
- [ ] No flaky tests (100% pass rate)
- [ ] Proper cleanup after failures
- [ ] Test infrastructure is reproducible

### Documentation Acceptance
- [ ] Test setup guide
- [ ] Local execution instructions
- [ ] CI/CD integration documented
- [ ] Troubleshooting guide

## 🧪 Test Scenarios

### Basic Authentication Tests
- User authentication with password
- Service authentication with keytab
- Multiple realm support
- Principal-to-role mapping

### Advanced Tests
- Ticket expiration handling
- Automatic ticket renewal
- Cross-realm authentication
- Delegation and impersonation

### Error Handling Tests
- Invalid credentials
- Expired tickets
- KDC unavailable
- Network timeouts

### Performance Tests
- Authentication latency
- Connection pooling
- High-concurrency authentication
- Ticket cache performance

## 📚 References

- [MIT Kerberos Test Suite](https://github.com/krb5/krb5/tree/master/src/tests)
- [Docker Kerberos Images](https://hub.docker.com/r/gcavalcante8808/krb5-server)
- [Kerberos Implementation](../../docs/en/security/KERBEROS_AUTHENTICATION.md)

## 🔗 Related Issues

- Parent: Issue #[parent-issue-number] - Kerberos/GSSAPI Authentication Support
- Related: Issue #[client-libraries-issue] - Kerberos Client Libraries

## 💬 Notes

**Test Environment:**
- Uses Docker for isolation
- Supports local and CI/CD execution
- Self-contained with minimal dependencies

**Maintenance:**
- Update KDC images regularly
- Keep test principals synchronized
- Monitor test execution times

**Estimated Effort:** 4 weeks (1 developer + 1 QA)

---

**Created:** 2026-01-12 (Future Enhancement from Kerberos Implementation)  
**Status:** 📋 Planned  
**Priority:** MEDIUM  
**Labels:** `type:test`, `area:security`, `priority:P2`, `effort:medium`
