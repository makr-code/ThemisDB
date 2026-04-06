# Token Bucket Algorithm for Per-Client Rate Limiting

**Metadaten:**
- Source: RFC 6585 — Additional HTTP Status Codes (§4 "429 Too Many Requests"); Token Bucket algorithm (classic network traffic shaping literature)
- URL: https://www.rfc-editor.org/rfc/rfc6585
- Tags: api-design, performance
- ThemisDB-Versionen: v1.6.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

Unbounded request rates from a single client can exhaust server resources and deny service to legitimate users. The token bucket algorithm is the standard mechanism for smoothing bursts while allowing short-term rate spikes: a bucket holds up to `capacity` tokens; tokens refill at a constant rate; each request consumes one token; when the bucket is empty, requests are rejected with HTTP 429. RFC 6585 standardises the `429 Too Many Requests` response status and the `Retry-After` header that tells clients how long to wait.

ThemisDB implements this pattern at two levels: a node-local in-process token bucket (per client-ID, stored in an `std::unordered_map` protected by a `shared_mutex`) and a distributed `RateLimiterV2` backend that uses Redis atomic scripts (EVAL) for cross-node rate limiting, introduced in v1.6.0.

## 🎯 Core Principles

- **Bucket capacity controls burst**: `capacity` tokens permit short bursts without penalty; sustained throughput is governed by the `refill_rate` (tokens per second).
- **Per-client granularity**: Each authenticated client (identified by API key or OAuth subject) has an independent bucket, preventing one noisy client from affecting others.
- **Atomic token deduction**: Token decrement and availability check must be atomic to prevent race conditions under concurrent requests from the same client.
- **RFC-compliant 429 response**: Rejected requests receive `HTTP 429 Too Many Requests` with a `Retry-After: <seconds>` header computed from the bucket refill time.
- **Graceful degradation to local limiter**: If the distributed Redis backend is unreachable, the system falls back to the node-local bucket rather than failing open (unlimited) or closed (total rejection).

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/server/` — Node-local `TokenBucket` class; HTTP middleware applies the limiter before routing; WebSocket handshake checks rate before upgrading.
- `src/server/rate_limiter_v2.cpp` — `RateLimiterV2` wraps a Redis connection pool; uses a Lua EVAL script for atomic "check-and-decrement" on a Redis sorted-set key per client.
- `src/server/http_server.cpp` — Middleware chain calls `RateLimiterV2::check(client_id, cost=1)` and converts `RateLimitExceeded` exception to `429` with `Retry-After`.

### What Was Adopted?

- Local `TokenBucket`: fields `double tokens_`, `double capacity_`, `double refill_rate_`, `std::chrono::steady_clock::time_point last_refill_`; `bool consume(double cost)` refills based on elapsed time, then atomically checks and decrements.
- `RateLimiterV2` Lua script:
  ```lua
  local tokens = tonumber(redis.call('GET', KEYS[1]) or ARGV[1])
  local refill  = tonumber(ARGV[2])
  tokens = math.min(tonumber(ARGV[1]), tokens + refill)
  if tokens >= 1 then redis.call('SET', KEYS[1], tokens - 1, 'EX', ARGV[3]) return 1 end
  return 0
  ```
- HTTP response on rejection: `status=429`, `Retry-After: <ceil(1/refill_rate)>`, `X-RateLimit-Limit`, `X-RateLimit-Remaining`, `X-RateLimit-Reset` headers.
- Admin API allows per-client bucket override (higher capacity for premium tiers).

### Deviations & Rationale

- **Sliding window not used**: Leaky bucket / sliding window algorithms provide stricter burst control. Token bucket was chosen because it is simpler to implement in a distributed Lua script and the burst allowance is a product requirement (short bursts are acceptable).
- **Redis TTL-based expiry**: Rather than storing the exact last-access timestamp, the Redis key TTL is set to `ceil(capacity / refill_rate)` seconds. This means buckets for idle clients expire automatically without a cleanup job, at the cost of some token accuracy near expiry.
- **Cost-based deduction not exposed in public API**: Although `consume(cost)` supports fractional costs, the public HTTP middleware always uses `cost=1`. Weighted rate limiting (e.g., expensive operations cost more) is planned for v2.2.0.

## ⚠️ Trade-offs & Limitations

- **Redis round-trip latency**: Each request to the distributed limiter adds ~0.5–2 ms of Redis round-trip. Under very high request rates (>10k req/s per node) this can become a bottleneck. Local bucket is used as primary; Redis is consulted only for cross-node enforcement.
- **Clock skew in distributed mode**: Refill calculations rely on wall clock time; Redis and application server clock skew can cause minor token accounting errors. For practical rate limiting (≥100 ms refill granularity) this is negligible.
- **No rate limit persistence across restarts**: Node-local buckets are in-memory; a server restart resets all local buckets, potentially allowing a brief burst post-restart. Redis-backed buckets survive restarts.
- **Abuse via rapid client rotation**: Sophisticated attackers can rotate client identifiers to get a fresh bucket. This is an application-layer concern (auth + fraud detection) rather than a rate-limiter concern.

## 🔬 Validation

- [x] Code reviewed against RFC 6585 §4 and token bucket literature
- [x] Unit tests in `tests/server/rate_limiter_test.cpp` verify burst allowance, sustained-rate enforcement, and 429 response headers
- [x] Load test (`benchmarks/server/rate_limit_bench.cpp`) confirms Redis path stays under 2 ms p99 at 5k req/s
- [x] Module README linked (`src/server/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [JWT Short-Lived Tokens](jwt_short_lived_tokens.md)
- [OpenTelemetry Tracing](opentelemetry_tracing.md)

---
**Last Updated:** 2026-04-06
