#!/usr/bin/env python3
"""Check namespace balance in all graph module cpp files"""

import os
import sys

# Files from THEMIS_GRAPH_SOURCES in exact order
files = [
    "src/acceleration/ai_hardware_dispatcher.cpp",
    "src/index/graph_auto_buffer.cpp",
    "src/index/spatial_index.cpp",
    "src/index/temporal_graph.cpp",
    "src/index/property_graph.cpp",
    "src/index/edge_types.cpp",
    "src/index/process_graph.cpp",
    "src/index/gnn_embeddings.cpp",
    "src/index/graph_analytics.cpp",
    "src/graph/graph_query_optimizer.cpp",
    "src/graph/explain_plan.cpp",
    "src/graph/ontology_manager.cpp",
    "src/graph/knowledge_graph_reasoner.cpp",
    "src/graph/rotate_completion.cpp",
    "src/query/result_stream.cpp",
    "src/graph/path_constraints.cpp",
    "src/graph/distributed_graph.cpp",
    "src/graph/gpu_traversal.cpp",
    "src/graph/parallel_traversal.cpp",
    "src/graph/scheduled_edge_refresh.cpp",
]

os.chdir(r"c:\Projects\ThemisDB")
cumulative_balance = 0

for i, fname in enumerate(files, 1):
    if not os.path.exists(fname):
        print(f"⚠️  File {i}: {fname} - NOT FOUND")
        continue
    
    with open(fname, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    opens = content.count('{')
    closes = content.count('}')
    balance = opens - closes
    cumulative_balance += balance
    
    status = "✅" if balance == 0 else "❌"
    print(f"{status} {i:2d}. {fname:45} opens={opens:3d} closes={closes:3d} balance={balance:+3d} cumul={cumulative_balance:+3d}")

print()
print(f"TOTAL CUMULATIVE BALANCE: {cumulative_balance:+d}")
if cumulative_balance != 0:
    print(f"⚠️  UNCLOSED CONTEXT DETECTED - {cumulative_balance} extra opening braces")
