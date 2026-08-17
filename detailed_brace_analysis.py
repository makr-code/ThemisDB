#!/usr/bin/env python3

filepath = "/home/runner/work/ThemisDB/ThemisDB/src/llm/grafana_metrics.cpp"

with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

# Track every brace and print balance changes only for critical sections
balance = 0
in_namespace = 0
critical_lines = []

for i, line in enumerate(lines, 1):
    open_count = line.count('{') - line.count("}{")  # Avoid double counting
    close_count = line.count('}') - line.count("}{")
    
    # Only count actual braces
    open_count = line.count('{')
    close_count = line.count('}')
    
    balance += open_count - close_count
    
    # Print lines near certain functions
    if any(x in line for x in ['::start()', '::stop()', '::handleRequest', 'handlePost', 'handleDelete']):
        print(f"L{i:4d} [{balance:+3d}] {line.rstrip()}")
    
    # Check for unmatched structural elements
    if 'class ' in line or 'struct ' in line:
        print(f"L{i:4d} [{balance:+3d}] {line.rstrip()}")
    
    if 'namespace ' in line:
        in_namespace += open_count - close_count
        print(f"L{i:4d} [{balance:+3d}] {line.rstrip()}")

print(f"\nFinal balance: {balance}")
print(f"Final namespace balance: {in_namespace}")

# Now let's check if there are function bodies without opening braces
print("\n\nChecking for function signatures...")

for i, line in enumerate(lines, 1):
    # Look for lines that look like function signature (ends with ) and has no {)
    if ')' in line and '{' not in line and 'namespace' not in line:
        next_line_idx = i
        if next_line_idx < len(lines):
            next_line = lines[next_line_idx]
            # If next line is not empty and doesn't have opening brace
            if next_line.strip() and '{' not in next_line and next_line.strip()[0] != '/':
                if any(x in line for x in ['MetricsServer::', 'PrometheusExporter::', 'LLMMetricsCollector::', 'GrafanaDashboardGenerator::']):
                    print(f"Potential issue L{i}: {line.rstrip()}")
                    print(f"  Next: {next_line.rstrip()}")
