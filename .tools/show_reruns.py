"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            show_reruns.py                                     ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     34                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from pathlib import Path
for p in [Path('C:/Temp/themis_wsl_rerun_vault.txt'), Path('C:/Temp/themis_wsl_rerun_perf.txt')]:
    print('-----', p, '-----')
    if not p.exists():
        print('MISSING')
        continue
    data = p.read_text(encoding='utf-8', errors='replace')
    if not data.strip():
        print('(file empty or whitespace)')
    else:
        print(data)
    print()
