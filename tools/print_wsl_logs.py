"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            print_wsl_logs.py                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     46                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
