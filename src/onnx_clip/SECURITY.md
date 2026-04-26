> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — ONNX CLIP Plugin

## Threat Model

### 1. Malicious Image Input (Adversarial Images)
- **Risk:** A caller supplies crafted image bytes designed to exploit the ONNX model
  or the image decoding library (buffer overflow, integer overflow in JPEG decoder).
- **Mitigation:** Image decoding is delegated to the system image library (OpenCV /
  stb_image); all decoded tensors are explicitly shaped to [1, 3, 224, 224] before
  inference. Inputs exceeding this shape trigger a resize, not a buffer expansion.
- **Status:** ✅ Fixed-shape tensor prevents shape-based attacks

### 2. Untrusted ONNX Model File
- **Risk:** An attacker replaces the ONNX model file on disk with a crafted model
  containing malicious operators or oversized weights.
- **Mitigation:** ONNX model files should be stored in a read-only, access-controlled
  directory. SHA-256 hash verification on model load is planned for v0.3.0. Until then,
  operators must ensure model files are not writable by the ThemisDB process user.
- **Status:** ⚠️ Hash verification planned Q1 2027; operator path restriction required now

### 3. GPU Memory Exhaustion (CUDA / TensorRT)
- **Risk:** Large batch inputs or many concurrent callers exhaust GPU memory, causing
  OOM crashes affecting other GPU workloads on the same device.
- **Mitigation:** Input images are resized to 224×224 before inference; maximum tensor
  size is bounded. Batch sizes are capped at 64. The mutex prevents concurrent session
  runs from the same plugin instance.
- **Status:** ✅ Input size bounded; concurrent run protection via mutex

### 4. Model Output Leakage
- **Risk:** Embedding vectors may encode sensitive biometric or visual identity
  information (face embeddings) that could be used for re-identification.
- **Mitigation:** CLIP image embeddings are semantic, not biometric. However, operators
  storing embeddings of human faces in the ThemisDB vector index must conduct a DSGVO
  Art. 9 assessment and apply appropriate access controls on the vector collection.
- **Status:** ⚠️ Policy responsibility of the operator; not a code-level control

### 5. Denial of Service via Repeated Warmup or Health Checks
- **Risk:** A caller invokes `warmup()` or `healthCheck()` in a tight loop to starve
  inference threads.
- **Mitigation:** `warmup()` and `healthCheck()` use the same mutex as `generateEmbedding()`;
  they are serialised and do not bypass rate limiting at the API layer.
- **Status:** ✅ Mutex-serialised; rate limiting is caller/API-layer responsibility

---

## Security Controls Summary

| Control | Implementation | Status |
|---------|---------------|--------|
| Fixed-shape tensor preprocessing | 224×224 resize before inference | ✅ |
| Mutex-serialised session runs | `std::mutex` in `Impl` | ✅ |
| Batch size cap | 64 images maximum per batch call | ✅ |
| Model file access control | Operator responsibility (read-only path) | ⚠️ |
| Model integrity hash | Planned v0.3.0 | ❌ |

---

## Known Limitations

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| CLIP-SEC-01 | No SHA-256 verification of ONNX model file on load | High | Open (planned Q1 2027) |
| CLIP-SEC-02 | Face/biometric embedding storage policy is operator responsibility | Medium | Open (by design) |
| CLIP-SEC-03 | No per-caller rate limiting on embedding generation | Low | Open |
