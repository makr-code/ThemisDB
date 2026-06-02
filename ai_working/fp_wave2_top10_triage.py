import json
import re
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
report_path = repo / 'ai_working' / 'fp_delta_wave2_report.json'

report = json.loads(report_path.read_text(encoding='utf-8', errors='ignore'))
remaining = report.get('remaining_top25', {})


def read_ctx(rel_path: str, line_no: int, radius: int = 20):
    p = repo / rel_path.replace('\\', '/')
    if not p.exists():
        return '', '', '', []
    lines = p.read_text(encoding='utf-8', errors='ignore').splitlines()
    i = max(0, line_no - 1)
    s = max(0, i - radius)
    e = min(len(lines), i + radius + 1)
    prev = '\n'.join(lines[s:i])
    cur = lines[i] if i < len(lines) else ''
    nxt = '\n'.join(lines[i + 1:e])
    return prev, cur, nxt, lines


def classify_deadlock(item):
    prev, cur, nxt, _ = read_ctx(item['file'], int(item['line']))
    ctx = (prev + '\n' + cur + '\n' + nxt)
    lock_lines = re.findall(r'std::(?:lock_guard|unique_lock|scoped_lock|shared_lock)\s*<[^>]*>\s*\w*\s*\(\s*([A-Za-z_]\w*)', ctx)
    uniq = sorted(set(lock_lines))
    if len(uniq) <= 1:
        return 'likely_fp', 'single_mutex_in_context'
    if 'std::lock(' in ctx or 'std::scoped_lock' in ctx:
        return 'likely_real', 'multi_lock_pattern_detected'
    return 'needs_review', 'multiple_locks_nearby'


def classify_lock_in_loop(item):
    prev, cur, nxt, _ = read_ctx(item['file'], int(item['line']), radius=40)
    ctx = (cur + '\n' + nxt)
    has_lock = bool(re.search(r'\b(lock_guard|unique_lock|scoped_lock)\b', ctx))
    if has_lock:
        return 'likely_real', 'lock_construct_found_in_loop_body_window'
    return 'likely_fp', 'no_lock_construct_in_loop_body_window'


def classify_null_deref(item):
    prev, cur, nxt, _ = read_ctx(item['file'], int(item['line']))
    m = re.search(r'\b([A-Za-z_]\w*)\s*->', cur)
    if not m:
        return 'likely_fp', 'no_dereference_on_flagged_line'
    var = re.escape(m.group(1))
    guard_patterns = [
        rf'if\s*\(\s*{var}\s*!=\s*nullptr\s*\)',
        rf'if\s*\(\s*!\s*{var}\s*\)',
        rf'\b{var}\b\s*&&\s*{var}\s*->',
        rf'\b{var}\b\s*==\s*nullptr\s*\|\|\s*{var}\s*->',
        rf'assert\s*\(\s*{var}\s*\)',
        rf'CHECK\s*\(\s*{var}\s*\)',
    ]
    if any(re.search(p, prev + '\n' + cur) for p in guard_patterns):
        return 'likely_fp', 'null_guard_present'
    if re.search(rf'\b{var}\b\s*=\s*nullptr', prev):
        return 'likely_fp', 'nullptr_assignment_nearby'
    return 'likely_real', 'unguarded_pointer_dereference'


def classify_uncaught(item):
    prev, cur, nxt, _ = read_ctx(item['file'], int(item['line']), radius=30)
    line = cur.lower()
    if 'throw' not in line:
        return 'likely_fp', 'non_throw_line_flagged'
    if any(exc in line for exc in [
        'std::invalid_argument', 'std::out_of_range', 'std::domain_error',
        'std::logic_error', 'std::runtime_error',
    ]):
        near = (prev + '\n' + cur).lower()
        if 'if (' in near and any(tok in near for tok in ['invalid', 'must', 'required', 'cannot', 'empty', 'range']):
            return 'likely_fp', 'constructor_or_argument_validation_throw'
        if re.search(r'\b([A-Za-z_]\w*)::\1\s*\(', prev):
            return 'likely_fp', 'constructor_validation_throw'
    if 'catch (...)' in cur:
        return 'likely_real', 'generic_catch_reported_as_uncaught_policy_issue'
    return 'needs_review', 'throw_without_local_try_context'


classifiers = {
    'deadlock_risk': classify_deadlock,
    'lock_in_loop': classify_lock_in_loop,
    'null_dereference': classify_null_deref,
    'uncaught_exception': classify_uncaught,
}

triage = {}
for gtype, items in remaining.items():
    top10 = items[:10]
    out = []
    fn = classifiers[gtype]
    for it in top10:
        verdict, reason = fn(it)
        out.append({
            'file': it['file'],
            'line': it['line'],
            'verdict': verdict,
            'reason': reason,
        })
    triage[gtype] = out

summary = {}
for gtype, items in triage.items():
    counts = {'likely_fp': 0, 'likely_real': 0, 'needs_review': 0}
    for it in items:
        counts[it['verdict']] += 1
    summary[gtype] = counts

result = {'summary': summary, 'top10_triage': triage}
json_out = repo / 'ai_working' / 'fp_wave2_top10_triage.json'
md_out = repo / 'ai_working' / 'fp_wave2_top10_triage.md'
json_out.write_text(json.dumps(result, indent=2), encoding='utf-8')

lines = ['# FP Wave2 Top10 Triage', '']
for gtype in ['deadlock_risk', 'lock_in_loop', 'null_dereference', 'uncaught_exception']:
    c = summary.get(gtype, {'likely_fp': 0, 'likely_real': 0, 'needs_review': 0})
    lines.append(f'## {gtype}')
    lines.append(f"likely_fp={c['likely_fp']} likely_real={c['likely_real']} needs_review={c['needs_review']}")
    for it in triage.get(gtype, []):
        lines.append(f"- {it['file']}:{it['line']} -> {it['verdict']} ({it['reason']})")
    lines.append('')
md_out.write_text('\n'.join(lines), encoding='utf-8')

print(json_out)
print(md_out)
for gtype, c in summary.items():
    print(f"{gtype}: fp={c['likely_fp']} real={c['likely_real']} review={c['needs_review']}")
