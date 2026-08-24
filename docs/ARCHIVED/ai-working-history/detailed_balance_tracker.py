#!/usr/bin/env python3

filepath = "/home/runner/work/ThemisDB/ThemisDB/src/llm/grafana_metrics.cpp"

with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
    lines = f.readlines()

balance = 0
min_balance = 0
min_line_num = 0
issue_lines = []

for i, line in enumerate(lines, 1):
    opens = line.count('{')
    closes = line.count('}')
    balance += opens - closes
    
    if balance < min_balance:
        min_balance = balance
        min_line_num = i
        issue_lines.append((i, line.strip(), balance))

print(f"Minimum balance reached: {min_balance} at line {min_line_num}")
print(f"Final balance: {balance}")

# Show lines where balance dips below 0
print("\nLines where balance becomes negative:")
for line_num, content, bal in issue_lines:
    print(f"L{line_num}: {content[:80]}")
    print(f"  Balance: {bal}")

# Let's specifically check line 1467 and surrounding lines
print("\n\nLines around 1459-1470:")
for i in range(1458, 1472):
    if i < len(lines):
        line = lines[i]
        opens = line.count('{')
        closes = line.count('}')
        balance += opens - closes
        print(f"L{i+1}: {line.rstrip()}")
