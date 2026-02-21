/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_configuration.h                               ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:58:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     73                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 5d03090e8  2025-12-08  Implement P1.2 Raft - Part 3: Membership Changes ✅ ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <set>
#include <string>
#include <mutex>

namespace themis {
namespace sharding {

struct ConfigurationEntry {
    std::set<std::string> old_members;  // Empty if not joint consensus
    std::set<std::string> new_members;
    bool is_joint_consensus;
};

class RaftConfiguration {
public:
    RaftConfiguration();
    explicit RaftConfiguration(const std::set<std::string>& members);
    
    // Membership changes
    void addNode(const std::string& node_id);
    void removeNode(const std::string& node_id);
    void applyConfiguration(const ConfigurationEntry& entry);
    
    // State queries
    bool isInTransition() const;
    bool isJointConsensus() const;
    bool isMember(const std::string& node_id) const;
    std::set<std::string> getMembers() const;
    std::set<std::string> getOldMembers() const;
    std::set<std::string> getNewMembers() const;
    
    // Quorum calculation
    bool hasQuorum(const std::set<std::string>& votes) const;
    size_t getQuorumSize() const;
    
private:
    std::set<std::string> old_members_;
    std::set<std::string> new_members_;
    bool is_joint_consensus_;
    mutable std::mutex mutex_;
    
    size_t calculateQuorum(size_t size) const;
};

}  // namespace sharding
}  // namespace themis
