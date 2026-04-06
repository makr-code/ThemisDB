# ThemisDB Dual Namespace Approach - Explained

**Version:** 1.0  
**Date:** 2026-01-20  
**Status:** Architecture Decision Record

---

## Executive Summary

ThemisDB uses a **dual namespace approach** with:
- **`themis`** as the primary namespace (90%+ of codebase)
- **`themisdb`** as an extended namespace for specific subsystems

**This is intentional, architecturally sound, and should NOT be "fixed" to use only one namespace.**

---

## Why Two Namespaces?

### Historical Context

The dual namespace approach emerged from the need to clearly separate:
1. **Core ThemisDB functionality** (`themis`) - the database system
2. **Extended/specialized components** (`themisdb`) - advanced subsystems

This is similar to how:
- Linux has both `std::` and `boost::`
- Kubernetes has `k8s.io` and `extensions.k8s.io`
- PostgreSQL has core and extension namespaces

---

## Namespace Allocation

### `themis` Namespace (Primary) - 90%+ of Codebase

**Purpose:** Core database functionality

**Components:**
- `themis::storage` - Storage layer (RocksDB)
- `themis::query` - Query engine and AQL parser
- `themis::server` - HTTP/gRPC servers and API handlers
- `themis::auth` - Authentication and authorization
- `themis::llm` - LLM integration (core)
- `themis::sharding` - Standard sharding components
- `themis::cache` - Caching mechanisms
- `themis::utils` - Utilities and helpers
- ... and 20+ other modules

**Philosophy:** 
> "If it's part of the core database, it's in `themis`"

---

### `themisdb` Namespace (Extended) - ~10% of Codebase

**Purpose:** Specialized/extended components that benefit from isolation

**Components:**

#### 1. `themisdb::sharding` - RAFT Consensus System

**Why separate?**
- RAFT is a complex consensus algorithm deserving its own namespace
- Clear distinction from standard sharding in `themis::sharding`
- Easier to maintain and test independently
- Can evolve without affecting standard sharding

**Files (12):**
- `raft_consensus.h/cpp`
- `raft_log.h/cpp`
- `raft_state.h/cpp`
- `raft_wal_integration.h/cpp`
- `quorum_manager.h/cpp`
- `partition_detector.h/cpp`
- `hot_spare_manager.h/cpp`
- `predictive_detector.h/cpp`
- `operational_metrics.h/cpp`
- `redundancy_strategy.h/cpp`
- `auto_recovery_manager.h/cpp`
- `backpressure_protocol.h/cpp`

#### 2. `themisdb::temporal` - Temporal Conflict Resolution

**Why separate?**
- Specialized bitemporal data handling
- Advanced conflict resolution algorithms
- Distinct from core transaction management

**Files (1):**
- `temporal_conflict_resolver.h/cpp`

#### 3. `themisdb::replication` - Replication Orchestration

**Why separate?**
- High-level replication orchestration vs low-level write concern tracking
- Leader election and failover logic
- Complements `themis::sharding::ReplicationCoordinator` (different abstraction level)

**Files (2):**
- `replication_manager.h/cpp`
- `multi_master_replication.h/cpp`

**Key Distinction:**
- `themisdb::replication::ReplicationManager` = Orchestration (who is leader? how to failover?)
- `themis::sharding::ReplicationCoordinator` = Write concern tracking (did replicas ack this write?)

#### 4. `themisdb::storage` - Extended Storage Components

**Why separate?**
- Advanced storage features beyond core RocksDB wrapper
- RAID-level redundancy strategies

**Files (1):**
- `blob_redundancy_manager.h/cpp`

#### 5. `themisdb::streaming` - Streaming Protocols

**Why separate?**
- Real-time streaming protocols
- Backpressure management

**Note:** Currently only documented in architecture, not yet fully implemented.

---

## Architecture Benefits

### 1. Clear Separation of Concerns

```
themis::sharding::ShardRouter         ← Core sharding (routing, topology)
themisdb::sharding::RaftConsensus     ← Advanced consensus (RAFT)
```

Developers immediately know:
- `themis::` = core database feature
- `themisdb::` = advanced/specialized subsystem

### 2. Independent Evolution

Extended components can evolve without affecting core:
- RAFT implementation can be rewritten without touching standard sharding
- Temporal resolution can add features independently
- Clear API boundaries between subsystems

### 3. Testability

```cpp
// Test RAFT in isolation
using namespace themisdb::sharding;
RaftConsensus raft(...);

// Test standard sharding separately
using namespace themis::sharding;
ShardRouter router(...);
```

### 4. Documentation Clarity

Architecture docs can clearly separate:
- **Core Features** (themis namespace) - always available
- **Extended Features** (themisdb namespace) - may require configuration

---

## Common Questions

### Q: Why not just use nested namespaces?

**A:** We do! Both approaches are used:

```cpp
namespace themis::sharding { }      // Nested in primary
namespace themisdb::sharding { }    // Nested in extended
```

The dual root namespaces (`themis` vs `themisdb`) provide the top-level distinction.

### Q: Doesn't this confuse developers?

**A:** No more than `std::` vs `boost::` in C++. With proper documentation (like this file), it becomes clear and natural.

### Q: Should I use `themis` or `themisdb` for new code?

**A:** Use this decision tree:

```
Is it core database functionality?
├─ YES → Use `themis::`
└─ NO → Is it an advanced/specialized subsystem?
   ├─ YES → Consider `themisdb::`
   └─ NO → Use `themis::`
```

**Rule of thumb:** Default to `themis::` unless you have a strong reason for `themisdb::`.

### Q: Can I move code from `themis` to `themisdb` or vice versa?

**A:** Yes, but:
1. It's a breaking API change
2. Requires strong justification
3. Must be documented in CHANGELOG
4. Typically done only in major versions

---

## Documentation References

### Official Architecture Documentation

**Primary Source:** `docs/de/architecture/namespace-architektur.md`
- Section: "Root-Namespace" (lines 33-81)
- Section: "Detaillierte Namespace-Dokumentation" (lines 85-434)

**Key Quote:**
> "Der primäre Root-Namespace ist `themis`, mit einem alternativen `themisdb`-Namespace für bestimmte Komponenten."

### Audit Report

**Source:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md`
- Section 1: Namespace-Konsistenz-Analyse
- Section 5.1: Recommendations

**Key Finding:**
> "Die Namespace-Nutzung ist **architektonisch korrekt** und folgt der dokumentierten Struktur."

---

## Examples in the Wild

### Example 1: RAFT vs Standard Sharding

```cpp
// Core sharding - routes queries to shards
namespace themis::sharding {
    class ShardRouter {
        ShardResult routeQuery(const Query& q);
    };
}

// Extended sharding - RAFT consensus
namespace themisdb::sharding {
    class RaftConsensus {
        void replicateLogEntry(const LogEntry& e);
        LeaderElectionResult electLeader();
    };
}
```

**Different purposes, different namespaces.**

### Example 2: Replication Layers

```cpp
// High-level orchestration
namespace themisdb::replication {
    class ReplicationManager {
        void promoteToLeader();
        void demoteToFollower();
    };
}

// Low-level write concern
namespace themis::sharding {
    class ReplicationCoordinator {
        WriteResult waitForReplication(LSN lsn);
        void recordAcknowledgment(string replica_id, LSN lsn);
    };
}
```

**Complementary, not duplicate.**

---

## Migration Guide (If Needed)

### Scenario: Accidentally Used Wrong Namespace

If you accidentally put code in the wrong namespace:

**Step 1: Evaluate Impact**
```bash
# Find all references
git grep "namespace_name::ClassName"
```

**Step 2: Create Migration Issue**
- Document why the change is needed
- List all affected files
- Plan backward compatibility

**Step 3: Implement Migration**
```cpp
// Option 1: Namespace alias (backward compatible)
namespace old_namespace {
    using new_namespace::ClassName;
}

// Option 2: Deprecation warning
[[deprecated("Use new_namespace::ClassName instead")]]
using OldClassName = new_namespace::ClassName;
```

**Step 4: Update Documentation**
- Add to CHANGELOG.md
- Update architecture docs
- Add migration notes

---

## Conclusion

The dual namespace approach in ThemisDB is:

✅ **Intentional** - Designed for clear separation of concerns  
✅ **Documented** - Fully documented in architecture guides  
✅ **Beneficial** - Provides clarity, testability, and modularity  
✅ **Correct** - Should NOT be "fixed" or unified

**When in doubt:** Default to `themis::` for new code. Only use `themisdb::` for genuinely advanced/specialized subsystems that benefit from isolation.

---

## Further Reading

- **Architecture Guide:** `docs/de/architecture/namespace-architektur.md`
- **Audit Report:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md`
- **Audit Summary:** `NAMESPACE_AUDIT_SUMMARY.md`
- **Coding Standards:** `CODING_STANDARDS.md` (namespace conventions)

---

**Document Status:** ✅ Approved and Active  
**Last Updated:** 2026-04-06  
**Maintainer:** ThemisDB Core Team
