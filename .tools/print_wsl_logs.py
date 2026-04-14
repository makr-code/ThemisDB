"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            print_wsl_logs.py                                  ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:21:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     47                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from pathlib import Path
files = [
    Path(r"C:/Temp/themis_wsl_run_02_AQLOrTest.FulltextInOr_ShouldFail.txt"),
    Path(r"C:/Temp/themis_wsl_run_03_AQLTranslatorTest.OrOperatorNotSupported.txt"),
    Path(r"C:/Temp/themis_wsl_run_04_EncryptionE2ETest.Performance_BulkEncryption_1000Entities.txt"),
    Path(r"C:/Temp/themis_wsl_run_05_HttpPiiManagerTest.DeleteIdempotent.txt"),
    Path(r"C:/Temp/themis_wsl_run_06_HttpPiiManagerTest.DeleteMapping.txt"),
    Path(r"C:/Temp/themis_wsl_run_07_KeyProviderSigning.SignVerifyUsingKeyProvider.txt"),
    Path(r"C:/Temp/themis_wsl_run_09_VaultKeyProviderRetry.RetriesAndSucceeds.txt"),
]
for p in files:
    print('-----', p, '-----')
    if not p.exists():
        print('MISSING')
        print()
        continue
    lines = p.read_text(encoding='utf-8', errors='replace').splitlines()
    tail = lines[-200:]
    if not tail:
        print('(file empty)')
    else:
        for l in tail:
            print(l)
    print()
