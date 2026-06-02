# user_storage_encrypted Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: user_storage_encrypted
- Generated: 2026-06-02 11:09:13
- Status: Findings Present
- Total Findings: 17
- Actionable Findings (Critical + High): 0
- Affected Files: 12

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 0 |
| Medium | 17 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| raii | 39 |
| reliability | 22 |
| performance_patterns | 15 |
| exception_safety | 14 |
| platform | 13 |
| container | 11 |
| memory | 8 |
| performance | 4 |
| audit_logging | 3 |
| observability | 2 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/user_storage_encrypted/multi_level_storage.cpp | 11 | 0 | 0 | 11 | 0 |
| src/user_storage_encrypted/gocryptfs_backend.cpp | 4 | 0 | 0 | 4 | 0 |
| src/user_storage_encrypted/key_derivation_service.cpp | 2 | 0 | 0 | 2 | 0 |
| include/user_storage_encrypted/encryption_backend_interface.hpp | 0 | 0 | 0 | 0 | 0 |
| include/user_storage_encrypted/gocryptfs_backend.hpp | 0 | 0 | 0 | 0 | 0 |
| include/user_storage_encrypted/irotation_store.hpp | 0 | 0 | 0 | 0 | 0 |
| include/user_storage_encrypted/key_derivation_service.hpp | 0 | 0 | 0 | 0 | 0 |
| include/user_storage_encrypted/key_rotation_scheduler.hpp | 0 | 0 | 0 | 0 | 0 |
| include/user_storage_encrypted/multi_level_storage.hpp | 0 | 0 | 0 | 0 | 0 |
| include/user_storage_encrypted/security_level.hpp | 0 | 0 | 0 | 0 | 0 |
| include/user_storage_encrypted/user_models.hpp | 0 | 0 | 0 | 0 | 0 |
| src/user_storage_encrypted/key_rotation_scheduler.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/user_storage_encrypted/multi_level_storage.cpp
Total findings: 11

- Line 212: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto storage_config = config["multi_level_storage"];
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc = level_json["encryption"];
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto rot = level_json["rotation"];
  Confidence: band=high; score=0.74
- Line 875: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: users.push_back(result.value());
  Confidence: band=high; score=0.74
- Line 942: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups.push_back(result.value());
  Confidence: band=high; score=0.74
- Line 962: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: status.errors.push_back(level_health.error());
  Confidence: band=high; score=0.74
- Line 1021: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: known_mount_points.push_back(cfg.mount_point);
  Confidence: band=high; score=0.74
- Line 1047: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stale_mounts.push_back(mount_point);
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(a.c_str()));
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(a.c_str()));
  Confidence: band=high; score=0.74
- Line 1085: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(a.c_str()));
  Confidence: band=high; score=0.74

### src/user_storage_encrypted/gocryptfs_backend.cpp
Total findings: 4

- Line 235: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: hex_key += '\n';
  Confidence: band=high; score=0.74
- Line 307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(arg.c_str()));
  Confidence: band=high; score=0.74
- Line 476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(arg.c_str()));
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: c_args.push_back(const_cast<char*>(arg.c_str()));
  Confidence: band=high; score=0.74

### src/user_storage_encrypted/key_derivation_service.cpp
Total findings: 2

- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: key_derivation_service.cpp | Version: 0.0.12 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "user_storage_encrypted/key_derivation_service.hpp"
  Confidence: band=high; score=0.74

### include/user_storage_encrypted/encryption_backend_interface.hpp
Total findings: 0


### include/user_storage_encrypted/gocryptfs_backend.hpp
Total findings: 0


### include/user_storage_encrypted/irotation_store.hpp
Total findings: 0


### include/user_storage_encrypted/key_derivation_service.hpp
Total findings: 0


### include/user_storage_encrypted/key_rotation_scheduler.hpp
Total findings: 0


### include/user_storage_encrypted/multi_level_storage.hpp
Total findings: 0


### include/user_storage_encrypted/security_level.hpp
Total findings: 0


### include/user_storage_encrypted/user_models.hpp
Total findings: 0


### src/user_storage_encrypted/key_rotation_scheduler.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
