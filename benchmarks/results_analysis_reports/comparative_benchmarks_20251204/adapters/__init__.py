"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __init__.py                                        ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     38                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

# ThemisDB Comparative Benchmark - Adapters Package
"""
Database adapters for comparative benchmarking.
"""

from .themisdb_adapter import ThemisDBAdapter
from .postgresql_adapter import PostgreSQLAdapter
from .neo4j_adapter import Neo4jAdapter
from .chromadb_adapter import ChromaDBAdapter

__all__ = [
    "ThemisDBAdapter",
    "PostgreSQLAdapter",
    "Neo4jAdapter",
    "ChromaDBAdapter",
]
