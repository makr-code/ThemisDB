#!/usr/bin/env python3
import json

data = json.load(open('gap_scan_aggregate.json'))
sorted_modules = sorted(data.items(), key=lambda x: x[1]['total'], reverse=True)

print("TOP 20 MODULES BY GAP COUNT")
print("=" * 60)
for i, (module, summary) in enumerate(sorted_modules[:20], 1):
    print(f"{i:2}. {module:25} {summary['total']:3} gaps "
          f"(C:{summary['critical']:2} H:{summary['high']:2} M:{summary['medium']:2})")
