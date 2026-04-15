/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            database_adapter.hpp                               ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 18:44:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     29                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 29ac1cf537  2026-04-14  fix                                     ║
    • fd7d928faf  2026-04-10  refactor(chimera): extract generic chimera assets into su... ║
    • 04a46f63a9  2026-03-12  fix(chimera): address PR review comments on multi-databas... ║
    • 3bd2167e65  2026-03-12  feat(chimera): implement multi-database adapter registrat... ║
    • 16eb8c2a4c  2026-03-12  fix(chimera): address async API review comments (RAII cle... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "../../external/chimera/include/chimera/database_adapter.hpp"
