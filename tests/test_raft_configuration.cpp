#include "sharding/raft_configuration.h"
#include <cassert>
#include <iostream>

using namespace themis::sharding;

void test_initial_configuration() {
    std::set<std::string> members = {"A", "B", "C"};
    RaftConfiguration config(members);
    
    assert(config.getMembers() == members);
    assert(!config.isJointConsensus());
    assert(config.isMember("A"));
    assert(config.isMember("B"));
    assert(config.isMember("C"));
    assert(!config.isMember("D"));
    
    std::cout << "✓ test_initial_configuration passed\n";
}

void test_add_node() {
    std::set<std::string> members = {"A", "B", "C"};
    RaftConfiguration config(members);
    
    config.addNode("D");
    
    assert(config.isJointConsensus());
    assert(config.isMember("D"));
    
    auto all_members = config.getMembers();
    assert(all_members.size() == 4);
    
    std::cout << "✓ test_add_node passed\n";
}

void test_remove_node() {
    std::set<std::string> members = {"A", "B", "C", "D"};
    RaftConfiguration config(members);
    
    config.removeNode("D");
    
    assert(config.isJointConsensus());
    
    auto new_members = config.getNewMembers();
    assert(new_members.size() == 3);
    assert(new_members.count("D") == 0);
    
    std::cout << "✓ test_remove_node passed\n";
}

void test_joint_consensus_quorum() {
    std::set<std::string> members = {"A", "B", "C"};
    RaftConfiguration config(members);
    
    // Add two nodes: {A,B,C} → {A,B,C,D,E}
    config.addNode("D");
    ConfigurationEntry entry{{}, {"A", "B", "C", "D", "E"}, true};
    entry.old_members = {"A", "B", "C"};
    config.applyConfiguration(entry);
    
    // Need 2/3 from old AND 3/5 from new
    assert(!config.hasQuorum({"A", "B"}));  // 2/3 old ✓, 2/5 new ✗
    assert(config.hasQuorum({"A", "B", "C"}));  // 3/3 old ✓, 3/5 new ✓
    assert(config.hasQuorum({"A", "B", "C", "D"}));  // 3/3 old ✓, 4/5 new ✓
    
    std::cout << "✓ test_joint_consensus_quorum passed\n";
}

void test_quorum_calculation() {
    // 3 nodes → quorum = 2
    RaftConfiguration config3({"A", "B", "C"});
    assert(config3.getQuorumSize() == 2);
    assert(config3.hasQuorum({"A", "B"}));
    assert(!config3.hasQuorum({"A"}));
    
    // 5 nodes → quorum = 3
    RaftConfiguration config5({"A", "B", "C", "D", "E"});
    assert(config5.getQuorumSize() == 3);
    assert(config5.hasQuorum({"A", "B", "C"}));
    assert(!config5.hasQuorum({"A", "B"}));
    
    std::cout << "✓ test_quorum_calculation passed\n";
}

void test_apply_configuration() {
    RaftConfiguration config;
    
    ConfigurationEntry entry{{}, {"A", "B", "C"}, false};
    config.applyConfiguration(entry);
    
    assert(config.getMembers().size() == 3);
    assert(!config.isJointConsensus());
    
    std::cout << "✓ test_apply_configuration passed\n";
}

void test_concurrent_changes_blocked() {
    std::set<std::string> members = {"A", "B", "C"};
    RaftConfiguration config(members);
    
    config.addNode("D");
    
    try {
        config.addNode("E");
        assert(false);  // Should have thrown
    } catch (const std::runtime_error&) {
        // Expected
    }
    
    std::cout << "✓ test_concurrent_changes_blocked passed\n";
}

void test_two_phase_transition() {
    std::set<std::string> members = {"A", "B", "C"};
    RaftConfiguration config(members);
    
    // Phase 1: C_old,new
    ConfigurationEntry joint{{"A", "B", "C"}, {"A", "B", "C", "D"}, true};
    config.applyConfiguration(joint);
    
    assert(config.isJointConsensus());
    assert(config.isMember("D"));
    
    // Phase 2: C_new
    ConfigurationEntry new_config{{}, {"A", "B", "C", "D"}, false};
    config.applyConfiguration(new_config);
    
    assert(!config.isJointConsensus());
    assert(config.getMembers().size() == 4);
    
    std::cout << "✓ test_two_phase_transition passed\n";
}

int main() {
    test_initial_configuration();
    test_add_node();
    test_remove_node();
    test_joint_consensus_quorum();
    test_quorum_calculation();
    test_apply_configuration();
    test_concurrent_changes_blocked();
    test_two_phase_transition();
    
    std::cout << "\n✅ All Raft configuration tests passed!\n";
    return 0;
}
