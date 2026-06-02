# scraper Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: scraper
- Generated: 2026-06-02 11:09:13
- Status: High-Priority Findings Present
- Total Findings: 35
- Actionable Findings (Critical + High): 2
- Affected Files: 8

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 2 |
| Medium | 32 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 31 |
| container | 17 |
| reliability | 17 |
| raii | 7 |
| platform | 4 |
| llm_ai_safety | 2 |
| performance | 2 |
| audit_logging | 1 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/scraper/gov_source_catalog.cpp | 9 | 0 | 0 | 8 | 1 |
| src/scraper/scraper_config.cpp | 6 | 0 | 0 | 6 | 0 |
| src/scraper/scraper_plugin.cpp | 6 | 0 | 0 | 6 | 0 |
| src/scraper/scraper_search_engine.cpp | 6 | 0 | 1 | 5 | 0 |
| src/scraper/scraper_api_client.cpp | 3 | 0 | 0 | 3 | 0 |
| src/scraper/scraper_llm_evaluator.cpp | 3 | 0 | 1 | 2 | 0 |
| src/scraper/scraper_js_renderer.cpp | 1 | 0 | 0 | 1 | 0 |
| src/scraper/scraper_metadata_writer.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/scraper/gov_source_catalog.cpp
Total findings: 9

- Line 50: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.type == type) result.push_back(&s);
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.type == type) result.push_back(&s);
  Confidence: band=high; score=0.74
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.bundesland == iso) result.push_back(&s);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.enabled) result.push_back(&s);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (const auto* s = findById(id)) result.push_back(s);
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sources_.push_back(std::move(source));
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };
  Confidence: band=high; score=0.74
- Line 27: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: GovSourceCatalog::GovSourceCatalog() {
  Confidence: band=medium; score=0.6

### src/scraper/scraper_config.cpp
Total findings: 6

- Line 35: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: g.keywords.push_back(kw.as<std::string>());
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: o.queries.push_back(q.as<std::string>());
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: o.source_ids.push_back(s.as<std::string>());
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.seed_urls.push_back(u.as<std::string>());
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.whitelist.push_back(u.as<std::string>());
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.blacklist.push_back(u.as<std::string>());
  Confidence: band=high; score=0.74

### src/scraper/scraper_plugin.cpp
Total findings: 6

- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: seeds.emplace_back(url, "");
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s->enabled) gov_sources.push_back(s);
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s->enabled) gov_sources.push_back(s);
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s->enabled) gov_sources.push_back(s);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!url.empty()) seeds.emplace_back(url, src->id);
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (html[i] == '>') { in_tag = false; out += ' '; continue; }
  Confidence: band=high; score=0.74

### src/scraper/scraper_search_engine.cpp
Total findings: 6

- Line 250: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Walk inputs
  Confidence: band=very_high; score=0.9
- Line 217: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '>')      { in_tag = false; out += ' '; continue; }
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '>')      { in_tag = false; out += ' '; continue; }
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sf));
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!item.url.empty()) page.items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!item.url.empty()) page.items.push_back(std::move(item));
  Confidence: band=high; score=0.74

### src/scraper/scraper_api_client.cpp
Total findings: 3

- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/scraper/scraper_llm_evaluator.cpp
Total findings: 3

- Line 194: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: themis::llm::InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (e.is_string()) result.key_entities.push_back(e.get<std::string>());
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: themis::llm::LLMPluginManager::instance().generate(req);
  Confidence: band=high; score=0.74

### src/scraper/scraper_js_renderer.cpp
Total findings: 1

- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: argv.push_back(nullptr);
  Confidence: band=high; score=0.74

### src/scraper/scraper_metadata_writer.cpp
Total findings: 1

- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(e));
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
