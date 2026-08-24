// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_dk_diagnostics_focused.cpp
 * @brief Focused unit tests for DistributedKnowledgeDiagnosticEmitter (DKD-01..DKD-14).
 *
 * All tests are fully self-contained: no network I/O, no filesystem I/O.
 * Diagnostic listeners are implemented inline as recording stubs.
 *
 * ## Test families
 *
 * ### DKD-01..04 — Emitter lifecycle
 *   DKD-01  Empty emitter: emit() with no listeners does not throw
 *   DKD-02  addListener(nullptr) is a no-op
 *   DKD-03  clearListeners() removes all listeners
 *   DKD-04  listenerCount() reflects add and clear operations
 *
 * ### DKD-05..08 — Event dispatch
 *   DKD-05  Single listener receives emitted event
 *   DKD-06  Multiple listeners receive the same event
 *   DKD-07  Throwing listener does not prevent subsequent listeners from receiving event
 *   DKD-08  timestamp_utc is auto-filled when empty
 *
 * ### DKD-09..11 — Convenience overloads
 *   DKD-09  emitMergeTimeout populates correct fields
 *   DKD-10  emitDedupCollision populates correct fields and metadata
 *   DKD-11  emitTrustGateReject populates correct fields with ERROR severity
 *   DKD-12  emitPartialShardMerge populates correct fields and metadata
 *   DKD-13  emitFederationRollback populates correct fields
 *
 * ### DKD-14 — Policy types
 *   DKD-14  MergeHardeningPolicy isConstrained() semantics
 *
 * @see include/distributed_knowledge/dk_diagnostic_emitter.h
 * @see include/distributed_knowledge/distributed_knowledge_api_contract.h
 * @see src/distributed_knowledge/ROADMAP.md — Phase 3 / Q4 2026
 */

#include <gtest/gtest.h>

#include "distributed_knowledge/dk_diagnostic_emitter.h"

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::distributed_knowledge;

// ─────────────────────────────────────────────────────────────────────────────
// Recording listener
// ─────────────────────────────────────────────────────────────────────────────

class RecordingListener final : public IDKDiagnosticListener {
public:
    void onEvent(const DKDiagnosticEvent& event) override {
        events.push_back(event);
    }
    std::vector<DKDiagnosticEvent> events;
};

/// A listener that always throws.
class ThrowingListener final : public IDKDiagnosticListener {
public:
    void onEvent(const DKDiagnosticEvent&) override {
        throw std::runtime_error("listener intentionally throws");
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DKD-01..04 — Emitter lifecycle
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test DKD-01: emit() with no listeners does not throw.
 */
TEST(DKDiagnosticsEmitter, EmitWithNoListenersDoesNotThrow) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    DKDiagnosticEvent ev;
    ev.type  = DKDiagnosticEventType::MERGE_TIMEOUT;
    ev.cause = "test";
    EXPECT_NO_THROW(emitter.emit(ev));
}

/**
 * @test DKD-02: addListener(nullptr) is silently ignored.
 */
TEST(DKDiagnosticsEmitter, AddNullListenerIsNoOp) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    emitter.addListener(nullptr);
    EXPECT_EQ(emitter.listenerCount(), 0u);
}

/**
 * @test DKD-03: clearListeners() removes all registered listeners.
 */
TEST(DKDiagnosticsEmitter, ClearListenersRemovesAll) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    emitter.addListener(std::make_shared<RecordingListener>());
    emitter.addListener(std::make_shared<RecordingListener>());
    EXPECT_EQ(emitter.listenerCount(), 2u);
    emitter.clearListeners();
    EXPECT_EQ(emitter.listenerCount(), 0u);
}

/**
 * @test DKD-04: listenerCount() reflects add and clear operations.
 */
TEST(DKDiagnosticsEmitter, ListenerCountIsAccurate) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    EXPECT_EQ(emitter.listenerCount(), 0u);
    emitter.addListener(std::make_shared<RecordingListener>());
    EXPECT_EQ(emitter.listenerCount(), 1u);
    emitter.addListener(std::make_shared<RecordingListener>());
    EXPECT_EQ(emitter.listenerCount(), 2u);
    emitter.clearListeners();
    EXPECT_EQ(emitter.listenerCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// DKD-05..08 — Event dispatch
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test DKD-05: A single registered listener receives the emitted event.
 */
TEST(DKDiagnosticsEmitter, SingleListenerReceivesEvent) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    auto rec = std::make_shared<RecordingListener>();
    emitter.addListener(rec);

    DKDiagnosticEvent ev;
    ev.type         = DKDiagnosticEventType::DEDUP_COLLISION;
    ev.severity     = DKDiagnosticSeverity::INFO;
    ev.shard_id     = "shard-01";
    ev.operation_id = "op-001";
    ev.cause        = "duplicate doc_id";
    emitter.emit(ev);

    ASSERT_EQ(rec->events.size(), 1u);
    EXPECT_EQ(rec->events[0].type, DKDiagnosticEventType::DEDUP_COLLISION);
    EXPECT_EQ(rec->events[0].shard_id, "shard-01");
    EXPECT_EQ(rec->events[0].cause, "duplicate doc_id");
}

/**
 * @test DKD-06: Multiple listeners all receive the same event.
 */
TEST(DKDiagnosticsEmitter, MultipleListenersAllReceiveEvent) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    auto rec1 = std::make_shared<RecordingListener>();
    auto rec2 = std::make_shared<RecordingListener>();
    emitter.addListener(rec1);
    emitter.addListener(rec2);

    DKDiagnosticEvent ev;
    ev.type     = DKDiagnosticEventType::TRUST_GATE_REJECT;
    ev.severity = DKDiagnosticSeverity::ERROR;
    ev.cause    = "untrusted domain";
    emitter.emit(ev);

    ASSERT_EQ(rec1->events.size(), 1u);
    ASSERT_EQ(rec2->events.size(), 1u);
    EXPECT_EQ(rec1->events[0].type, DKDiagnosticEventType::TRUST_GATE_REJECT);
    EXPECT_EQ(rec2->events[0].type, DKDiagnosticEventType::TRUST_GATE_REJECT);
}

/**
 * @test DKD-07: A throwing listener does not prevent subsequent listeners from
 *               receiving the event.
 */
TEST(DKDiagnosticsEmitter, ThrowingListenerDoesNotBlockOthers) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    auto thrower = std::make_shared<ThrowingListener>();
    auto rec     = std::make_shared<RecordingListener>();
    emitter.addListener(thrower);  // registered first
    emitter.addListener(rec);

    DKDiagnosticEvent ev;
    ev.type  = DKDiagnosticEventType::MERGE_TIMEOUT;
    ev.cause = "slow shard";
    EXPECT_NO_THROW(emitter.emit(ev));

    // Despite the throwing listener, rec must have received the event.
    ASSERT_EQ(rec->events.size(), 1u);
    EXPECT_EQ(rec->events[0].type, DKDiagnosticEventType::MERGE_TIMEOUT);
}

/**
 * @test DKD-08: timestamp_utc is populated automatically when left empty.
 */
TEST(DKDiagnosticsEmitter, TimestampAutoFilled) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    auto rec = std::make_shared<RecordingListener>();
    emitter.addListener(rec);

    DKDiagnosticEvent ev;
    ev.type         = DKDiagnosticEventType::FEDERATION_ROLLBACK;
    ev.timestamp_utc = "";  // intentionally empty
    emitter.emit(ev);

    ASSERT_EQ(rec->events.size(), 1u);
    EXPECT_FALSE(rec->events[0].timestamp_utc.empty())
        << "timestamp_utc must be auto-filled by the emitter";
}

// ─────────────────────────────────────────────────────────────────────────────
// DKD-09..13 — Convenience overloads
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test DKD-09: emitMergeTimeout fills type, shard_id, operation_id, cause.
 */
TEST(DKDiagnosticsEmitter, EmitMergeTimeoutPopulatesFields) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    auto rec = std::make_shared<RecordingListener>();
    emitter.addListener(rec);

    emitter.emitMergeTimeout("shard-us", "op-42", "exceeded 500 ms");

    ASSERT_EQ(rec->events.size(), 1u);
    const auto& ev = rec->events[0];
    EXPECT_EQ(ev.type,         DKDiagnosticEventType::MERGE_TIMEOUT);
    EXPECT_EQ(ev.severity,     DKDiagnosticSeverity::WARNING);
    EXPECT_EQ(ev.shard_id,     "shard-us");
    EXPECT_EQ(ev.operation_id, "op-42");
    EXPECT_EQ(ev.cause,        "exceeded 500 ms");
}

/**
 * @test DKD-10: emitDedupCollision populates fields and metadata.key.
 */
TEST(DKDiagnosticsEmitter, EmitDedupCollisionPopulatesFieldsAndMetadata) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    auto rec = std::make_shared<RecordingListener>();
    emitter.addListener(rec);

    emitter.emitDedupCollision("shard-eu", "op-99", "doc-dup-007");

    ASSERT_EQ(rec->events.size(), 1u);
    const auto& ev = rec->events[0];
    EXPECT_EQ(ev.type,           DKDiagnosticEventType::DEDUP_COLLISION);
    EXPECT_EQ(ev.severity,       DKDiagnosticSeverity::INFO);
    EXPECT_EQ(ev.shard_id,       "shard-eu");
    EXPECT_EQ(ev.operation_id,   "op-99");
    ASSERT_NE(ev.metadata.count("key"), 0u);
    EXPECT_EQ(ev.metadata.at("key"), "doc-dup-007");
}

/**
 * @test DKD-11: emitTrustGateReject sets severity to ERROR.
 */
TEST(DKDiagnosticsEmitter, EmitTrustGateRejectHasErrorSeverity) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    auto rec = std::make_shared<RecordingListener>();
    emitter.addListener(rec);

    emitter.emitTrustGateReject("shard-x", "fed-op-1", "domain not in allowlist");

    ASSERT_EQ(rec->events.size(), 1u);
    EXPECT_EQ(rec->events[0].type,     DKDiagnosticEventType::TRUST_GATE_REJECT);
    EXPECT_EQ(rec->events[0].severity, DKDiagnosticSeverity::ERROR);
    EXPECT_EQ(rec->events[0].shard_id, "shard-x");
}

/**
 * @test DKD-12: emitPartialShardMerge populates responding/total shard metadata.
 */
TEST(DKDiagnosticsEmitter, EmitPartialShardMergePopulatesMetadata) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    auto rec = std::make_shared<RecordingListener>();
    emitter.addListener(rec);

    emitter.emitPartialShardMerge("merge-op-5", 3u, 5u);

    ASSERT_EQ(rec->events.size(), 1u);
    const auto& ev = rec->events[0];
    EXPECT_EQ(ev.type, DKDiagnosticEventType::PARTIAL_SHARD_MERGE);
    EXPECT_EQ(ev.metadata.at("responding_shards"), "3");
    EXPECT_EQ(ev.metadata.at("total_shards"),       "5");
}

/**
 * @test DKD-13: emitFederationRollback populates correct fields.
 */
TEST(DKDiagnosticsEmitter, EmitFederationRollbackPopulatesFields) {
    DistributedKnowledgeDiagnosticEmitter emitter;
    auto rec = std::make_shared<RecordingListener>();
    emitter.addListener(rec);

    emitter.emitFederationRollback("round-007", "quorum lost");

    ASSERT_EQ(rec->events.size(), 1u);
    EXPECT_EQ(rec->events[0].type,         DKDiagnosticEventType::FEDERATION_ROLLBACK);
    EXPECT_EQ(rec->events[0].severity,     DKDiagnosticSeverity::ERROR);
    EXPECT_EQ(rec->events[0].operation_id, "round-007");
    EXPECT_EQ(rec->events[0].cause,        "quorum lost");
}

// ─────────────────────────────────────────────────────────────────────────────
// DKD-14 — Policy type semantics
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test DKD-14: MergeHardeningPolicy::isConstrained() semantics.
 *
 * Verifies all four constraint scenarios:
 *  - Unconstrained policy (all zeros, BEST_EFFORT) → false
 *  - Constrained by max_merge_latency_ms → true
 *  - Constrained by dedup_window_size → true
 *  - Constrained by FAIL_CLOSED timeout_behavior → true
 */
TEST(MergeHardeningPolicy, IsConstrainedSemantics) {
    // Unconstrained
    MergeHardeningPolicy unconstrained;
    EXPECT_FALSE(unconstrained.isConstrained());

    // Constrained by latency
    MergeHardeningPolicy latency;
    latency.max_merge_latency_ms = 500u;
    EXPECT_TRUE(latency.isConstrained());

    // Constrained by dedup window
    MergeHardeningPolicy dedup;
    dedup.dedup_window_size = 1000u;
    EXPECT_TRUE(dedup.isConstrained());

    // Constrained by FAIL_CLOSED mode
    MergeHardeningPolicy failClosed;
    failClosed.timeout_behavior = TimeoutBehavior::FAIL_CLOSED;
    EXPECT_TRUE(failClosed.isConstrained());

    // Constrained by min shard count
    MergeHardeningPolicy minShard;
    minShard.partial_shard_min_count = 3u;
    EXPECT_TRUE(minShard.isConstrained());
}
