### Context

This issue implements the roadmap item 'Geo-Replication with Consistency Levels' for the replication domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Geo-Replication with Consistency Levels

### Goal

Deliver the scoped changes for Geo-Replication with Consistency Levels in src/replication/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Geo-Replication with Consistency Levels
**Priority:** Medium  
**Target Version:** v1.7.0

Support multiple consistency levels for geo-distributed deployments, allowing applications to choose between consistency and availability.

**Features:**
- Per-request consistency level
- Consistency levels: STRONG, BOUNDED_STALENESS, SESSION, EVENTUAL
- Automatic routing based on consistency requirements
- Session tokens for read-your-writes guarantee

**Architecture:**
```cpp
class GeoReplicationManager {
public:
    enum ConsistencyLevel {
        STRONG,             // Linearizable (sync replication to all regions)
        BOUNDED_STALENESS,  // Stale reads within time bound (e.g., 5s)
        SESSION,            // Read-your-writes within session
        EVENTUAL            // No guarantee (fastest)
    };
    
    struct GeoConfig {
        std::vector<std::string> regions;
        uint32_t replication_factor = 3;
        uint32_t local_replicas = 2;
        uint32_t global_replicas = 1;
        ConsistencyLevel default_consistency = SESSION;
    };
    
    // Write with specific consistency level
    bool write(
        const std::string& key,
        const std::string& value,
        ConsistencyLevel consistency = SESSION
    );
    
    // Read with specific consistency level
    std::optional<std::string> read(
        const std::string& key,
        ConsistencyLevel consistency = SESSION,
        const std::string& session_token = ""
    );
    
    // Get session token (for SESSION consistency)
    std::string getSessionToken() const;
    
    // Check staleness of local replica
    std::chrono::milliseconds getStaleness(const std::string& region) const;
};

// Example: Geo-distributed writes
GeoReplicationManager geo_repl(geo_config);

// Strong consistency (slow but guaranteed)
geo_repl.write("critical_data", value, ConsistencyLevel::STRONG);

// Session consistency (read-your-writes)
auto token = geo_repl.getSessionToken();
geo_repl.write("user_profile", profile, ConsistencyLevel::SESSION);
auto profile_read = geo_repl.read("user_profile", ConsistencyLevel::SESSION, token);

// Bounded staleness (fast reads, slightly stale)
auto data = geo_repl.read("analytics_data", ConsistencyLevel::BOUNDED_STALENESS);

// Eventual consistency (fastest, possibly stale)
auto cached = geo_repl.read("product_catalog", ConsistencyLevel::EVENTUAL);
```

**Consistency Guarantees:**
- STRONG: Linearizable, up-to-date reads
- BOUNDED_STALENESS: Stale by at most N seconds/versions
- SESSION: Read-your-writes within session
- EVENTUAL: No guarantee, best performance

---

### Acceptance Criteria

- [ ] Per-request consistency level
- [ ] Consistency levels: STRONG, BOUNDED_STALENESS, SESSION, EVENTUAL
- [ ] Automatic routing based on consistency requirements
- [ ] Session tokens for read-your-writes guarantee
- [ ] STRONG: Linearizable, up-to-date reads
- [ ] BOUNDED_STALENESS: Stale by at most N seconds/versions
- [ ] SESSION: Read-your-writes within session
- [ ] EVENTUAL: No guarantee, best performance

### Relationships

- Roadmap row: #198 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/replication/FUTURE_ENHANCEMENTS.md#geo-replication-with-consistency-levels
- Source key: roadmap:198:replication:v1.7.0:geo-replication-with-consistency-levels

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:198:replication:v1.7.0:geo-replication-with-consistency-levels -->
<!-- roadmap-ref: row=198;module=replication;target=v1.7.0 -->
<!-- roadmap-detail: src/replication/FUTURE_ENHANCEMENTS.md#geo-replication-with-consistency-levels -->
