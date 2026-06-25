#!/usr/bin/env python3
"""Check namespace declarations in graph files."""

graph_files = [
    'src/graph/graph_query_optimizer.cpp',
    'src/graph/explain_plan.cpp',
    'src/graph/ontology_manager.cpp',
    'src/graph/knowledge_graph_reasoner.cpp',
    'src/graph/rotate_completion.cpp',
    'src/graph/path_constraints.cpp',
    'src/graph/distributed_graph.cpp',
    'src/graph/gpu_traversal.cpp',
    'src/graph/parallel_traversal.cpp',
    'src/graph/scheduled_edge_refresh.cpp',
    'src/graph/graph_query_rewriter.cpp',
]

for f in graph_files:
    with open(f, 'r', encoding='utf-8', errors='ignore') as fp:
        content = fp.read()
    
    has_themis_ns = 'namespace themis {' in content
    has_graph_ns = 'namespace graph {' in content
    fname = f.split('/')[-1]
    
    status = '✓' if (has_themis_ns and has_graph_ns) else '✗'
    print(f'{status} {fname:40} themis={has_themis_ns} graph={has_graph_ns}')
