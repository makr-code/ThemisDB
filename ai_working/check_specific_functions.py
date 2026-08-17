#!/usr/bin/env python3

import re

filepath = "/home/runner/work/ThemisDB/ThemisDB/src/llm/grafana_metrics.cpp"

with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

# Find each function implementation and check its braces
functions_to_check = [
    ('isRunning', 1433),
    ('getMetricsURL', 1437),
    ('getDashboardURL', 1441),
    ('getHealthURL', 1445),
    ('getReadyURL', 1449),
    ('getModelsURL', 1453),
    ('getAdminReloadURL', 1457),
    ('getAdminSimulateURL', 1461),
    ('getAdminSessionsURL', 1465),
]

for func_name, line_num in functions_to_check:
    print(f"\n{func_name} (line {line_num}):")
    for i in range(line_num - 1, min(line_num + 3, len(lines))):
        print(f"  L{i+1}: {lines[i].rstrip()}")
        # Count braces
        opens = lines[i].count('{')
        closes = lines[i].count('}')
        if opens > 0 or closes > 0:
            print(f"        Opens: {opens}, Closes: {closes}")
