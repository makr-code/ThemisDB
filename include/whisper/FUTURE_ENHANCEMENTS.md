## include/whisper

### Scope
- Evolve transcription API contracts for richer metadata and diagnostics.

### Design Constraints
- Keep `IAudioBackend` and transcriber abstraction compatibility.

### Required Interfaces
- `WhisperPlugin`
- `IWhisperTranscriber`
- `IAudioChunkReader`
- `WhisperConfig`

### Implementation Notes
- Prefer additive metadata fields and explicit defaults.

### Test Strategy
- Contract tests for config parsing and WAV reader input edge cases.

### Performance Targets
- Keep API-level overhead negligible versus transcription workload.

### Security / Reliability
- Validate file-reading paths and configuration boundaries.