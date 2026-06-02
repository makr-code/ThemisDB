import json
import importlib.util
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
before_dir = repo / 'ai_working' / 'fp_tuning_before'
after_dir = repo / 'ai_working' / 'fp_tuning_after'
before_dir.mkdir(parents=True, exist_ok=True)
after_dir.mkdir(parents=True, exist_ok=True)

specs = [
    (
        'concurrency',
        'ConcurrencyGapScanner',
        'gap_scan_v3_concurrency_aggregate.json',
        repo / 'ai_working' / '_tmp_old_gap_scanner_v3_concurrency.py',
        repo / 'tools' / 'gap_scanner_v3_concurrency.py',
    ),
    (
        'reliability',
        'ReliabilityGapScanner',
        'gap_scan_v3_reliability_aggregate.json',
        repo / 'ai_working' / '_tmp_old_gap_scanner_v3_reliability.py',
        repo / 'tools' / 'gap_scanner_v3_reliability.py',
    ),
    (
        'memory',
        'MemoryGapScanner',
        'gap_scan_v3_memory_aggregate.json',
        repo / 'ai_working' / '_tmp_old_gap_scanner_v3_memory.py',
        repo / 'tools' / 'gap_scanner_v3_memory.py',
    ),
    (
        'security',
        'SecurityGapScanner',
        'gap_scan_v3_security_aggregate.json',
        repo / 'ai_working' / '_tmp_old_gap_scanner_v3_security.py',
        repo / 'tools' / 'gap_scanner_v3_security.py',
    ),
    (
        'container',
        'ContainerGapScanner',
        'gap_scan_v3_container_misuse_aggregate.json',
        repo / 'ai_working' / '_tmp_old_gap_scanner_v3_container_misuse.py',
        repo / 'tools' / 'gap_scanner_v3_container_misuse.py',
    ),
]


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, str(path))
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


for tag, cls_name, out_name, old_path, new_path in specs:
    old_mod = load_module(old_path, f'old_{tag}')
    old_cls = getattr(old_mod, cls_name)
    old_scanner = old_cls(str(repo))
    old_agg = old_scanner.run_full_scan(str(before_dir))
    (before_dir / out_name).write_text(json.dumps(old_agg, indent=2), encoding='utf-8')
    print(f'before_{tag}_modules={len(old_agg)}')

    new_mod = load_module(new_path, f'new_{tag}')
    new_cls = getattr(new_mod, cls_name)
    new_scanner = new_cls(str(repo))
    new_agg = new_scanner.run_full_scan(str(after_dir))
    (after_dir / out_name).write_text(json.dumps(new_agg, indent=2), encoding='utf-8')
    print(f'after_{tag}_modules={len(new_agg)}')

print('done')
