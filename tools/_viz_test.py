import importlib.util
from pathlib import Path
import sys

module_path = Path(__file__).parent / 'visualizer_tk.py'
if not module_path.exists():
    print('visualizer_tk.py not found at', module_path)
    sys.exit(2)

spec = importlib.util.spec_from_file_location('vizmod', str(module_path))
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

graph_path = Path('ai_working/include_graph_tools_scanners_libclang.json')
if not graph_path.exists():
    print('graph file not found:', graph_path)
    sys.exit(3)

m = mod.GraphModel(str(graph_path))
print('nodes:', len(m.nodes))
print('edges:', len(m.edges))
print('chunks:', len(m.chunks))
print('gaps:', len(m.gaps))
# print first 5 node ids
print('sample nodes:', list(m.nodes.keys())[:5])
print('OK')
