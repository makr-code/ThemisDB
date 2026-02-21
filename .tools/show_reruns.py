"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            show_reruns.py                                     ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     12                                             ║
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
