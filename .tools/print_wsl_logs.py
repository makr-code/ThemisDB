"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            print_wsl_logs.py                                  ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:33:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • cd30d9ee9  2025-11-16  Stabilize WSL tests: Vault helper, policy override, index... ║
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
