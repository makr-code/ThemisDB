# Audit Trail Integrity - Quick Reference Guide

**Version:** 0.0.48 | **Status:** Production-Ready | **Date:** August 18, 2026

---

## Quick Links

- **API Documentation:** `include/governance/governance_audit_integrity.h`
- **Implementation:** `src/governance/governance_audit_integrity.cpp`
- **Tests:** `tests/governance/test_audit_integrity.cpp` (40+ test cases)
- **Full Documentation:** `docs/AUDIT_TRAIL_INTEGRITY.md`
- **Operator Procedures:** Section 9 of AUDIT_TRAIL_INTEGRITY.md

---

## Common Tasks

### Task 1: Create an Audit Integrity Manager

```cpp
#include "governance/governance_audit_integrity.h"

using namespace themis::governance;

// Create signing signer
auto signer = std::make_shared<AuditSigner>(
    AuditSigner::SignatureAlgorithm::HMAC_SHA256,
    "signing-key-2024-001",
    "your-secret-key-here"
);

// Create retention policy
AuditRetentionPolicy policy;
policy.policy_id = "default-retention";
policy.retention_period_days = 2555;  // 7 years
policy.archive_after_days = 365;      // Archive after 1 year

// Create manager
AuditIntegrityManager manager(policy, signer);
```

### Task 2: Add an Audit Entry

```cpp
// Create entry
ImmutableAuditEntry entry;
entry.entry_id = "audit-entry-001";
entry.rule_id = "policy-rule-gdpr-001";
entry.operation = "update";
entry.user = "policy-admin@example.com";
entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
).count();
entry.details = {
    {"field_name", "retention_period"},
    {"old_value", "90 days"},
    {"new_value", "180 days"}
};

// Add entry (automatically signed)
auto signed_entry = manager.addEntry(entry);

std::cout << "Entry ID: " << signed_entry.entry_id << "\n";
std::cout << "Signature: " << signed_entry.signature_info.signature << "\n";
std::cout << "Hash: " << signed_entry.signature_info.entry_hash << "\n";
```

### Task 3: Verify Audit Trail Integrity

```cpp
// Verify entire trail
auto incidents = manager.verifyIntegrity();

if (incidents.empty()) {
    std::cout << "✓ Audit trail integrity verified\n";
} else {
    std::cout << "✗ TAMPERING DETECTED!\n";
    
    for (const auto& incident : incidents) {
        std::cout << "  Incident: " << incident.incident_id << "\n";
        std::cout << "  Type: " << static_cast<int>(incident.type) << "\n";
        std::cout << "  Evidence: " << incident.evidence << "\n";
    }
}
```

### Task 4: Query Audit Entries

```cpp
// Query by rule
auto entries = manager.queryEntries("policy-rule-gdpr-001");

// Query by user
entries = manager.queryEntries(std::nullopt, "policy-admin@example.com");

// Query by time range
int64_t start = /* ... */;
int64_t end = /* ... */;
entries = manager.queryEntries(
    std::nullopt, 
    std::nullopt, 
    start, 
    end
);

// Use results
for (const auto& entry : entries) {
    std::cout << entry.entry_id << ": " << entry.operation 
              << " by " << entry.user << "\n";
}
```

### Task 5: Archive Expired Entries

```cpp
// Archive entries older than policy threshold
int64_t archived = manager.archiveExpiredEntries();
std::cout << "Archived " << archived << " entries\n";

// Later, perform cleanup
int64_t deleted = manager.performCleanup();
std::cout << "Deleted " << deleted << " expired entries\n";
```

### Task 6: Place Legal Hold

```cpp
// Create legal hold
LegalHold hold;
hold.hold_id = "HOLD-2024-LITIGATION-001";
hold.rule_id = "";  // Empty = all entries
hold.initiated_by = "legal@company.com";
hold.reason = "Pending litigation";
hold.status = "active";
hold.expire_at_ms = 0;  // Indefinite

// Add hold
manager.addLegalHold(hold);

// Now retention policy can't delete entries

// Later, release hold
manager.releaseLegalHold("HOLD-2024-LITIGATION-001");
```

### Task 7: Rotate Signing Key

```cpp
// Create new signer with new key
auto new_signer = std::make_shared<AuditSigner>(
    AuditSigner::SignatureAlgorithm::HMAC_SHA256,
    "signing-key-2024-Q2",
    "new-secret-key-here"
);

// Record rotation in audit trail
ImmutableAuditEntry rotation_entry;
rotation_entry.entry_id = "key-rotation-event";
rotation_entry.operation = "key_rotation";
rotation_entry.user = "security-admin@example.com";
rotation_entry.timestamp_ms = current_time_ms;

// Perform rotation
manager.rotateKey(new_signer, rotation_entry);

// New entries now signed with new key
// Old entries remain verifiable
```

### Task 8: Export Audit Trail

```cpp
// Export to JSON
auto export_data = manager.exportAuditTrail(false);  // false = no compression

// Save to file
std::ofstream file("audit_trail_backup.json");
file << export_data.dump(2);

// Or compressed
export_data = manager.exportAuditTrail(true);  // true = compress
```

### Task 9: Monitor Performance

```cpp
// Get performance metrics
auto metrics = manager.getPerformanceMetrics();

std::cout << "Signing latency: " << metrics["avg_signing_ms"] << " ms\n";
std::cout << "Verification latency: " << metrics["avg_verification_ms"] << " ms\n";
std::cout << "Total entries: " << metrics["total_entries"] << "\n";

// Check against requirements
if (!metrics["signing_latency_ok"].get<bool>()) {
    std::cout << "WARNING: Signing latency exceeded 1ms\n";
}

if (!metrics["verification_latency_ok"].get<bool>()) {
    std::cout << "WARNING: Verification latency exceeded 10ms\n";
}
```

---

## Performance Targets

| Operation | Target | Typical |
|-----------|--------|---------|
| Sign entry | ≤1ms | 0.3-0.5ms |
| Verify entry | ≤10ms | 1-2ms |
| Archive entries | N/A | 5-10ms each |
| Full trail verification | N/A | 100-200ms (100 entries) |

---

## Tamper Incident Types

| Type | Cause | Severity |
|------|-------|----------|
| INVALID_SIGNATURE | Signature doesn't match | CRITICAL |
| BROKEN_CHAIN | Previous hash mismatch | CRITICAL |
| MISSING_ENTRY | Gap in sequence | CRITICAL |
| REORDERED_ENTRY | Out of order | CRITICAL |
| ALTERED_ENTRY | Content changed | CRITICAL |
| CLOCK_SKEW | Timestamp anomaly | WARNING |
| KEY_ROTATION_ERROR | Key management issue | WARNING |

---

## Retention Policy Lifecycle

```
Day 0: Entry created → Hot storage
Day 365: Entry archived (if configured)
Day 2555: Entry deleted (if not on legal hold)
```

**Configuration:**
```cpp
policy.retention_period_days = 2555;     // Total retention
policy.archive_after_days = 365;         // Archive threshold
policy.enable_legal_hold = true;         // Legal hold support
policy.compress_on_archive = true;       // Compress archived entries
```

---

## Cryptographic Algorithms

**HMAC-SHA256 (Default)**
- ✅ Fast (~0.3-0.5ms)
- ✅ Symmetric (shared secret)
- ❌ No non-repudiation
- Use for: Standard governance audits

**RSA-SHA256**
- ⚠️ Slower (~1-5ms)
- ✅ Asymmetric (public key verification)
- ✅ Non-repudiation
- Use for: Compliance, legal proceedings

---

## Common Issues & Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| "Verification failed" | Entry altered or key mismatch | Check entry content and key_id |
| "Broken chain" | Previous entry deleted or altered | Restore from backup or regenerate |
| "Missing entry" | Entries deleted without audit | Check for unauthorized deletion |
| "Slow signing" | High system load or RSA algorithm | Use HMAC or optimize system |

---

## Regulatory Alignment

| Regulation | Requirement | Satisfied By |
|------------|-------------|--------------|
| GDPR | Audit trail with consent proof | Immutable entries with timestamps |
| HIPAA | Non-repudiation | RSA-SHA256 signatures |
| SOC2 | Integrity monitoring | Tamper detection |
| PCI-DSS | 90-day retention | Configurable retention policy |

---

## API Quick Reference

```cpp
// Main class
AuditIntegrityManager {
    ImmutableAuditEntry addEntry(const ImmutableAuditEntry& entry);
    std::vector<TamperIncident> verifyIntegrity();
    std::vector<TamperIncident> verifyTimeRange(int64_t start, int64_t end);
    std::optional<ImmutableAuditEntry> getEntry(const std::string& id);
    std::vector<ImmutableAuditEntry> queryEntries(...);
    int64_t archiveExpiredEntries();
    int64_t performCleanup();
    nlohmann::json getPerformanceMetrics() const;
    nlohmann::json exportAuditTrail(bool compress = false) const;
    bool importAuditTrail(const nlohmann::json& data);
    void rotateKey(const std::shared_ptr<AuditSigner>& new_signer, ...);
};

// Signer
AuditSigner {
    SignatureInfo signEntry(const ImmutableAuditEntry& entry, ...);
    bool verifySignature(const ImmutableAuditEntry& entry, ...);
    std::string getKeyId() const;
    std::string getAlgorithmName() const;
};

// Detector
AuditTamperDetector {
    std::optional<TamperIncident> verifyEntry(...);
    std::vector<TamperIncident> verifyAuditTrail(...);
    std::vector<TamperIncident> verifyTimeRange(...);
    static nlohmann::json generateTamperReport(...);
};

// Retention
AuditRetentionManager {
    bool shouldArchive(const ImmutableAuditEntry& entry, ...);
    bool shouldDelete(const ImmutableAuditEntry& entry, ...);
    bool isOnLegalHold(const std::string& rule_id) const;
    void addLegalHold(const LegalHold& hold);
    void releaseLegalHold(const std::string& hold_id);
    void setPolicy(const AuditRetentionPolicy& policy, ...);
};
```

---

## Contact & Support

- **Documentation:** See `docs/AUDIT_TRAIL_INTEGRITY.md`
- **Implementation:** See `src/governance/governance_audit_integrity.cpp`
- **Tests:** See `tests/governance/test_audit_integrity.cpp`
- **Issues:** Report via project issue tracking

---

**Last Updated:** August 18, 2026  
**Version:** 0.0.48  
**Status:** Production-Ready
