// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_log.h"
#include <cassert>
#include <iostream>

using namespace themisdb::sharding;

void test_append_single_entry() {
    RaftLog log;
    
    LogEntry entry(1, 1, "command1");
    uint64_t index = log.append(entry);
    
    assert(index == 1);
    assert(log.size() == 1);
    assert(log.getLastLogIndex() == 1);
    assert(log.getLastLogTerm() == 1);
    
    std::cout << "✓ test_append_single_entry passed\n";
}

void test_append_multiple_entries() {
    RaftLog log;
    
    log.append(LogEntry(1, 1, "cmd1"));
    log.append(LogEntry(1, 2, "cmd2"));
    log.append(LogEntry(2, 3, "cmd3"));
    
    assert(log.size() == 3);
    assert(log.getLastLogIndex() == 3);
    assert(log.getLastLogTerm() == 2);
    
    std::cout << "✓ test_append_multiple_entries passed\n";
}

void test_get_entry() {
    RaftLog log;
    
    log.append(LogEntry(1, 1, "cmd1"));
    log.append(LogEntry(1, 2, "cmd2"));
    
    auto entry = log.getEntry(1);
    assert(entry.has_value());
    assert(entry->index == 1);
    assert(entry->term == 1);
    assert(entry->command == "cmd1");
    
    auto missing = log.getEntry(99);
    assert(!missing.has_value());
    
    std::cout << "✓ test_get_entry passed\n";
}

void test_get_entries_range() {
    RaftLog log;
    
    log.append(LogEntry(1, 1, "cmd1"));
    log.append(LogEntry(1, 2, "cmd2"));
    log.append(LogEntry(2, 3, "cmd3"));
    log.append(LogEntry(2, 4, "cmd4"));
    
    auto entries = log.getEntries(2, 3);
    assert(entries.size() == 2);
    assert(entries[0].index == 2);
    assert(entries[1].index == 3);
    
    std::cout << "✓ test_get_entries_range passed\n";
}

void test_has_entry() {
    RaftLog log;
    
    log.append(LogEntry(1, 1, "cmd1"));
    log.append(LogEntry(2, 2, "cmd2"));
    
    // Index 0 always matches (no previous entry)
    assert(log.hasEntry(0, 0));
    
    // Existing entries
    assert(log.hasEntry(1, 1));
    assert(log.hasEntry(2, 2));
    
    // Wrong term
    assert(!log.hasEntry(1, 2));
    
    // Non-existent index
    assert(!log.hasEntry(99, 1));
    
    std::cout << "✓ test_has_entry passed\n";
}

void test_truncate_from() {
    RaftLog log;
    
    log.append(LogEntry(1, 1, "cmd1"));
    log.append(LogEntry(1, 2, "cmd2"));
    log.append(LogEntry(2, 3, "cmd3"));
    log.append(LogEntry(2, 4, "cmd4"));
    
    log.truncateFrom(3);
    
    assert(log.size() == 2);
    assert(log.getLastLogIndex() == 2);
    assert(!log.getEntry(3).has_value());
    assert(!log.getEntry(4).has_value());
    
    std::cout << "✓ test_truncate_from passed\n";
}

void test_commit_index() {
    RaftLog log;
    
    assert(log.getCommitIndex() == 0);
    
    log.setCommitIndex(5);
    assert(log.getCommitIndex() == 5);
    
    log.setCommitIndex(10);
    assert(log.getCommitIndex() == 10);
    
    std::cout << "✓ test_commit_index passed\n";
}

void test_commit_index_adjusted_on_truncate() {
    RaftLog log;
    
    log.append(LogEntry(1, 1, "cmd1"));
    log.append(LogEntry(1, 2, "cmd2"));
    log.append(LogEntry(2, 3, "cmd3"));
    
    log.setCommitIndex(3);
    assert(log.getCommitIndex() == 3);
    
    // Truncate past commit index
    log.truncateFrom(2);
    
    // Commit index should be adjusted to index - 1
    assert(log.getCommitIndex() == 1);
    
    std::cout << "✓ test_commit_index_adjusted_on_truncate passed\n";
}

void test_append_entries_empty_heartbeat() {
    RaftLog log;
    
    AppendEntriesRequest req;
    req.term = 1;
    req.leader_id = "leader1";
    req.prev_log_index = 0;
    req.prev_log_term = 0;
    req.entries = {};  // Empty for heartbeat
    req.leader_commit = 0;
    
    // Simulate receiver logic
    assert(log.hasEntry(req.prev_log_index, req.prev_log_term));
    
    AppendEntriesResponse resp(1, true, 0);
    assert(resp.success);
    
    std::cout << "✓ test_append_entries_empty_heartbeat passed\n";
}

void test_append_entries_with_entries() {
    RaftLog log;
    
    // Initial log
    log.append(LogEntry(1, 1, "cmd1"));
    log.append(LogEntry(1, 2, "cmd2"));
    
    // AppendEntries with new entries
    AppendEntriesRequest req;
    req.term = 2;
    req.leader_id = "leader1";
    req.prev_log_index = 2;
    req.prev_log_term = 1;
    req.entries = {
        LogEntry(2, 3, "cmd3"),
        LogEntry(2, 4, "cmd4")
    };
    req.leader_commit = 4;
    
    // Receiver logic
    if (log.hasEntry(req.prev_log_index, req.prev_log_term)) {
        for (const auto& entry : req.entries) {
            log.append(entry);
        }
        log.setCommitIndex(std::min(req.leader_commit, log.getLastLogIndex()));
    }
    
    assert(log.size() == 4);
    assert(log.getLastLogIndex() == 4);
    assert(log.getCommitIndex() == 4);
    
    std::cout << "✓ test_append_entries_with_entries passed\n";
}

void test_append_entries_consistency_check_fail() {
    RaftLog log;
    
    log.append(LogEntry(1, 1, "cmd1"));
    
    // AppendEntries with mismatched prev entry
    AppendEntriesRequest req;
    req.term = 2;
    req.prev_log_index = 2;  // We don't have index 2
    req.prev_log_term = 1;
    req.entries = {LogEntry(2, 3, "cmd3")};
    
    // Should fail consistency check
    assert(!log.hasEntry(req.prev_log_index, req.prev_log_term));
    
    AppendEntriesResponse resp(1, false, 0);
    assert(!resp.success);
    
    std::cout << "✓ test_append_entries_consistency_check_fail passed\n";
}

void test_append_entries_with_conflict() {
    RaftLog log;
    
    // Follower log
    log.append(LogEntry(1, 1, "cmd1"));
    log.append(LogEntry(1, 2, "cmd2"));
    log.append(LogEntry(2, 3, "cmd3_old"));  // Conflicting entry
    log.append(LogEntry(2, 4, "cmd4_old"));
    
    // Leader wants to replace index 3 onward
    AppendEntriesRequest req;
    req.term = 3;
    req.prev_log_index = 2;
    req.prev_log_term = 1;
    req.entries = {
        LogEntry(3, 3, "cmd3_new"),
        LogEntry(3, 4, "cmd4_new")
    };
    
    // Receiver logic: Check consistency
    if (log.hasEntry(req.prev_log_index, req.prev_log_term)) {
        // Check for conflicts
        for (const auto& entry : req.entries) {
            auto existing = log.getEntry(entry.index);
            if (existing.has_value() && existing->term != entry.term) {
                // Conflict: truncate from this index
                log.truncateFrom(entry.index);
                break;
            }
        }
        
        // Append new entries
        for (const auto& entry : req.entries) {
            log.append(entry);
        }
    }
    
    assert(log.size() == 4);
    auto entry3 = log.getEntry(3);
    assert(entry3->command == "cmd3_new");
    auto entry4 = log.getEntry(4);
    assert(entry4->command == "cmd4_new");
    
    std::cout << "✓ test_append_entries_with_conflict passed\n";
}

void test_append_entries_advance_commit() {
    RaftLog log;
    
    log.append(LogEntry(1, 1, "cmd1"));
    log.append(LogEntry(1, 2, "cmd2"));
    log.append(LogEntry(2, 3, "cmd3"));
    
    log.setCommitIndex(2);
    
    // Leader has committed up to 3
    AppendEntriesRequest req;
    req.term = 2;
    req.prev_log_index = 3;
    req.prev_log_term = 2;
    req.entries = {};
    req.leader_commit = 3;
    
    // Advance commit index
    if (log.hasEntry(req.prev_log_index, req.prev_log_term)) {
        uint64_t new_commit = std::min(req.leader_commit, log.getLastLogIndex());
        log.setCommitIndex(new_commit);
    }
    
    assert(log.getCommitIndex() == 3);
    
    std::cout << "✓ test_append_entries_advance_commit passed\n";
}

void test_leader_replication_to_followers() {
    // Simulate leader replicating to two followers
    
    // Leader log
    RaftLog leader_log;
    leader_log.append(LogEntry(1, 1, "cmd1"));
    leader_log.append(LogEntry(1, 2, "cmd2"));
    leader_log.append(LogEntry(2, 3, "cmd3"));
    leader_log.setCommitIndex(3);
    
    // Follower 1 log (behind)
    RaftLog follower1_log;
    follower1_log.append(LogEntry(1, 1, "cmd1"));
    
    // Follower 2 log (empty)
    RaftLog follower2_log;
    
    // Replicate to follower 1
    {
        AppendEntriesRequest req;
        req.term = 2;
        req.prev_log_index = 1;
        req.prev_log_term = 1;
        req.entries = leader_log.getEntries(2, 3);
        req.leader_commit = 3;
        
        if (follower1_log.hasEntry(req.prev_log_index, req.prev_log_term)) {
            for (const auto& entry : req.entries) {
                follower1_log.append(entry);
            }
            follower1_log.setCommitIndex(std::min(req.leader_commit, follower1_log.getLastLogIndex()));
        }
    }
    
    // Replicate to follower 2
    {
        AppendEntriesRequest req;
        req.term = 2;
        req.prev_log_index = 0;
        req.prev_log_term = 0;
        req.entries = leader_log.getEntries(1, 3);
        req.leader_commit = 3;
        
        if (follower2_log.hasEntry(req.prev_log_index, req.prev_log_term)) {
            for (const auto& entry : req.entries) {
                follower2_log.append(entry);
            }
            follower2_log.setCommitIndex(std::min(req.leader_commit, follower2_log.getLastLogIndex()));
        }
    }
    
    // All logs should match
    assert(leader_log.size() == 3);
    assert(follower1_log.size() == 3);
    assert(follower2_log.size() == 3);
    
    assert(leader_log.getCommitIndex() == 3);
    assert(follower1_log.getCommitIndex() == 3);
    assert(follower2_log.getCommitIndex() == 3);
    
    std::cout << "✓ test_leader_replication_to_followers passed\n";
}

void test_follower_catchup_after_partition() {
    // Leader log (up to date)
    RaftLog leader_log;
    leader_log.append(LogEntry(1, 1, "cmd1"));
    leader_log.append(LogEntry(1, 2, "cmd2"));
    leader_log.append(LogEntry(2, 3, "cmd3"));
    leader_log.append(LogEntry(2, 4, "cmd4"));
    leader_log.append(LogEntry(3, 5, "cmd5"));
    leader_log.setCommitIndex(5);
    
    // Follower log (lagging, was partitioned)
    RaftLog follower_log;
    follower_log.append(LogEntry(1, 1, "cmd1"));
    follower_log.append(LogEntry(1, 2, "cmd2"));
    
    // Leader tries to replicate from index 3
    AppendEntriesRequest req;
    req.term = 3;
    req.prev_log_index = 2;
    req.prev_log_term = 1;
    req.entries = leader_log.getEntries(3, 5);
    req.leader_commit = 5;
    
    if (follower_log.hasEntry(req.prev_log_index, req.prev_log_term)) {
        for (const auto& entry : req.entries) {
            follower_log.append(entry);
        }
        follower_log.setCommitIndex(std::min(req.leader_commit, follower_log.getLastLogIndex()));
    }
    
    assert(follower_log.size() == 5);
    assert(follower_log.getCommitIndex() == 5);
    
    std::cout << "✓ test_follower_catchup_after_partition passed\n";
}

void test_out_of_order_append_entries() {
    RaftLog log;
    
    log.append(LogEntry(1, 1, "cmd1"));
    
    // Receive AppendEntries for index 3 before index 2
    AppendEntriesRequest req;
    req.term = 2;
    req.prev_log_index = 2;
    req.prev_log_term = 1;
    req.entries = {LogEntry(2, 3, "cmd3")};
    
    // Should fail because we don't have index 2
    assert(!log.hasEntry(req.prev_log_index, req.prev_log_term));
    
    // Now receive AppendEntries for index 2
    AppendEntriesRequest req2;
    req2.term = 2;
    req2.prev_log_index = 1;
    req2.prev_log_term = 1;
    req2.entries = {LogEntry(1, 2, "cmd2")};
    
    if (log.hasEntry(req2.prev_log_index, req2.prev_log_term)) {
        for (const auto& entry : req2.entries) {
            log.append(entry);
        }
    }
    
    // Now the first request should succeed
    if (log.hasEntry(req.prev_log_index, req.prev_log_term)) {
        for (const auto& entry : req.entries) {
            log.append(entry);
        }
    }
    
    assert(log.size() == 3);
    
    std::cout << "✓ test_out_of_order_append_entries passed\n";
}

void test_wal_integration_scenario() {
    // Simulate Raft log integrated with WAL
    RaftLog log;
    
    // Leader appends to Raft log (which wraps WAL entries)
    log.append(LogEntry(1, 1, "WAL:INSERT user1"));
    log.append(LogEntry(1, 2, "WAL:UPDATE user1"));
    log.append(LogEntry(2, 3, "WAL:DELETE user2"));
    
    // Leader commits index 2
    log.setCommitIndex(2);
    
    // Follower receives AppendEntries
    RaftLog follower_log;
    
    AppendEntriesRequest req;
    req.term = 2;
    req.prev_log_index = 0;
    req.prev_log_term = 0;
    req.entries = log.getEntries(1, 3);
    req.leader_commit = 2;
    
    if (follower_log.hasEntry(req.prev_log_index, req.prev_log_term)) {
        for (const auto& entry : req.entries) {
            follower_log.append(entry);
        }
        
        uint64_t old_commit = follower_log.getCommitIndex();
        uint64_t new_commit = std::min(req.leader_commit, follower_log.getLastLogIndex());
        follower_log.setCommitIndex(new_commit);
        
        // Apply committed entries to WAL
        for (uint64_t i = old_commit + 1; i <= new_commit; ++i) {
            auto entry = follower_log.getEntry(i);
            // In real implementation: wal_manager.append(entry->command)
            assert(entry.has_value());
        }
    }
    
    assert(follower_log.getCommitIndex() == 2);
    
    std::cout << "✓ test_wal_integration_scenario passed\n";
}

int main() {
    std::cout << "Running RaftLog tests...\n\n";
    
    test_append_single_entry();
    test_append_multiple_entries();
    test_get_entry();
    test_get_entries_range();
    test_has_entry();
    test_truncate_from();
    test_commit_index();
    test_commit_index_adjusted_on_truncate();
    
    test_append_entries_empty_heartbeat();
    test_append_entries_with_entries();
    test_append_entries_consistency_check_fail();
    test_append_entries_with_conflict();
    test_append_entries_advance_commit();
    
    test_leader_replication_to_followers();
    test_follower_catchup_after_partition();
    test_out_of_order_append_entries();
    test_wal_integration_scenario();
    
    std::cout << "\n✅ All RaftLog tests passed!\n";
    return 0;
}
