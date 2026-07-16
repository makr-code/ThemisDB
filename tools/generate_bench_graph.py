#!/usr/bin/env python3
"""
Generate a synthetic benchmark graph JSON for the visualizer.
Usage: python tools/generate_bench_graph.py --nodes 2000 --avg-degree 2 --out tools/bench_2000.json
"""
import argparse
import json
import random

def make_graph(n_nodes=1000, avg_deg=2, out='tools/bench.json'):
    nodes = []
    edges = []
    for i in range(n_nodes):
        nid = f"n{i}"
        nodes.append({"id": nid, "label": f"src/file_{i}.cpp"})
    # ensure connectivity via a ring
    for i in range(n_nodes):
        edges.append({"source": f"n{i}", "target": f"n{(i+1)%n_nodes}"})
    # add random edges to reach avg degree
    target_edges = int(n_nodes * avg_deg)
    extra = max(0, target_edges - n_nodes)
    seen = set((e['source'], e['target']) for e in edges)
    while extra > 0:
        a = random.randrange(n_nodes)
        b = random.randrange(n_nodes)
        if a == b:
            continue
        key = (f"n{a}", f"n{b}")
        if key in seen:
            continue
        edges.append({"source": key[0], "target": key[1]})
        seen.add(key)
        extra -= 1
    graph = {"nodes": nodes, "edges": edges, "chunks": {}, "gaps": {}}
    with open(out, 'w', encoding='utf-8') as f:
        json.dump(graph, f)
    print(f"Wrote {len(nodes)} nodes, {len(edges)} edges to {out}")

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--nodes', type=int, default=1000)
    p.add_argument('--avg-degree', type=float, default=2.0)
    p.add_argument('--out', type=str, default='tools/bench.json')
    args = p.parse_args()
    make_graph(args.nodes, args.avg_degree, args.out)
