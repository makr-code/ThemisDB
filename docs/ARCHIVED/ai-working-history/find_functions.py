#!/usr/bin/env python3

import re

filepath = "/home/runner/work/ThemisDB/ThemisDB/src/llm/grafana_metrics.cpp"

with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
    content = f.read()
    lines = content.split('\n')

# Look for function implementations
func_pattern = r'^\s*(?:virtual\s+)?(?:inline\s+)?(?:static\s+)?(?:constexpr\s+)?(?:explicit\s+)?[a-zA-Z_:][\w:*&\s,<>*]+::[a-zA-Z_~][a-zA-Z0-9_]*\s*\([^)]*\)\s*(?:const\s+)?(?:noexcept\s+)?(?:override\s+)?'

funcs = []
for i, line in enumerate(lines, 1):
    if '::' in line and '(' in line and ')' in line:
        funcs.append((i, line.strip()))

print("Function signatures found:")
for line_num, func_line in funcs:
    print(f"L{line_num}: {func_line[:100]}")

# Now let's look for functions that might be missing implementations
print("\n\nLooking for functions without opening braces on the same line:")
for i in range(len(lines)):
    line = lines[i].strip()
    if ')' in line and '::' in line and '{' not in line and 'namespace' not in line:
        # Check if it's a function signature  
        if any(x in line for x in ['MetricsServer::', 'PrometheusExporter::', 'LLMMetricsCollector::', 'GrafanaDashboardGenerator::']) and not line.startswith('//'):
            # Check the next few lines for opening brace
            found_brace = False
            for j in range(1, 5):
                if i+j < len(lines) and '{' in lines[i+j]:
                    found_brace = True
                    break
            
            if found_brace:
                print(f"L{i+1}: {line}")
                for k in range(1, 5):
                    if i+k < len(lines):
                        print(f"  L{i+k+1}: {lines[i+k][:80]}")
