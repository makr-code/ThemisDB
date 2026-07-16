# Implementation Plan - 8 Missing Integrations (2026-05-25)

## Scope
- Fail-closed distributed transaction phase-2 handling.
- Startup validation for protobuf wire and JSON wire integration hooks.
- Cloud backup provider fail-closed behavior without mock-success fallbacks.
- PITR strict WAL replay requirement.
- Distributed trainer strict callback guards for multi-rank mode.
- Wire bootstrap hardening via centralized startup checks.

## Files
- src/transaction/distributed_transaction_manager.cpp
- include/transaction/distributed_transaction_manager.h
- src/themis/wire_protocol_server.cpp
- src/network/wire_protocol_server.cpp
- src/sharding/cloud_backup.cpp
- src/storage/backup_manager.cpp
- src/llm/lora_framework/distributed_trainer.cpp

## Acceptance Criteria
- No silent success when required distributed/cloud/wire callbacks are missing.
- Startup fails with explicit error logs if required runtime bridges are not wired.
- PITR returns failure when WAL replay callback is missing.
- world_size > 1 cannot proceed with local fallback collectives.
- Build remains green for touched translation units.

## Test Scope
- Build: CMake Build (windows-release).
- Focused CTest runs for transaction, wire protocol, backup manager, and lora trainer tests if present.
