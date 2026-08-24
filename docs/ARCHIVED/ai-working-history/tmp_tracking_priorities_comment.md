## Update: Zusätzliche Doku-Issues verknüpft und priorisiert

Neu angelegte Zusatz-Issues:
- #4914 Toolchain (MkDocs/Publishing)
- #4915 API (OpenAPI/Generator)
- #4916 i18n (de/en/es/fr/ja)
- #4917 Docs-Meta (QA/Generated/Reports)
- #4918 Compendium non-chapter
- #4919 Root AI-Governance/Versioning

### Empfohlene Bearbeitungsreihenfolge
1. P1: #4914, #4915
2. P2: #4919, #4916
3. P3: #4917, #4918

### Begründung
- Toolchain/API zuerst, da sie viele Folge-Dokuänderungen technisch/strukturell beeinflussen.
- Danach Governance/Versioning und i18n für konsistente Inhalte.
- Abschließend Meta- und Compendium-Konsolidierung als Cleanup/Hardening.

### Abhängigkeiten zu bestehenden Root-Issues
- #4908 Einstiegspfad
- #4909 Governance/Community
- #4910 Release/Planung
- #4911 Architektur/Audit/Security
- #4912 Performance
- #4913 Root-Artefakte/Betriebslogs

---

## Doxygen-Auswertung (maschinenfreundlich)

source_log: build/doxygen/doxygen-warnings.log
timestamp: 2026-05-11

metrics:
	total: 716
	undocumented: 31
	param_mismatch: 152 (baseline) -> 121 (current) [-31, 20% reduction]
	unsupported_xml_html_tag: 241 -> 28 (after DX-001)

dx002_progress:
	status: Phase 1 Complete
	baseline_param_violations: 152
	current_param_violations: 121
	fixed: 31 (20% reduction)
	patches_applied: 5 files (rag_judge, vram_secure_clear, graph_query_optimizer, timeseries, query_federation)
	strategy: Split overloaded function documentation to match individual signatures
	breakdown:
		- too_many: 56 -> 44 (-12)
		- param_mismatch: 73 -> 61 (-12)
		- no_args_with_param: 17 -> 15 (-2)
	phase_2: Target 20 high-priority files, reduce to ≤50 total violations

module_ranking:
	- { module: query, count: 69 }
	- { module: index, count: 54 }
	- { module: plugins, count: 40 }
	- { module: rag, count: 39 }
	- { module: content, count: 38 }
	- { module: analytics, count: 35 }
	- { module: utils, count: 30 }
	- { module: server, count: 27 }
	- { module: process, count: 26 }
	- { module: storage, count: 24 }
	- { module: llm, count: 24 }
	- { module: auth, count: 21 }
	- { module: search, count: 20 }
	- { module: temporal, count: 19 }
	- { module: graph, count: 16 }

agent_queue:
	- id: DX-001
		status: done
		priority: P1
		type: syntax
		selector: "Unsupported xml/html tag"
		scope: "include/query/**"
		target: "Q2 2026"
		result: "query unsupported-tag warnings 19 -> 0"
	- id: DX-002
		status: open
		priority: P1
		type: param
		selector: "too many @param|argument .* @param"
		scope: "include/**"
		target: "Q2 2026"
	- id: DX-003
		priority: P2
		type: undocumented
		selector: "is not documented"
		scope: "include/query/**|include/index/**|include/server/**"
		target: "Q2 2026"
	- id: DX-004
		priority: P3
		type: undocumented
		selector: "is not documented"
		scope: "include/plugins/**|include/rag/**|include/content/**|include/analytics/**"
		target: "Q3 2026"

definition_of_done:
	- "Count(Unsupported xml/html tag) == 0"
	- "Count(too many @param) == 0"
	- "Count(argument .* @param) == 0"
	- "UNDOC in DX-003 scopes == 0"
