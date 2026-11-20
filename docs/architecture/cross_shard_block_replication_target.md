---
Author: "ThemisDB Contributors"
Created: "2026-11-20"
Last Updated: "2026-11-20"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Target: Decentralized Cross-Shard Block Transactions

## Status and scope

This document selects a **target architecture for review**. It is not evidence that these
guarantees are implemented or production-ready, and acceptance requires maintainer review.
The decisions apply independently to each object version, such as a small file or a large
file spanning shards. They do not change the semantics of existing writes or promise
atomicity across multiple objects.

The [Raft-per-shard guide](../en/sharding/RAFT_CONSENSUS_PER_SHARD.md) describes shard
replication and the [RAID architecture](../en/sharding/RAID_SHARD_REFERENCING_ARCHITECTURE.md)
describes RAID-style placement and cross-shard transactions. The
[replication roadmap](../../src/replication/ROADMAP.md) tracks unresolved integration
evidence. [DIS-05 PR #8295](https://github.com/makr-code/ThemisDB/pull/8295) proposes
consolidating the named CrossShard 2PC path; it does not establish this Raft-backed
decision protocol or cross-coordinator recovery.

## Architecture decisions

### Units, placement and replication

- Split the immutable bytes of an object version into fixed blocks of at most 256 KiB;
  the last block may be smaller. Identify each block by SHA-256 and verify its bytes on
  receipt and read. A versioned manifest records object and version IDs, ordered block
  hashes, lengths, participant shard IDs and a placement-map epoch. Hashes provide
  integrity and identity, not proof of durability. The block size is an initial target,
  not a measured optimum; changing it requires a versioned format and benchmarks.
  Store blocks scoped to object version and ordinal; cross-version deduplication is not
  part of this target, so reclaiming one version cannot delete another version's bytes.
- A shard is a logical partition backed by a Raft group. The default profile uses three
  replicas on independent nodes in three failure domains, with a majority of two required
  for durable commit. This tolerates one replica failure per group, provided its remaining
  quorum and the other required groups remain available. No silent downgrade to one copy
  is permitted. A RAID mirror across separate shards is not the same as the replicas of a
  shard; this profile uses shard replication and does not add a second cross-shard mirror.
- Place each block on one logical shard using rendezvous hashing of its object ID and block
  ordinal against the shard set in the pinned placement-map epoch. The manifest records
  the selected shard, so reads do not depend on recalculating placement after rebalancing.
  Changes to placement and shard-group membership require explicit, recoverable migration.
- Carry block bytes in bounded Raft log entries for the responsible shard. A replica's
  Raft acknowledgement counts only after the entry is durable on stable storage. A
  majority-committed log entry establishes the required durable byte copies; an HTTP,
  TCP, packet or asynchronous shipper acceptance alone does not. Log compaction must not
  discard the last recoverable copies: durable snapshots or storage must retain the bytes
  and transaction state needed by the supported recovery procedure.
- Network packets are transport fragments, not transaction, durability or commit units.
  Transfers may stream and batch block entries with bounded buffering and backpressure.
  Shard-to-shard and client-to-shard traffic must use authenticated, encrypted transport.

### Parallel client transfer and small-file batches

- After the home group has recorded the transaction and its pinned placement, clients may
  upload different blocks concurrently to the responsible shard leaders. Clients obtain
  authenticated routing information and follow validated leader redirects; they do not
  directly write follower replicas or bypass the shard's Raft replication. Block receipt
  is only transfer progress, not a durable `PREPARED`, transaction commit or client success.
- After obtaining the published manifest and reader pin, clients may fetch its blocks in
  parallel from authorized replicas able to serve that exact committed version. Verify
  each block's hash and length before assembling it in manifest order. A stale, missing
  or corrupt response fails that block; retry another eligible replica without mixing
  versions. Release the pin only after all parallel reads have finished or been cancelled.
- For small files, clients may group uploads or reads into bounded **per-destination
  transport batches**. A batch has per-item object/version IDs, transaction or reader-pin
  identity, block ordinal, length and hash. Batching may combine independent files but
  does not create a multi-object transaction: each file retains its own home group,
  authorization, 2PC decision, manifest, publication and outcome.
- Return per-item results; a successful batch transport does not imply that every item
  succeeded or that any file committed. On partial failure or lost response, retry or
  query each item using its original idempotency identity. Only the per-file outcome
  contract can report publication success; a timeout remains unknown.
- Enforce authentication and authorization for each item and pin, including when a batch
  contains multiple tenants; reject unauthorized items without exposing their data.
  Splitting files across instances does not itself provide confidentiality. Use
  authenticated encrypted connections and apply the deployment's encryption-at-rest and
  key-management policy.
- Bound batch item count, total bytes, flush delay and parallel in-flight work, with
  backpressure and per-tenant fairness. Tune these limits from measured throughput and
  tail latency; a batch must not wait indefinitely to fill, and small or failed items
  must not indefinitely block unrelated files.

### Decentralized coordinator and decision

- Use **2PC with a Raft-replicated decision**, not 3PC, for this object-version path.
  Existing 3PC APIs are unaffected; they are not covered by this target or claimed
  non-blocking under arbitrary network partitions.
- The object's stable metadata/home shard is selected by rendezvous hashing the tenant
  and object ID against a versioned shard map when the object is created. Its Raft group
  owns the transaction decision and the published manifest pointer. Home assignments
  remain stable across object versions; moving a home requires an explicit migration with
  fencing and preserved decisions. Different objects can have different home groups:
  there is no single global coordinator group or leader.
- The durable object locator includes the tenant, object ID, home shard ID and creation
  map epoch. Name-to-locator lookup is outside this contract; clients route transactions
  and reads using the locator. A home migration must install a durable forwarding record
  at the old home and fence the old owner before the new owner accepts work, preserving
  redirects and outcome lookup for existing locators.
- The current home-group leader acts as coordinator. Its replicated state machine commits
  the transaction ID, object/version IDs, expected previous version, participant list,
  placement epoch and one immutable `COMMIT` or `ABORT` decision. Raft term/leadership
  fencing and state-machine validation prevent stale leaders or duplicate requests from
  producing conflicting decisions. A proposal alone is not a committed decision.
- The home group participates in the transaction and durably reserves the object against
  concurrent version updates during prepare. The reservation and expected-version check
  prevent a conflicting publication after a commit decision. The reservation survives
  leader changes and is released only after safe completion of commit or abort.
- Membership changes use Raft's safe configuration-change protocol. Placement changes do
  not reinterpret an in-flight transaction: its pinned participants and epoch remain
  authoritative until it resolves. Losing any required group quorum favors safety over
  write availability; neither Raft nor replicated 2PC removes this limitation.

## Transaction and visibility contract

1. The client supplies a unique transaction/idempotency ID, object ID and expected prior
   version. The home group durably records the transaction before prepare. Reuse of the ID
   with different inputs is rejected. Blocks and their manifest are staged but not exposed
   as the current object version.
2. Each required shard validates its assigned block bytes and logs the bytes and prepared
   state through its Raft group. It votes `PREPARED` only once the selected durability
   profile is satisfied and it can recover the staged work after a leader change. All
   required participants, including the home shard, must vote yes; a majority of shard
   groups is not sufficient.
3. Only after all yes votes does the home group durably commit `COMMIT` by Raft quorum.
   Otherwise it may decide `ABORT` while no commit exists; it must durably commit the
   abort decision before dispatching it. Timeouts do not authorize a participant or an
   isolated former leader to invent a conflicting decision.
4. The home leader delivers the recorded decision to all participants. Participants
   durably apply it through their own Raft groups and handle duplicate delivery
   idempotently. A new home leader re-drives unresolved deliveries from the authoritative
   decision. A committed decision is never converted to abort after a lost ACK.
5. After all required participants acknowledge durable commit, the home group atomically
   publishes the immutable manifest pointer in a quorum-committed state-machine update.
   To read, the client atomically obtains the current pointer and a durable reader pin
   through the home group, then reads and verifies the manifest's blocks. It releases the
   pin after reading. Readers see the old complete version or the new complete version,
   not a partially published version; a pin lets an existing read finish on an old version.
6. The client receives success only after durable publication. A timeout or lost response
   is **unknown**, not proof of abort: the client queries or retries by object ID and
   transaction ID. The home group provides durable outcome lookup distinguishing in
   progress, committed but awaiting publication, published and aborted. Transaction IDs
   are not reused; durable outcome records or tombstones must prevent a retry from becoming
   a new transaction. Retention and compaction must preserve that invariant.

The home group retains enough committed transaction and manifest metadata to finish
publication after failover. Participants retain staged or committed bytes while referenced
by an unresolved transaction or retained manifest. Garbage collection uses durable
decision and reference information; a timeout alone never deletes in-doubt data. Old
versions cannot be reclaimed while a durable reader pin exists. The home state machine
serializes pin creation, publication and reclamation eligibility, and only then issues
idempotent deletion commands to the block shards. Crashed clients can leave pins behind:
reclaiming them requires explicit revocation that fences new reads and drains outstanding
reads before the pin is removed, not an unverified timeout. If that cannot be established,
retain the pin and bytes. This favors safety over automatic space reclamation and adds a
quorum write to reads.

## Migration and operational boundaries

- This is a new, explicitly enabled path with versioned records and APIs. Do not silently
  reinterpret or fall back between coordinator-specific WALs. Drain or recover old in-flight
  transactions using their original coordinator before migrating an object; verify block
  data and manifests before publishing the new representation.
- Rollback requires draining new-format in-flight transactions and an explicitly tested
  compatible representation. If these conditions are not met, fail closed rather than
  discard decisions or pretend an old coordinator can replay the new log.
- A client timeout is not a safe automatic abort. Recovery can continue only when the
  home decision group and required participant groups regain sufficient quorum. Publish
  topology-specific availability and durability expectations rather than claiming
  availability through arbitrary partitions.

## Acceptance evidence

Demonstrate the contract on the real configured network and persistent storage path,
not only mocks or in-process callbacks:

- In a three-replica, independent-failure-domain setup, prove `PREPARED` follows durable
  byte and prepared-state replication to two replicas, and prove no success or unsafe
  downgrade on quorum loss. Test data recovery after one replica failure and log compaction.
- Inject failures before and after the home-group decision reaches quorum, during phase-two
  delivery, during publication and after a client timeout. Restart nodes and elect leaders;
  prove one decision, idempotent replay, outcome lookup and no commit-to-abort reversal.
- Verify corrupted, missing, reordered and duplicate blocks and interrupted transfers cannot
  publish a partial version. Concurrent readers observe only complete versions, including
  while competing writes, old-version retention and garbage collection are exercised.
- Test fencing, membership and placement migration with in-flight work, interrupted upgrades
  and supported rollback. Verify old coordinator WALs are not silently reinterpreted.
- Exercise parallel uploads and pinned reads across real shard leaders and replicas,
  including leader changes, stale or corrupt responses and retries. Verify no partial or
  cross-version file is exposed and no receipt is reported as a commit.
- Exercise mixed-success small-file batches, lost batch replies, retries, authorization
  failures and tenant isolation. Verify independent per-file outcomes and bounded queues.
  Benchmark single-file versus batched and parallel transfers for small and large files,
  reporting throughput, p95/p99 latency, resource use and the tested limits.

Record the exact topology, failure domains, durability configuration, transport, commands
and results. Release claims require this evidence and maintainer acceptance.
