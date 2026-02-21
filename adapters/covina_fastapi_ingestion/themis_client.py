"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:30:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     35                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 038fc5edd  2026-01-10  Add TCO Calculator Tool with HTML, CSS, and PowerShell Sc... ║
    • 956d2d5ff  2026-01-10  Remove marketing/VCC-internal content from main branch ║
    • 4a8c696bf  2025-10-31  time-series: Gorilla-Codec fix + Tests   ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

import os
import httpx
from typing import Any, Dict

THEMIS_URL = os.getenv("THEMIS_URL", "http://127.0.0.1:8765")

class ThemisClient:
    def __init__(self, base_url: str | None = None, timeout_s: float = 30.0) -> None:
        self.base_url = base_url or THEMIS_URL
        self.timeout_s = timeout_s

    async def import_content(self, payload: Dict[str, Any]) -> Dict[str, Any]:
        async with httpx.AsyncClient(timeout=self.timeout_s) as client:
            r = await client.post(f"{self.base_url}/content/import", json=payload)
            r.raise_for_status()
            return r.json()
