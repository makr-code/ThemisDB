#!/usr/bin/env python3

filepath = "/home/runner/work/ThemisDB/ThemisDB/src/llm/grafana_metrics.cpp"

with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
    content = f.read()
    lines = content.split('\n')

# Count all braces properly (accounting for strings)
def count_braces(text):
    """Count braces, ignoring those in strings"""
    opens = 0
    closes = 0
    in_string = False
    in_raw_string = False
    i = 0
    while i < len(text):
        if in_raw_string:
            if text[i:].startswith('")'):
                in_raw_string = False
                i += 2
                continue
        elif in_string:
            if text[i] == '"' and (i == 0 or text[i-1] != '\\'):
                in_string = False
            i += 1
        else:
            if text[i:].startswith('R"'):
                in_raw_string = True
                i += 2
                continue
            elif text[i] == '"':
                in_string = True
                i += 1
            elif text[i] == '{':
                opens += 1
                i += 1
            elif text[i] == '}':
                closes += 1
                i += 1
            else:
                i += 1
    return opens, closes

# Check the entire file
total_opens, total_closes = count_braces(content)
print(f"Total opening braces: {total_opens}")
print(f"Total closing braces: {total_closes}")
print(f"Difference: {total_opens - total_closes}")

# The issue says 3 missing opening braces
# That means there are 3 extra closing braces
# Let's find where closing braces don't have matching opening braces

# Check section by section
sections = [
    ("Header and namespaces", 0, 50),
    ("Prometheus exporter", 25, 250),
    ("LLMMetricsCollector", 240, 900),
    ("GrafanaDashboardGenerator", 1000, 1300),
    ("MetricsServer", 1280, 1400),
    ("HTTP handlers", 1400, 1572),
]

for name, start, end in sections:
    section_text = '\n'.join(lines[start:end])
    opens, closes = count_braces(section_text)
    balance = opens - closes
    print(f"{name:30s} [{start:4d}-{end:4d}]: opens={opens:3d}, closes={closes:3d}, balance={balance:+3d}")
