/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_log.cpp                                       ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     136                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_log.h"
#include <algorithm>

namespace themisdb {
namespace sharding {

RaftLog::RaftLog() : commit_index_(0) {}

uint64_t RaftLog::append(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_[entry.index] = entry;
    return entry.index;
}

std::optional<LogEntry> RaftLog::getEntry(uint64_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = log_.find(index);
    if (it != log_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<LogEntry> RaftLog::getEntries(uint64_t start_index, uint64_t end_index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LogEntry> entries;
    
    for (uint64_t i = start_index; i <= end_index; ++i) {
        auto it = log_.find(i);
        if (it != log_.end()) {
            entries.push_back(it->second);
        } else {
            break;  // Stop at first missing entry
        }
    }
    
    return entries;
}

bool RaftLog::hasEntry(uint64_t index, uint64_t term) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Special case: index 0 always matches (no previous entry)
    if (index == 0) {
        return true;
    }
    
    auto it = log_.find(index);
    if (it == log_.end()) {
        return false;
    }
    
    return it->second.term == term;
}

void RaftLog::truncateFrom(uint64_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Erase all entries from index onward
    auto it = log_.lower_bound(index);
    log_.erase(it, log_.end());
    
    // If we truncated past the commit index, adjust it
    if (commit_index_ >= index) {
        commit_index_ = (index > 0) ? index - 1 : 0;
    }
}

void RaftLog::setCommitIndex(uint64_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    commit_index_ = index;
}

uint64_t RaftLog::getCommitIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return commit_index_;
}

uint64_t RaftLog::getLastLogIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_.empty()) {
        return 0;
    }
    return log_.rbegin()->first;
}

uint64_t RaftLog::getLastLogTerm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_.empty()) {
        return 0;
    }
    return log_.rbegin()->second.term;
}

size_t RaftLog::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_.size();
}

void RaftLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    log_.clear();
    commit_index_ = 0;
}

}  // namespace sharding
}  // namespace themisdb
