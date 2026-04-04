# Voice Module Troubleshooting & Tuning Guide

## Common Issues

### STT Fails / No Transcript Produced

**Symptoms:** `BatchItemResult.transcript` is empty; preprocessing `success = false`.

**Causes & Fixes:**

1. **Audio format mismatch** – Ensure audio is 16-bit PCM at 16 kHz mono.
   ```cpp
   opts.target_sample_rate = 16000;
   ```

2. **VAD too aggressive** – Lower the VAD threshold so speech is not filtered:
   ```cpp
   opts.vad_threshold = 0.3f; // default 0.5
   ```

3. **Low input volume** – Enable normalization:
   ```cpp
   opts.enable_normalization = true;
   opts.target_rms = 0.1f;
   ```

4. **Model not loaded** – Check `VoiceModelCache` for a cache miss, and verify the model path:
   ```cpp
   bool cached = cache.isCached("stt-en-v1");
   auto stats = cache.getStats();
   // stats.cache_misses indicates load failures
   ```

---

### TTS Fails / No Audio Output

**Symptoms:** `VoiceTTSCustomizer::synthesize()` returns empty result or error.

**Causes & Fixes:**

1. **Voice profile not set** – Always call `setVoiceProfile()` before synthesizing.
2. **SSML syntax error** – Validate SSML before passing to `synthesizeSSML()`.
3. **Rate/pitch out of range** – Keep `speaking_rate` in [0.25, 4.0] and `pitch` in [-20, 20].

---

### High Latency

**Symptoms:** `BatchItemResult.processing_time_ms` exceeds budget.

**Causes & Fixes:**

1. **Echo cancellation enabled unnecessarily:**
   ```cpp
   opts.enable_echo_cancellation = false; // saves ~20% CPU per frame
   ```

2. **Model not cached** – First-call load penalty. Pin hot models:
   ```cpp
   cache.pin("stt-en-v1");
   ```

3. **Batch size too small** – Increase to saturate CPU/GPU:
   ```cpp
   bp_cfg.default_batch_size = 16;
   ```

4. **Session context too large** – Reduce max turns:
   ```cpp
   cfg.max_context_turns = 10;
   ```

---

### Memory Issues

**Symptoms:** OOM crash; cache evicting models too aggressively.

**Causes & Fixes:**

1. **Cache memory limit too low:**
   ```cpp
   cache_cfg.max_memory_bytes = 8ULL * 1024 * 1024 * 1024; // 8 GB
   ```

2. **Too many sessions open simultaneously** – Enforce a limit:
   ```cpp
   cfg.max_sessions = 50;
   ```

3. **Hot audio storage accumulating** – Trigger tier demotion:
   ```cpp
   storage.applyTierPolicy();
   ```

4. **Manually evict a specific model:**
   ```cpp
   cache.evict("large-llm-v2");
   ```

---

### PII Not Redacted

**Symptoms:** Phone numbers, emails, or names appear in transcripts.

**Causes & Fixes:**

1. **Redaction not called** – Always pass transcripts through `VoiceSecurity::redactPII()`:
   ```cpp
   std::string safe = security.redactPII(raw_transcript);
   ```

2. **Custom PII patterns not registered** – Add domain-specific patterns:
   ```cpp
   security.addRedactionPattern("CASE_ID", "CASE-[0-9]{6}");
   ```

3. **Consent not granted** – Verify consent before processing:
   ```cpp
   if (!security.hasConsent(user_id, "transcription")) {
       // Block processing
   }
   ```

---

## Diagnostic Commands

### Dump cache statistics

```cpp
auto stats = cache.getStats();
std::cout << "hits=" << stats.cache_hits
          << " misses=" << stats.cache_misses
          << " hit_rate=" << stats.hit_rate
          << " memory_used=" << stats.total_memory_bytes << "\n";

// Full JSON detail
auto detail = cache.getDetailedStats();
std::cout << detail.dump(2) << "\n";
```

### Dump batch processor statistics

```cpp
auto js = processor.getStatistics();
std::cout << js.dump(2) << "\n";
// {"jobs_submitted":5,"items_processed":40,"items_failed":1}
```

### Dump session list

```cpp
auto session_stats = sessions.getStats();
std::cout << session_stats.dump(2) << "\n";
```

### Dump accessibility export statistics

```cpp
auto acc_stats = acc.getStatistics();
std::cout << acc_stats.dump(2) << "\n";
// {"exports_completed":3,"total_cues_generated":120}
```

### Dump error handler circuit breaker state

```cpp
auto err_stats = handler.getStats();
std::cout << err_stats.dump(2) << "\n";
```

---

## Performance Tuning

### Audio Preprocessing Settings

| Option | Default | Tuning Advice |
|--------|---------|--------------|
| `enable_noise_reduction` | `true` | Disable for clean studio audio |
| `enable_echo_cancellation` | `false` | Enable only for far-field mic |
| `enable_vad` | `true` | Keep enabled; reduces processing load |
| `vad_threshold` | `0.5` | Lower for quiet speakers (0.3–0.4) |
| `noise_reduction_strength` | `0.7` | Increase for noisy environments (0.8–0.9) |
| `target_rms` | `0.1` | Increase for louder output (0.15–0.2) |

### Circuit Breaker Configuration

The `VoiceErrorHandler` uses a circuit breaker to protect downstream STT/TTS services.

```cpp
themis::voice::CircuitBreakerConfig cb;
cb.failure_threshold   = 5;      // Open after 5 consecutive failures
cb.recovery_timeout_ms = 10000;  // Try again after 10s
cb.half_open_max_calls = 1;      // Probe with 1 call when half-open

handler.setCircuitBreakerConfig("stt", cb);
```

Recommended values per environment:

| Environment | failure_threshold | recovery_timeout_ms |
|-------------|------------------|---------------------|
| Production  | 3                | 30000               |
| Staging     | 5                | 10000               |
| Development | 10               | 2000                |

### Session Timeouts

```cpp
cfg.session_timeout_ms = 180000; // 3 minutes (production)
cfg.session_timeout_ms = 30000;  // 30 seconds (unit tests)
```

---

## Audio Quality Troubleshooting

### WER Too High

Word Error Rate (WER) > 0.3 indicates poor transcription quality.

**Checklist:**

1. Is the input audio at 16 kHz mono PCM? Resample if needed.
2. Is background noise suppressed? (`enable_noise_reduction = true`)
3. Is the speaker close to the microphone? Check `snr_db` in `AudioQualityMetrics`.
4. Does the SNR exceed 20 dB?

```cpp
auto metrics = processor.computeQualityMetrics(audio, 16000);
std::cout << "SNR=" << metrics.snr_db << " dB, quality=" << metrics.quality_label << "\n";
```

Target SNR thresholds:

| SNR (dB) | PESQ MOS | Quality Label |
|----------|----------|---------------|
| < 0      | 1.0      | poor          |
| 14       | 2.0      | fair          |
| 28       | ~3.0     | good          |
| 57       | 5.0      | excellent     |

### MOS Too Low

PESQ MOS < 2.5 indicates audio quality problems:

1. **Clipping** – Check `clipping_ratio`. If > 0.01, reduce input gain.
2. **Low RMS energy** – Check `rms_energy`. If < 0.005, audio is too quiet.
3. **Noise floor too high** – Enable stronger noise reduction.

```cpp
if (metrics.clipping_ratio > 0.01f) {
    // Reduce microphone gain
}
if (metrics.rms_energy < 0.005f) {
    // Signal too weak
}
```

---

## Load Testing

Use `VoiceBatchProcessor::runLoadTest()` to benchmark throughput:

```cpp
themis::voice::BatchAudioItem tmpl;
tmpl.item_id    = "load-test";
tmpl.audio_data = generate_test_audio(16000, 1.0f); // 1s of audio
tmpl.sample_rate = 16000;

auto summary = processor.runLoadTest(100, tmpl);
std::cout << "Completed=" << summary.completed_items
          << " Failed=" << summary.failed_items
          << " Elapsed=" << summary.elapsed_ms << "ms\n";
double throughput = 100.0 / (summary.elapsed_ms / 1000.0);
std::cout << "Throughput=" << throughput << " items/sec\n";
```

Typical targets (8-core CPU):

| Batch Size | Expected Throughput |
|------------|---------------------|
| 8          | 50–100 items/sec    |
| 32         | 150–300 items/sec   |
| 128        | 400–800 items/sec   |

---

## FAQ

**Q: Can I use the Voice Module without a GPU?**  
A: Yes. All components run on CPU. GPU acceleration is optional and controlled by `THEMIS_ENABLE_GPU`.

**Q: Does VoiceAccessibility require an external library?**  
A: No. All caption formats (VTT, SRT, plain text, HTML, JSON) are generated with pure C++ standard library code.

**Q: How do I add a custom caption format?**  
A: Subclass `VoiceAccessibility` or post-process the `JSON` format export.

**Q: How is PESQ calculated?**  
A: ThemisDB uses a linear approximation: `PESQ = clamp(1.0 + SNR_dB × 0.07, 1.0, 5.0)`. This is a no-dependency estimate; for ITU-T P.862 compliant scores, integrate an external PESQ library and set `pesq_mos` directly on `AudioQualityMetrics`.

**Q: How does the LRU cache handle memory limits?**  
A: When inserting a model that would exceed `max_memory_bytes`, the cache evicts the least recently used non-pinned models until sufficient space is available. Pinned models are never evicted automatically.

**Q: What happens when all models are pinned and the cache is full?**  
A: `insert()` will fail to evict and the new model will still be inserted (exceeding the soft limit). Avoid pinning all models simultaneously if memory is constrained.

**Q: Can sessions be persisted across restarts?**  
A: Not in the current implementation. Sessions are in-memory only. For persistence, serialize the session context to `VoiceAudioStorage` before shutdown.
