# scraper Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: scraper
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 108
- Actionable Findings (Critical + High): 21
- Affected Files: 8

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 7 |
| High | 14 |
| Medium | 87 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 34 |
| performance_patterns | 31 |
| reliability | 22 |
| raii | 8 |
| platform | 4 |
| llm_ai_safety | 2 |
| memory | 2 |
| performance | 2 |
| audit_logging | 1 |
| observability | 1 |
| security | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/scraper/scraper_js_renderer.cpp | 26 | 2 | 4 | 20 | 0 |
| src/scraper/gov_source_catalog.cpp | 19 | 0 | 2 | 16 | 1 |
| src/scraper/scraper_plugin.cpp | 15 | 3 | 1 | 11 | 0 |
| src/scraper/scraper_search_engine.cpp | 14 | 0 | 2 | 12 | 0 |
| src/scraper/scraper_api_client.cpp | 13 | 1 | 4 | 8 | 0 |
| src/scraper/scraper_config.cpp | 12 | 0 | 0 | 12 | 0 |
| src/scraper/scraper_llm_evaluator.cpp | 6 | 0 | 1 | 5 | 0 |
| src/scraper/scraper_metadata_writer.cpp | 3 | 1 | 0 | 2 | 0 |

## Full Scanner Findings

### src/scraper/scraper_js_renderer.cpp
Total findings: 26

- Line 158: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: const int dev_null = ::open("/dev/null", O_WRONLY);
- Line 170: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((n = ::read(pipefd[0], buf.data(), buf.size())) > 0)
- Line 137: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: if (::pipe(pipefd) != 0) {
- Line 139: severity=HIGH; category=posix_only_api
  Description: POSIX-only API pipe( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: result.error   = "pipe() failed";
- Line 143: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: const pid_t pid = ::fork();
- Line 148: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: result.error   = "fork() failed";
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (ss >> tok) tokens.push_back(tok);
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(req.url);
- Line 116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back("--timeout");
- Line 117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::to_string(req.timeout_ms));
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back("--wait-for");
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(req.wait_selector);
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back("--header");
- Line 125: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(kv.first + ": " + kv.second);
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& arg : req.extra_args) tokens.push_back(arg);
- Line 132: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& t : tokens) argv.push_back(const_cast<char*>(t.c_str()));
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: argv.push_back(nullptr);
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: argv.push_back(nullptr);
- Line 145: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipefd[0]);
- Line 146: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipefd[1]);
- Line 155: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipefd[0]);
- Line 156: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipefd[1]);
- Line 159: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (dev_null >= 0) { ::dup2(dev_null, STDERR_FILENO); ::close(dev_null); }
- Line 165: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipefd[1]);
- Line 173: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(pipefd[0]);
- Line 205: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: _pclose(pipe);

### src/scraper/gov_source_catalog.cpp
Total findings: 19

- Line 109: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("GovSourceCatalog: cannot open '" + path + "'");
- Line 167: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.type == type) result.push_back(&s);
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.type == type) result.push_back(&s);
  Confidence: band=high; score=0.74
- Line 53: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s.type == type) result.push_back(&s);
- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.bundesland == iso) result.push_back(&s);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s.bundesland == iso) result.push_back(&s);
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.enabled) result.push_back(&s);
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s.enabled) result.push_back(&s);
- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (const auto* s = findById(id)) result.push_back(s);
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (const auto* s = findById(id)) result.push_back(s);
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sources_.push_back(std::move(source));
  Confidence: band=high; score=0.74
- Line 92: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sources_.push_back(std::move(source));
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };
- Line 283: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: auto add = [&](GovDataSource s) { sources_.push_back(std::move(s)); };
- Line 29: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: GovSourceCatalog::GovSourceCatalog() {
  Confidence: band=medium; score=0.6

### src/scraper/scraper_plugin.cpp
Total findings: 15

- Line 169: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: static std::size_t write(char* p, std::size_t sz, std::size_t nmemb, void* ud) {
- Line 334: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: const WriteResult wr = writer_->write(rel, node, edges, vec);
- Line 491: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: const WriteResult wr = writer_->write(rel, node, edges, vec);
- Line 515: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!initialized_) throw std::runtime_error("ScraperPlugin not initialized");
- Line 127: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: seeds.emplace_back(url, "");
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s->enabled) gov_sources.push_back(s);
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s->enabled) gov_sources.push_back(s);
- Line 143: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s->enabled) gov_sources.push_back(s);
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s->enabled) gov_sources.push_back(s);
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s->enabled) gov_sources.push_back(s);
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (s->enabled) gov_sources.push_back(s);
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!url.empty()) seeds.emplace_back(url, src->id);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { return {}; }
- Line 247: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (html[i] == '>') { in_tag = false; out += ' '; continue; }
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (html[i] == '>') { in_tag = false; out += ' '; continue; }

### src/scraper/scraper_search_engine.cpp
Total findings: 14

- Line 252: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Walk inputs
  Confidence: band=very_high; score=0.9
- Line 361: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: v.push_back("//*[@data-result]");
- Line 219: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '>')      { in_tag = false; out += ' '; continue; }
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '>')      { in_tag = false; out += ' '; continue; }
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '>')      { in_tag = false; out += ' '; continue; }
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sf));
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(sf));
- Line 372: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!item.url.empty()) page.items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!item.url.empty()) page.items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!item.url.empty()) page.items.push_back(std::move(item));
- Line 438: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 454: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 477: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: page.items.push_back(std::move(item));

### src/scraper/scraper_api_client.cpp
Total findings: 13

- Line 275: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: static std::size_t write(char* ptr, std::size_t size,
- Line 278: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: buf->data.append(ptr, size * nmemb);
- Line 278: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buf->data.append(ptr, size * nmemb);
- Line 292: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!curl) throw std::runtime_error("curl_easy_init failed");
- Line 320: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(rc));
- Line 136: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(r));
- Line 184: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 248: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { break; }
- Line 261: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}

### src/scraper/scraper_config.cpp
Total findings: 12

- Line 37: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: g.keywords.push_back(kw.as<std::string>());
  Confidence: band=high; score=0.74
- Line 38: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: g.keywords.push_back(kw.as<std::string>());
- Line 70: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: o.queries.push_back(q.as<std::string>());
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: o.queries.push_back(q.as<std::string>());
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: o.source_ids.push_back(s.as<std::string>());
  Confidence: band=high; score=0.74
- Line 107: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: o.source_ids.push_back(s.as<std::string>());
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.seed_urls.push_back(u.as<std::string>());
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.seed_urls.push_back(u.as<std::string>());
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.whitelist.push_back(u.as<std::string>());
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.whitelist.push_back(u.as<std::string>());
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.blacklist.push_back(u.as<std::string>());
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.blacklist.push_back(u.as<std::string>());

### src/scraper/scraper_llm_evaluator.cpp
Total findings: 6

- Line 196: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: themis::llm::InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 38: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (e.is_string()) result.key_entities.push_back(e.get<std::string>());
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (e.is_string()) result.key_entities.push_back(e.get<std::string>());
- Line 204: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: themis::llm::LLMPluginManager::instance().generate(req);
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/scraper/scraper_metadata_writer.cpp
Total findings: 3

- Line 189: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: WriteResult InMemoryScraperMetadataWriter::write(
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(std::move(e));

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
