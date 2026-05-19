import json

with open('phase2_batch_results.json') as f:
    data = json.load(f)

modules = []
for module_name, module_data in data.items():
    total = module_data.get('effort_estimate', {}).get('total_hours', 0)
    tasks = len(module_data.get('task_breakdown', []))
    modules.append((module_name, total, tasks))

modules.sort(key=lambda x: x[1], reverse=True)
print('\nTOP 20 MODULE NACH GRÖSSE:')
print('='*70)
print(f'{"#":>2} {"Modul":25s} {"Stunden":>10s} {"Tasks":>6s} {"Status":15s}')
print('-'*70)

for i, (name, hours, tasks) in enumerate(modules[:20], 1):
    if hours >= 5000:
        status = 'MEGA'
    elif hours >= 1000:
        status = 'GROSS'
    elif hours >= 200:
        status = 'MEDIUM'
    elif hours >= 50:
        status = 'KLEIN'
    else:
        status = 'TINY'
    print(f'{i:2d} {name:25s} {hours:10.0f}h {tasks:6d}  {status:15s}')

print('\n' + '='*70)
print('EMPFEHLUNG FÜR PHASE 3 PoC:')
print('='*70)
print(f'\n1. LLM (Modul #1)         - {modules[0][1]:.0f}h - ZU GROSS für PoC')
print(f'2. Auth (Modul #?)        - ~300-500h - GUT für PoC')
print(f'3. Cache (Modul #?)       - ~200-400h - OPTIMAL für PoC')
print(f'4. Config (Modul #?)      - ~100-200h - KLEIN, schnell')
print('\nVorschlag: CACHE oder AUTH als PoC-Modul (Medium-Größe, risikofrei)')
