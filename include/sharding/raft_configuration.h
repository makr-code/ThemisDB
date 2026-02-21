/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_configuration.h                               ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
