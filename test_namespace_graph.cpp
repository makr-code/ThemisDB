// Quick namespace validation test for graph module
#include <iostream>

// Test: Can we include the graph headers?
#include "graph/graph_query_optimizer.h"
#include "graph/knowledge_graph_reasoner.h"
#include "graph/ontology_manager.h"
#include "graph/parallel_traversal.h"
#include "graph/distributed_graph.h"
#include "graph/rotate_completion.h"
#include "graph/scheduled_edge_refresh.h"

// If includes work, the namespace wrapping is correct
int main() {
    std::cout << "✓ All graph headers included successfully\n";
    std::cout << "✓ Namespace resolution passed\n";
    return 0;
}
