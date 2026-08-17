#!/usr/bin/env python3

filepath = "/home/runner/work/ThemisDB/ThemisDB/src/llm/grafana_metrics.cpp"

with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

# Look for function signatures in class implementations
# Count function signatures vs. opening braces

func_signatures = []
for i, line in enumerate(lines, 1):
    # Look for lines like "Type ClassName::methodName() {"
    if ('::' in line and '(' in line and ')' in line and 
        'namespace' not in line and 'template' not in line and
        not line.strip().startswith('//')):
        
        # Check if this is a function signature (ends with ; or { on same or next line)
        if line.rstrip().endswith(';'):
            continue  # This is a declaration in header included
        
        # This is likely a function implementation
        has_brace = '{' in line
        func_signatures.append({
            'line': i,
            'content': line.strip()[:80],
            'has_brace': has_brace,
            'full_line': line
        })

print("Function signatures found (first 30):")
count = 0
for sig in func_signatures[:30]:
    if '::' in sig['content'] and '(' in sig['content']:
        print(f"L{sig['line']:4d} {'[HAS]' if sig['has_brace'] else '[MISS]'}: {sig['content']}")
        count += 1

print("\n\nLooking for function signatures missing opening braces on same line:")
for sig in func_signatures:
    if not sig['has_brace'] and 'static constexpr' not in sig['full_line']:
        # Check if next few lines have opening brace
        line_num = sig['line']
        found_brace_soon = False
        for j in range(1, 5):
            if line_num + j <= len(lines) and '{' in lines[line_num + j - 1]:
                found_brace_soon = True
                break
        
        if found_brace_soon:
            print(f"L{sig['line']}: {sig['content']}")
