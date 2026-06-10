import json

with open('ai_working/gap_scan_v3_summary.json') as f:
    data = json.load(f)
    
print('=== WAVE 5/6 FILTER STATUS ===')
print(f'Total gaps: {data["total_gaps"]}')
print(f'Scan time: {data["scan_date"]}')
print(f'Modules: {data["modules_scanned"]}')

if data['total_gaps'] > 20000:
    print('\n⚠️  New scan WITHOUT Wave 5/6 filters')
    print(f'   Expected 12,539 post-Wave5, got {data["total_gaps"]}')
elif data['total_gaps'] < 15000:
    print('\n✅ Wave 5/6 filters applied successfully')
else:
    print('\n❓ Ambiguous: intermediate state')
