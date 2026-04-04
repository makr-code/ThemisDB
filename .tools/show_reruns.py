"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            show_reruns.py                                     ║
  Version:         0.0.36                                             ║
  Last Modified:   2026-03-30 04:03:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     35                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
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
