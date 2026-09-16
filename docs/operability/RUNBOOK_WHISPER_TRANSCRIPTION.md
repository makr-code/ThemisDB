# RUNBOOK: Whisper Transcription — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Audio-Processing Platform Team Lead
**Purpose:** Triage and recover from whisper transcription failures, model-load errors, and performance degradation events
**Severity:** High (transcription failures degrade voice-to-text pipelines and audio-indexed search capability)
**Estimated Duration:** 5 min – 2 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB whisper transcription module (`src/whisper/`). The module has four primary surfaces:

1. **Model lifecycle** — model file validation, load, hot-reload, and SHA-256 integrity check (`WhisperConfig`, `WhisperPlugin`)
2. **Transcription pipeline** — audio chunk → VAD pre-filter → transcription → provenance stamp (`WhisperPlugin::transcribe()`)
3. **Audio ingestion** — WAV/MP3/OGG/FLAC chunk reading, RIFF validation, FFmpeg subprocess (`WavAudioChunkReader`, `FfmpegAudioChunkReader`)
4. **Streaming / diarisation** — incremental token callback, speaker attribution (`transcribeStream()`, `transcribeWithDiarisation()`)

**Key Principles:**
- The whisper plugin is fail-closed; malformed audio and unavailable models return `success=false` + `error_message`, not silent empty transcripts
- FFmpeg absence degrades gracefully; WAV path remains operational
- SHA-256 model integrity validation blocks load if hash mismatches configured value
- Thread-safety is enforced by `transcribe_mutex_`; concurrent callers are serialised, not dropped

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Access to server logs with `[WHISPER:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing `whisper_*` metrics
- [ ] Knowledge of which model file is configured (`WhisperConfig.model_path`) and whether FFmpeg is on PATH
- [ ] Plugin hot-plug state accessible via `WhisperPluginRegistrar` admin API or log scan

---

## Failure Scenarios

---

### Scenario 1: Model Load Failure

**Symptoms:**
- Log pattern: `[WHISPER:ModelLoadFailed] model_path=<path> reason=<reason>`
- All transcription calls return `success=false` with `error_message` containing "model not loaded"
- `whisper_model_load_failures_total` counter rising

**Log patterns:**
```
[WHISPER:ModelLoadFailed] model_path=/models/ggml-base.bin reason=file_not_found
[WHISPER:ModelLoadFailed] model_path=/models/ggml-base.bin reason=sha256_mismatch expected=<hash> actual=<hash>
[WHISPER:ModelLoadFailed] model_path=/models/ggml-base.bin reason=out_of_memory
```

#### Step 1: Confirm Model State
```bash
# Check model load log events
grep '\[WHISPER:ModelLoadFailed\]' /var/log/themisdb/themisdb.log | tail -20

# Verify model file exists and is readable
ls -lh /models/ggml-base.bin

# Check configured SHA-256 (if enabled)
grep 'model_sha256' /etc/themisdb/whisper_config.json
```

#### Step 2: Resolve by Failure Reason

**file_not_found:** Restore model file from artifact store:
```bash
aws s3 cp s3://themisdb-models/ggml-base.bin /models/ggml-base.bin
chmod 644 /models/ggml-base.bin
```

**sha256_mismatch:** Re-download the authoritative model artifact:
```bash
curl -fsSL https://artifact-store.internal/models/ggml-base.bin -o /models/ggml-base.bin
sha256sum /models/ggml-base.bin  # verify against config value
```

**out_of_memory:** Reduce model size or increase heap:
```bash
# Switch to smaller model variant
sed -i 's/ggml-large/ggml-base/' /etc/themisdb/whisper_config.json
themis-admin reload-plugin whisper
```

#### Step 3: Trigger Hot Reload
```bash
themis-admin plugin reload whisper
# Verify: log should show [WHISPER:ModelLoaded] within 10 s
```

**Escalation:** If model load fails after 3 attempts, open a P2 incident and engage the ML Infrastructure team.

---

### Scenario 2: Transcription Timeout

**Symptoms:**
- Log pattern: `[WHISPER:TranscriptionTimeout] audio_id=<id> elapsed_ms=<ms>`
- p99 transcription latency exceeds SLO (> 5 000 ms per chunk)
- `whisper_transcription_latency_p99_ms` metric above threshold

**Log patterns:**
```
[WHISPER:TranscriptionTimeout] audio_id=<id> elapsed_ms=6500 threshold_ms=5000
[WHISPER:TranscriptionTimeout] audio_id=<id> reason=mutex_contention threads_waiting=<N>
```

#### Step 1: Identify Contention
```bash
grep '\[WHISPER:TranscriptionTimeout\]' /var/log/themisdb/themisdb.log | tail -20

# Check concurrent transcription thread count
query-metrics --metric whisper_concurrent_transcriptions --range 5m
```

#### Step 2: Reduce Concurrency
If `threads_waiting` is high, reduce concurrent callers or switch to async callback mode:
```bash
# Adjust whisper worker pool size in config
themis-admin config set whisper.max_concurrent_transcriptions 2
themis-admin reload-plugin whisper
```

#### Step 3: Check Audio Chunk Size
Large audio chunks (> 30 s) increase transcription latency. Split audio server-side:
```bash
# Verify chunk duration in ingest config
grep 'max_chunk_duration_s' /etc/themisdb/whisper_config.json
# Reduce if > 30
```

---

### Scenario 3: Audio Corruption

**Symptoms:**
- Log pattern: `[WHISPER:AudioCorruption] audio_id=<id> reason=<reason>`
- Transcription results contain empty text with `success=false`
- `whisper_audio_corruption_total` counter non-zero

**Log patterns:**
```
[WHISPER:AudioCorruption] audio_id=<id> reason=invalid_riff_magic
[WHISPER:AudioCorruption] audio_id=<id> reason=truncated_wav_header
[WHISPER:AudioCorruption] audio_id=<id> reason=unsupported_sample_rate rate=<hz>
```

#### Step 1: Identify Corruption Source
```bash
grep '\[WHISPER:AudioCorruption\]' /var/log/themisdb/themisdb.log | tail -20

# Check for systematic issue (same source / same audio_id pattern)
grep '\[WHISPER:AudioCorruption\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'audio_id=\S+' | sort | uniq -c | sort -rn | head -10
```

#### Step 2: Validate Audio Source Pipeline
If systematic corruption is observed from a specific source:
```bash
# Re-validate ingest pipeline for affected source
themis-admin ingest validate-source --source-id <source_id>
```

#### Step 3: Re-route to FFmpeg Fallback
If WAV parser fails on specific encodings, force FFmpeg path:
```bash
themis-admin config set whisper.force_ffmpeg_reader true
themis-admin reload-plugin whisper
```

---

### Scenario 4: Benchmark Miss (Performance Regression)

**Symptoms:**
- Log pattern: `[WHISPER:BenchmarkMiss] benchmark=<name> observed_p99_ms=<ms> threshold_ms=<ms>`
- CI benchmark gate fails on `bench_whisper_transcription`
- Throughput below 5 000 ops/sec on reference hardware

**Log patterns:**
```
[WHISPER:BenchmarkMiss] benchmark=WhisperStubThroughput observed_p99_ms=12 threshold_ms=5
[WHISPER:BenchmarkMiss] benchmark=WavChunkReaderThroughput observed_p99_ms=8 threshold_ms=3
```

#### Step 1: Identify Regression Commit
```bash
git log --oneline -20 -- src/whisper/
# Run benchmark comparison
./build/benchmarks/whisper/bench_whisper_transcription \
  --benchmark_repetitions=5 --benchmark_format=json > /tmp/bench_current.json
python3 benchmarks/compare.py /tmp/bench_baseline.json /tmp/bench_current.json
```

#### Step 2: Profile Hot Path
```bash
perf record -g ./build/benchmarks/whisper/bench_whisper_transcription \
  --benchmark_filter=WavChunkReaderThroughput
perf report
```

#### Step 3: Restore Baseline or Accept Regression
- If regression is introduced by a known change, update baseline in `benchmarks/baselines/`
- If unexpected, revert the offending commit and re-run benchmark gate

---

### Scenario 5: Hot-Plug Reload Failure

**Symptoms:**
- Plugin manager reports `WhisperPluginAdapter` in `ERROR` state after hot-plug
- Log shows `[WHISPER:ModelLoadFailed]` during `enableHotPlug()` callback
- `WhisperPluginRegistrar::enableHotPlug()` returns false

**Log patterns:**
```
[WHISPER:ModelLoadFailed] context=hot_plug model_path=<path> reason=<reason>
[WHISPER:PluginReloadFailed] adapter=WhisperPluginAdapter reason=<reason>
```

#### Step 1: Check Hot-Plug State
```bash
themis-admin plugin status whisper
grep 'hot_plug\|WhisperPlugin' /var/log/themisdb/themisdb.log | tail -30
```

#### Step 2: Disable Hot-Plug Temporarily
```bash
themis-admin config set whisper.hot_plug_enabled false
themis-admin reload-plugin whisper
```

#### Step 3: Re-enable After Model Fix
After restoring the model file (see Scenario 1):
```bash
themis-admin config set whisper.hot_plug_enabled true
themis-admin reload-plugin whisper
```

---

## Alert → Runbook Mapping

| Alert Name | Log Pattern | Runbook Scenario |
|------------|-------------|------------------|
| `whisper_model_load_failed` | `[WHISPER:ModelLoadFailed]` | Scenario 1 |
| `whisper_transcription_timeout` | `[WHISPER:TranscriptionTimeout]` | Scenario 2 |
| `whisper_audio_corruption` | `[WHISPER:AudioCorruption]` | Scenario 3 |
| `whisper_benchmark_miss` | `[WHISPER:BenchmarkMiss]` | Scenario 4 |
| `whisper_hot_plug_failed` | `[WHISPER:PluginReloadFailed]` | Scenario 5 |

---

## Escalation Path

1. **L1 (Operator):** Apply runbook steps; resolve within 30 min
2. **L2 (SRE):** Escalate if model load or corruption persists across 3 attempts
3. **L3 (ML Infra / Audio Platform):** Engage for model integrity issues or systematic audio pipeline failures

---

## Related Documentation

- `src/whisper/ROADMAP.md` — Wave D operability items
- `src/whisper/README.md` — Module overview and API contract
- `include/whisper/whisper_plugin.h` — Plugin interface
- `tests/integration/test_whisper_transcription_soak.cpp` — Wave D soak tests
- `tests/whisper/test_whisper_highcardinality_stress.cpp` — Wave D stress tests
- `benchmarks/whisper/bench_whisper_transcription.cpp` — Performance gates
