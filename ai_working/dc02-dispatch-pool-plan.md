# DC-02 Dispatch Performance Plan

## Acceptance checks

- Replace per-request thread creation with a shared, bounded worker pool while preserving shard errors, retries, circuit-breaker updates, and timeout behavior.
- Keep `max_parallel_shards` as an upper bound and document the default worker cap.
- Build the focused analytics test and benchmark targets; verify the DC-02 startup p99 remains at or below 500 us and the end-to-end p99 remains reported.
