#!/usr/bin/env python3

filepath = "/home/runner/work/ThemisDB/ThemisDB/src/llm/grafana_metrics.cpp"

with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

# Track scope stack
scopes = []
scope_lines = []

for i, line in enumerate(lines, 1):
    stripped = line.strip()
    
    # Skip empty lines and comments
    if not stripped or stripped.startswith('//'):
        continue
    
    # Count braces but ignore those in strings
    # Simple approach: remove string content first
    safe_line = line
    # Remove raw strings R"(...)"
    import re
    safe_line = re.sub(r'R"[^"]*"[^"]*"', '', safe_line)
    # Remove regular strings
    safe_line = re.sub(r'"[^"]*"', '', safe_line)
    
    opens = safe_line.count('{')
    closes = safe_line.count('}')
    
    for _ in range(opens):
        scopes.append((i, stripped[:50]))
        scope_lines.append((i, 'OPEN', stripped[:50]))
    
    for _ in range(closes):
        if scopes:
            open_line, open_content = scopes.pop()
            scope_lines.append((i, f'CLOSE from {open_line}', stripped[:50]))
        else:
            scope_lines.append((i, 'EXTRA CLOSE!', stripped[:50]))

print("Scope tracking (showing problematic ones):")
for line_num, action, content in scope_lines[-50:]:
    if 'EXTRA' in action or 'CLOSE' in action:
        print(f"L{line_num:4d} {action:25s} {content}")

print(f"\n\nRemaining open scopes: {len(scopes)}")
for line_num, content in scopes:
    print(f"  Opened at L{line_num}: {content}")
