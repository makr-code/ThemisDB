# Data Migration and Compatibility Matrix

## Overview

ThemisDB v1.4+ introduces a comprehensive data migration framework for managing version compatibility and safe upgrades in distributed sharding environments. This document defines the compatibility matrix and migration procedures.

## Version Compatibility Matrix

### ThemisDB Versions

| Source Version | Target Version | Migration Required | Data Format Changes | Downtime Required | Auto-Rollback |
|---------------|----------------|-------------------|--------------------|--------------------|---------------|
| 1.3.x | 1.4.0 | Yes | Schema versioning added | No | Yes |
| 1.4.0 | 1.4.1 | No | None | No | Yes |
| 1.4.x | 1.5.0 | Yes | TBD | TBD | Yes |
| < 1.3.0 | 1.4.x | Not supported | - | - | - |

### Consensus Module Compatibility

| Module | Version | Compatible With | Migration Path |
|--------|---------|----------------|----------------|
| Raft | 1.4.0 | All 1.4.x | Direct upgrade |
| Gossip | 1.4.0 | All 1.4.x | Direct upgrade |
| Paxos | 1.4.0 (new) | All 1.4.x | Requires initialization |

### Transaction Protocol Compatibility

| Protocol | Supported Since | Backward Compatible | Forward Compatible |
|----------|----------------|--------------------|--------------------|
| 2PC | 1.0.0 | Yes | Yes |
| 3PC | 1.4.0 | Yes (falls back to 2PC) | Yes |
| SAGA | 1.4.0 | No | Yes |
| Percolator | 1.4.0 | No | Yes |

## References

- [Distributed Systems Migration Best Practices](https://themisdb.com/docs/migration)
- [Zero-Downtime Migration Guide](https://themisdb.com/docs/zero-downtime)
- [Consensus Module Architecture](../sharding/CONSENSUS_MODULE.md)
- [Sharding Overview](https://themisdb.com/docs/sharding)
