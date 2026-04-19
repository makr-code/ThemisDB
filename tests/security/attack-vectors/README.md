> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Attack Vector Test Templates

This directory contains reusable test templates for systematic attack vector validation.

## Directory Structure

```
tests/security/attack-vectors/
├── README.md                          # This file
├── network/                           # Network attack vector tests (to be created)
├── authentication/                    # Auth attack vector tests (to be created)
├── injection/                         # Injection attack tests (to be created)
├── crypto/                            # Cryptography attack tests (to be created)
├── distributed/                       # Distributed system attacks (to be created)
└── scripts/                           # Helper scripts (to be created)
```

## Overview

These test templates provide a structured approach to validating security against known attack vectors. Tests are organized by attack category and integrate with the systematic attack vector analysis workflow.

## Integration with Workflow

Tests in this directory are referenced by:
- `.github/workflows/attack-vector-analysis.yml` - Automated attack vector analysis
- `docs/de/security/ANGRIFFSVEKTOREN_ANALYSE_RUNBOOK.md` - Manual testing guide

## Quick Start

### Running Tests

```bash
# Run all attack vector tests
cd /home/runner/work/ThemisDB/ThemisDB
mkdir -p build && cd build
cmake .. -DENABLE_SECURITY_TESTS=ON
make security_tests
ctest -R attack_vector
```

### Adding New Tests

1. Choose appropriate category directory
2. Create C++ test file following naming convention: `test_<attack_type>.cpp`
3. Use existing security tests as templates
4. Add test to `CMakeLists.txt`
5. Document the attack vector being tested

## Test Categories

### 1. Network Attack Vectors

Tests for network-level attacks:
- HTTP Request Smuggling
- SSRF (Server-Side Request Forgery)
- WebSocket Injection
- gRPC Protocol Attacks
- MQTT Topic Injection
- CORS Misconfiguration
- Path Traversal

**Reference:** See existing tests like `test_http_aql.cpp`, `test_websocket_cdc.cpp`, `test_grpc_channel_pool.cpp`

### 2. Authentication Attack Vectors

Tests for authentication/authorization attacks:
- JWT Token Manipulation
- Session Fixation/Hijacking
- Brute Force Prevention
- API Key Enumeration
- Privilege Escalation
- IDOR (Insecure Direct Object Reference)

**Reference:** See existing tests like `test_jwt_validator.cpp`, `test_access_control.cpp`, `test_mfa_authenticator.cpp`

### 3. Injection Attack Vectors

Tests for injection vulnerabilities:
- AQL Injection
- NoSQL Injection
- Command Injection
- LLM Prompt Injection
- Template Injection

**Reference:** See existing tests like `test_aql_injection_detector.cpp`, `test_input_validator.cpp`

### 4. Cryptography Attack Vectors

Tests for cryptographic weaknesses:
- Weak Cipher Suites
- Padding Oracle Attacks
- Key Management Issues
- IV Reuse
- Timing Attacks

**Reference:** See existing tests like `test_encryption.cpp`, `test_vault_key_provider.cpp`, `test_hsm_provider.cpp`

### 5. Distributed System Attack Vectors

Tests for distributed system vulnerabilities:
- Shard Key Enumeration
- Cross-Shard Injection
- Consensus Protocol Attacks
- MVCC Bypass
- Data Integrity Violations

**Reference:** See existing tests like `test_distributed_transactions.cpp`, `test_raft_consensus.cpp`, `test_mvcc.cpp`

## Test Template Example

```cpp
#include <gtest/gtest.h>
#include "themis/security/test_utils.h"

namespace themis {
namespace security {
namespace tests {

class NetworkAttackVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }
    
    void TearDown() override {
        // Cleanup
    }
};

TEST_F(NetworkAttackVectorTest, HTTPRequestSmuggling) {
    // Test description: Verify CL.TE vulnerability is blocked
    
    // Arrange: Create malicious HTTP request
    std::string malicious_request = 
        "POST / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 6\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "0\r\n"
        "\r\n"
        "GET /admin HTTP/1.1\r\n";
    
    // Act: Send request
    auto response = sendRequest(malicious_request);
    
    // Assert: Verify attack is blocked
    EXPECT_TRUE(response.isError());
    EXPECT_TRUE(response.errorContains("malformed"));
    
    // Verify security logging
    auto logs = getSecurityLogs();
    EXPECT_TRUE(logs.contains("attack_detected"));
}

} // namespace tests
} // namespace security
} // namespace themis
```

## Best Practices

1. **Negative Testing**: Tests should verify attacks are BLOCKED, not successful
2. **Documentation**: Comment what attack vector is being tested and why
3. **Isolation**: Each test should be independent and self-contained
4. **Cleanup**: Always clean up resources in TearDown()
5. **Realistic**: Use real-world attack patterns and payloads
6. **Logging**: Verify security events are properly logged
7. **False Positives**: Document any known false positives

## Existing Security Tests

ThemisDB already has extensive security tests. The attack vector tests in this directory complement existing tests by:
- Providing systematic coverage of all attack categories
- Following the structure defined in `ANGRIFFSVEKTOREN_ANALYSE.md`
- Enabling automated validation through CI/CD
- Supporting the repeatable analysis process

### Related Existing Tests

| Category | Existing Tests | Location |
|----------|---------------|----------|
| Authentication | JWT, MFA, Access Control | `test_jwt_*.cpp`, `test_mfa_*.cpp`, `test_access_control.cpp` |
| Encryption | Field encryption, HSM, Vault | `test_encryption*.cpp`, `test_hsm_*.cpp`, `test_vault_*.cpp` |
| Injection | AQL Injection, Input Validation | `test_aql_injection_detector.cpp`, `test_input_validator.cpp` |
| Network | HTTP, WebSocket, gRPC, MQTT | `test_http*.cpp`, `test_websocket*.cpp`, `test_grpc*.cpp`, `test_mqtt*.cpp` |
| Distributed | Transactions, MVCC, Raft | `test_distributed_*.cpp`, `test_mvcc.cpp`, `test_raft_*.cpp` |

## Integration with Security Tools

### OWASP ZAP

Tests can be run through OWASP ZAP proxy for dynamic analysis:

```bash
# Start ZAP proxy
docker run -d -p 8090:8090 owasp/zap2docker-stable zap.sh -daemon

# Run tests through proxy
export HTTP_PROXY=http://localhost:8090
ctest -R attack_vector

# Generate ZAP report
docker exec <container> zap-cli report -o zap-report.html
```

### AFL++ Fuzzing

Tests can be used as fuzzing targets:

```bash
# Build with AFL++ instrumentation
CC=afl-clang-lto CXX=afl-clang-lto++ cmake .. -DENABLE_FUZZING=ON
make fuzz_attack_vectors

# Run fuzzing
afl-fuzz -i testdata -o findings ./fuzz_attack_vectors @@
```

## Compliance Mapping

These tests support compliance with:

- **BSI C5**: DEV-01 (Secure Development), OPS-10 (Vulnerability Management)
- **ISO 27001**: A.12.6.1 (Technical Vulnerability Management), A.14.2.5 (Secure Development)
- **OWASP ASVS**: V1 (Architecture), V4 (Access Control), V5 (Validation), V8 (Data Protection)
- **NIST CSF**: DE.CM-8 (Vulnerability Scans), PR.DS-5 (Protection Against Data Leaks)

## Test Execution

### Local Development

```bash
# Run all attack vector tests
ctest -R attack_vector -V

# Run specific category
ctest -R attack_vector_injection -V

# Run with memory checking
ctest -R attack_vector -T memcheck
```

### CI/CD Pipeline

Tests are automatically executed in:
- Pull Request validation (`.github/workflows/ci.yml`)
- Security scans (`.github/workflows/security-scan.yml`)
- Attack vector analysis (`.github/workflows/attack-vector-analysis.yml`)
- Weekly security audits

### Test Reports

After execution, reports are available:
- JUnit XML: `build/test-reports/attack-vectors.xml`
- HTML Report: `build/test-reports/attack-vectors.html`
- Coverage: `build/coverage/attack-vectors/`

## Future Development

Planned additions:
- [ ] Network attack vector test suite
- [ ] Authentication/authorization test suite
- [ ] Injection attack test suite
- [ ] Cryptography attack test suite
- [ ] Distributed system attack test suite
- [ ] Helper scripts for test execution
- [ ] Automated report generation
- [ ] Integration with security scanners

## Contributing

To contribute attack vector tests:

1. Fork the repository
2. Create test file in appropriate category
3. Follow existing test patterns and style
4. Add documentation for the attack vector
5. Submit PR with "security-test" label
6. Ensure all existing tests pass

## References

### Internal Documentation
- [Attack Vector Analysis](../../../docs/de/security/ANGRIFFSVEKTOREN_ANALYSE.md)
- [Attack Vector Analysis Runbook](../../../docs/de/security/ANGRIFFSVEKTOREN_ANALYSE_RUNBOOK.md)
- [Threat Model](../../../docs/de/security/security_threat_model.md)
- [Security Policy](../../../SECURITY.md)

### External Standards
- [OWASP Testing Guide](https://owasp.org/www-project-web-security-testing-guide/)
- [OWASP Top 10](https://owasp.org/www-project-top-ten/)
- [CWE Top 25](https://cwe.mitre.org/top25/)
- [NIST SP 800-115](https://csrc.nist.gov/publications/detail/sp/800-115/final) - Technical Security Testing
- [BSI C5](https://www.bsi.bund.de/EN/Topics/CloudComputing/Compliance_Controls_Catalogue/Compliance_Controls_Catalogue_node.html)

### Testing Frameworks
- [Google Test Documentation](https://google.github.io/googletest/)
- [AFL++ Documentation](https://aflplus.plus/)
- [OWASP ZAP Documentation](https://www.zaproxy.org/docs/)

## Support

For questions or issues:
- **GitHub Issues**: [ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
- **Security**: See `SECURITY.md` for responsible disclosure
- **Documentation**: `docs/de/security/`

---

**Note:** This directory structure is part of the systematic attack vector analysis framework introduced in January 2026. For the complete workflow, see `.github/workflows/attack-vector-analysis.yml` and the runbook at `docs/de/security/ANGRIFFSVEKTOREN_ANALYSE_RUNBOOK.md`.
