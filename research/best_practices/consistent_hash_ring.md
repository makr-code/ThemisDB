# Consistent Hashing for Stateless Load Distribution

**Metadaten:**
- Source: Karger et al. (1997) — "Consistent Hashing and Random Trees: Distributed Caching Protocols for Relieving Hot Spots on the World Wide Web"
- URL: https://dl.acm.org/doi/10.1145/258533.258660
- Tags: distributed-systems, load-balancing
- ThemisDB-Versionen: v2.1.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

Naive modulo-based sharding (`hash(key) % N`) causes O(K) key remapping when a node is added or removed from a cluster of N nodes. Karger et al.'s consistent hashing algorithm reduces this to O(K/N) — only the keys that need to move actually move. Each node is mapped to multiple virtual nodes (replicas) on a hash ring; a key is assigned to the first virtual node clockwise from its hash position. Adding or removing a physical node only redistributes the keys between the predecessor and successor virtual nodes on the ring.

ThemisDB's distributed gateway (`src/server/distributed_gateway.cpp`) uses consistent hashing for WebSocket and SSE session affinity: a client reconnecting to any gateway node in the cluster is routed to the same backend node as its previous connection, enabling stateful session resumption without shared session storage.

## 🎯 Core Principles

- **Virtual nodes (vnodes) for uniform distribution**: Each physical node maps to V virtual nodes on the ring (ThemisDB uses V=150). More vnodes → more uniform key distribution across heterogeneous nodes.
- **O(log N) lookup via sorted ring**: The ring is stored as a `std::map<uint64_t, NodeId>` (sorted by hash); lookup uses `lower_bound(key_hash)` for O(log N) time.
- **Minimal disruption on membership changes**: Only the keys between the predecessor virtual node and the new/removed virtual node's hash position need to be remapped.
- **Deterministic hash function**: All nodes in the cluster must use the same hash function and seed; ThemisDB uses MurmurHash3-32 (see `murmur_hash_deterministic_sharding.md`) for ring position computation.
- **Health-aware routing**: Unhealthy nodes are removed from the ring; their key range is absorbed by the next healthy node. Health state is propagated via gossip protocol.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/server/distributed_gateway.cpp` — `ConsistentHashRing` class; `addNode(NodeId, weight)`, `removeNode(NodeId)`, `getNode(key)` API.
- `src/server/` WebSocket and SSE session handlers — `session_key = clientId + ":" + subscriptionId`; `ring.getNode(hash(session_key))` determines the backend node.
- `src/server/cluster_membership.cpp` — Gossip-based health updates call `ring.removeNode(dead_node)` and `ring.addNode(recovered_node)`.

### What Was Adopted?

- `ConsistentHashRing` stores `std::map<uint64_t, NodeId> ring_` and `std::shared_mutex ring_mutex_`.
- `addNode(id, weight)`: for each vnode index `i` in `[0, weight*150)`, computes `hash(id + ":" + i)` and inserts into `ring_`.
- `getNode(key)`: computes `h = MurmurHash3_32(key)`, calls `ring_.lower_bound(h)`, wraps around to `ring_.begin()` if at end (ring wrap).
- Virtual node count per physical node is proportional to the node's declared capacity weight (higher-capacity nodes get more vnodes).
- Ring state is serialised to JSON and gossiped to all peers on every membership change; each node maintains a local replica of the full ring.
- Read operations on the ring use `shared_lock`; membership changes use `unique_lock`.

### Deviations & Rationale

- **No jump consistent hash**: Jump consistent hash (Google, 2014) provides better uniformity but does not support arbitrary node removal (only removal of the highest-numbered node). ThemisDB requires arbitrary node failure handling, so the Karger ring approach is used.
- **Weight-based vnodes instead of fixed 150**: The original Karger paper uses a fixed number of random positions. ThemisDB scales vnode count by node weight to give higher-capacity nodes proportionally more load.
- **MurmurHash3 instead of MD5**: Karger's original paper used MD5 for hash ring positions. ThemisDB uses MurmurHash3-32 (non-cryptographic) for speed; the distribution quality is equivalent for this purpose.

## ⚠️ Trade-offs & Limitations

- **Memory overhead for large clusters**: A 100-node cluster with weight=150 vnodes each requires 15,000 ring entries (~120 KB). For the expected cluster size (≤50 nodes), this is negligible.
- **Hotspot risk with low vnode count**: With too few vnodes, hash ring distribution can be uneven (some nodes get 2–3× the load of others). The weight×150 formula is empirically validated to keep load imbalance under 10% for ≥10 nodes.
- **Session affinity breaks on node failure**: When a backend node is removed, its sessions are migrated to the successor node. Clients that had stateful WebSocket connections must reconnect. This is mitigated by connection resumption protocol (replay of last N events).
- **Ring gossip convergence latency**: Membership changes take up to `gossip_interval * fanout` time to propagate. During this window, different gateway nodes may route the same key to different backends.

## 🔬 Validation

- [x] Code reviewed against Karger et al. (1997) algorithm description
- [x] Unit tests in `tests/server/consistent_hash_ring_test.cpp` verify distribution uniformity, minimal remapping on add/remove
- [x] Integration test simulates node failure and verifies session migration
- [x] Distribution uniformity benchmark confirms <10% variance across 10-node cluster with 150 vnodes
- [x] Module README linked (`src/server/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [MurmurHash3 Deterministic Sharding](murmur_hash_deterministic_sharding.md)
- [Exponential Backoff Retry](exponential_backoff_retry.md)

---
**Last Updated:** 2026-04-06
