#!/usr/bin/env python3
"""Scan all 21 files for namespace opens/closes."""

import re

files = [
    'src/acceleration/ai_hardware_dispatcher.cpp',
    'src/index/graph_auto_buffer.cpp',
    'src/index/spatial_index.cpp',
    'src/index/temporal_graph.cpp',
    'src/index/property_graph.cpp',
    'src/index/edge_types.cpp',
    'src/index/process_graph.cpp',
    'src/index/gnn_embeddings.cpp',
    'src/index/graph_analytics.cpp',
    'src/graph/graph_query_optimizer.cpp',
    'src/graph/explain_plan.cpp',
    'src/graph/ontology_manager.cpp',
    'src/graph/knowledge_graph_reasoner.cpp',
    'src/graph/rotate_completion.cpp',
    'src/query/result_stream.cpp',
    'src/graph/path_constraints.cpp',
    'src/graph/distributed_graph.cpp',
    'src/graph/gpu_traversal.cpp',
    'src/graph/parallel_traversal.cpp',
    'src/graph/scheduled_edge_refresh.cpp',
    'src/graph/graph_query_rewriter.cpp',
]

total_open, total_close = 0, 0

for i, f in enumerate(files, start=1):
    with open(f, 'r', encoding='utf-8', errors='ignore') as fp:
        content = fp.read()
    
    opens = len(re.findall(r'namespace\s+\w+\s*\{', content))
    closes = len(re.findall(r'\}\s*//\s*namespace', content))
    total_open += opens
    total_close += closes
    
    bal = total_open - total_close
    fname = f.split("/")[-1]
    print(f'{i:2}: {fname:35} O={opens} C={closes} => total bal={bal:+d}')
