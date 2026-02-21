/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_configuration.h                               ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
