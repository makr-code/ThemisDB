from pathlib import Path
from collections import Counter
from tools.scanners.gs3_step04_doc_freshness_rules import ThemisDocFreshnessRulesScan

roots = [Path('src/voice'), Path('src/whisper'), Path('src/utils')]
files = []
for root in roots:
    for ext in ('*.cpp', '*.cc', '*.cxx', '*.h', '*.hpp', '*.hh', '*.hxx'):
        files.extend(root.rglob(ext))

scanner = ThemisDocFreshnessRulesScan('.')
results = scanner.scan_files(files)
print('count', len(results))
print(Counter(item.get('pattern', '') for item in results).most_common(10))
for item in results[:25]:
    print(f"{item.get('file')}:{item.get('line')} {item.get('pattern')} | {item.get('description')}")
