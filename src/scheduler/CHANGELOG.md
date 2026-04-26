> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Scheduler Module

All notable changes documented here. Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.8.0] — 2026-03-21
### Added
- `TaskScheduler::RequestContext` struct with `user_id` and `client_ip` fields for per-thread authentication context propagation
- `static setRequestContext(ctx)`, `clearRequestContext()`, `currentUserId(fallback)`, `currentClientIp()` API using `thread_local TLSRequestContext` — HTTP handlers set context before scheduler ops; scheduler thread falls back to `"system"`
- `sandbox_execution` config flag: when `true`, wraps user-provided task functions in `modules::ModuleSandbox` with cgroups v2 memory/CPU limits, seccomp-bpf syscall filtering on Linux; graceful fallback when sandbox launch fails in constrained environments
- `TaskSchedulerAuthContextFocusedTests` — 11 unit tests covering TLS context isolation, fallback behaviour, and `sandbox_execution` flag (`tests/test_task_scheduler_auth_context.cpp`)
- CI workflow: `.github/workflows/02-feature-modules_taskscheduler-auth-context-ci.yml`

### Fixed
- Audit events no longer hardcode `"system"` as actor; all `registerTask`, `updateTask`, `enableTask`, `disableTask`, `executeTaskNow` audit log entries now use the thread-local `currentUserId()` / `currentClientIp()` values

## [1.5.0] — 2026-03-12
### Added
- Cron expression parser with full standard support
- Task priority queues with backpressure
- Persistent job state (survive restarts)
- Distributed job coordination across nodes
- Task dependency graph (DAG scheduling)

## [1.0.0] — 2024-01-01
### Added
- Initial implementation
