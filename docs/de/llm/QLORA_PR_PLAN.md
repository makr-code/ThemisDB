# QLoRA/PEFT Integration - Pull Request Plan

**Datum:** 15. Januar 2026  
**Version:** 1.0  
**Related:** [QLORA_PEFT_RESEARCH_REPORT.md](QLORA_PEFT_RESEARCH_REPORT.md)

---

## Overview

Dieser PR-Plan definiert die konkrete Umsetzungsstrategie für die QLoRA/PEFT Integration in ThemisDB basierend auf den Erkenntnissen des Research Reports.

**Ziel:** Production-ready QLoRA Training in 3 Wochen (15 Arbeitstage)

**Approach:** Hybrid Python Training (Axolotl) + C++ Inference (llama.cpp)

---

## Sprint Plan

### Sprint 1: Foundation (Woche 1-2, 10 Tage)

**Ziel:** Kernkomponenten implementieren

#### PR #1: Python Training Wrapper
**Branch:** `feature/qlora-python-wrapper`  
**Aufwand:** 3 Tage  
**Owner:** Backend Team

**Files Created:**
```
src/python/themisdb_trainer/
├── __init__.py
├── axolotl_wrapper.py        # Axolotl API Wrapper
├── config_generator.py       # YAML Config Generator
├── data_connector.py         # ThemisDB → JSONL
├── training_monitor.py       # Status & Progress Tracking
└── README.md

requirements.txt              # axolotl, peft, bitsandbytes
```

**Key Features:**
- Axolotl CLI Wrapper
- Config Generation von ThemisDB Metadata
- Streaming Data Export
- Training Status Monitoring
- Error Handling & Logging

**Tests:**
```
tests/python/
├── test_axolotl_wrapper.py
├── test_config_generator.py
└── test_data_connector.py
```

**Acceptance Criteria:**
- [ ] Can generate Axolotl YAML config from ThemisDB metadata
- [ ] Can export JSONL from ThemisDB query
- [ ] Can start/stop training job
- [ ] Can monitor training progress
- [ ] Unit tests pass (>80% coverage)

**Dependencies:**
- Existing JSONL Exporter (already implemented)
- Python 3.9+
- CUDA 11.8+ (for testing)

---

#### PR #2: GGUF-ST Format Support
**Branch:** `feature/qlora-gguf-format`  
**Aufwand:** 2 Tage  
**Owner:** C++ Team

**Files Created/Modified:**
```
include/llm/formats/
├── gguf_reader.h             # NEW: GGUF format reader
├── gguf_writer.h             # NEW: GGUF format writer
└── format_converter.h        # NEW: Format conversion utilities

src/llm/formats/
├── gguf_reader.cpp
├── gguf_writer.cpp
└── format_converter.cpp

include/llm/lora_framework/
└── lora_adapter_manager.h    # MODIFIED: Add GGUF support
```

**Key Features:**
- GGUF File Format Parsing
- GGUF File Writing
- Metadata Extraction/Embedding
- Tensor Data Read/Write
- Signature Verification

**Tests:**
```
tests/
├── test_gguf_reader.cpp
├── test_gguf_writer.cpp
└── test_format_converter.cpp
```

**Acceptance Criteria:**
- [ ] Can read GGUF files from llama.cpp
- [ ] Can write GGUF files
- [ ] Can extract/embed metadata
- [ ] Can verify signatures
- [ ] Round-trip conversion (HF → GGUF → HF) preserves tensors
- [ ] Unit tests pass (>80% coverage)

**Dependencies:**
- llama.cpp (already integrated)
- Existing Adapter Manager

---

#### PR #3: AQL TRAIN Statement
**Branch:** `feature/qlora-aql-train`  
**Aufwand:** 2 Tage  
**Owner:** AQL Team

**Files Created/Modified:**
```
src/aql/parser/
├── train_statement.cpp       # NEW: TRAIN parser
└── parser.cpp                # MODIFIED: Add TRAIN keyword

include/aql/ast/
├── train_node.h              # NEW: AST node for TRAIN
└── ast_nodes.h               # MODIFIED: Include train_node

src/aql/executor/
└── train_executor.cpp        # NEW: TRAIN execution logic
```

**Syntax:**
```sql
TRAIN ADAPTER <adapter_id>
ON MODEL '<base_model>'
WITH METHOD '<method>'
USING (<select_query>)
HYPERPARAMETERS (<key>=<value>, ...);
```

**Example:**
```sql
TRAIN ADAPTER legal_qa_v2
ON MODEL 'mistralai/Mistral-7B-v0.1'
WITH METHOD 'qlora'
USING (
    SELECT question, answer, context
    FROM legal_knowledge
    WHERE quality_score > 0.8
      AND date > '2023-01-01'
)
HYPERPARAMETERS (
    lora_rank = 64,
    lora_alpha = 16,
    learning_rate = 2e-4,
    load_in_4bit = TRUE,
    max_steps = 1000
);
```

**Tests:**
```
tests/aql/
├── test_train_parser.cpp
├── test_train_executor.cpp
└── test_train_integration.cpp
```

**Acceptance Criteria:**
- [ ] Parses TRAIN statement correctly
- [ ] Validates hyperparameters
- [ ] Integrates with Python Training Wrapper
- [ ] Returns training job ID
- [ ] Unit tests pass

**Dependencies:**
- PR #1 (Python Training Wrapper)
- Existing AQL Parser

---

#### PR #4: Model Conversion Tools
**Branch:** `feature/qlora-conversion-tools`  
**Aufwand:** 2 Tage  
**Owner:** DevOps Team

**Files Created:**
```
tools/convert/
├── hf_to_gguf.py             # HuggingFace → GGUF converter
├── gguf_to_hf.py             # GGUF → HuggingFace converter
├── verify_conversion.py      # Conversion verification
├── batch_convert.sh          # Batch conversion script
└── README.md

scripts/
└── install_converters.sh     # Install conversion dependencies
```

**Key Features:**
- Automated HF → GGUF conversion
- Metadata preservation
- Tensor verification (hash check)
- Batch processing
- Error reporting

**Tests:**
```
tests/integration/
├── test_conversion_pipeline.py
└── test_conversion_accuracy.py
```

**Acceptance Criteria:**
- [ ] Can convert HF adapter to GGUF
- [ ] Can convert GGUF adapter to HF
- [ ] Metadata is preserved
- [ ] Tensor hashes match
- [ ] Integration tests pass

**Dependencies:**
- llama.cpp conversion scripts
- safetensors library
- PR #2 (GGUF Format Support)

---

#### PR #5: Integration Testing
**Branch:** `feature/qlora-integration-tests`  
**Aufwand:** 1 Tag  
**Owner:** QA Team

**Files Created:**
```
tests/integration/qlora/
├── test_end_to_end.py        # Full pipeline test
├── test_training_workflow.py # Training workflow
├── test_inference_workflow.py # Inference workflow
└── fixtures/
    ├── sample_config.yml
    ├── sample_data.jsonl
    └── sample_model/
```

**Test Scenarios:**
1. **Happy Path**: Export → Train → Convert → Load → Inference
2. **Error Handling**: Invalid config, OOM, conversion failure
3. **Performance**: Benchmark VRAM, latency, throughput
4. **Quality**: Verify output quality

**Acceptance Criteria:**
- [ ] End-to-end pipeline works
- [ ] Error scenarios handled gracefully
- [ ] Performance meets benchmarks
- [ ] Quality metrics pass

**Dependencies:**
- ALL previous PRs

---

### Sprint 2: Integration & API (Woche 3-4, 5 Tage)

#### PR #6: REST API Extensions
**Branch:** `feature/qlora-rest-api`  
**Aufwand:** 2 Tage  
**Owner:** Backend Team

**Files Created/Modified:**
```
include/api/handlers/
├── lora_training_handler.h   # NEW: Training API handler
└── lora_management_handler.h # MODIFIED: Add QLoRA metadata

src/api/handlers/
├── lora_training_handler.cpp
└── lora_management_handler.cpp

docs/api/
└── QLORA_API.md              # NEW: API documentation
```

**New Endpoints:**
```
POST   /api/v1/llm/train              # Start training job
GET    /api/v1/llm/train/{job_id}    # Get training status
DELETE /api/v1/llm/train/{job_id}    # Cancel training
GET    /api/v1/llm/train              # List all training jobs
```

**Enhanced Endpoints:**
```
GET    /api/v1/llm/lora/adapters/{id} # Add QLoRA metadata
POST   /api/v1/llm/lora/adapters      # Support QLoRA creation
```

**Tests:**
```
tests/api/
├── test_lora_training_api.cpp
└── test_qlora_metadata_api.cpp
```

**Acceptance Criteria:**
- [ ] Can start training via REST API
- [ ] Can monitor training progress
- [ ] Can cancel running training
- [ ] Can list all training jobs
- [ ] API tests pass

**Dependencies:**
- PR #1 (Python Training Wrapper)
- PR #3 (AQL TRAIN)

---

#### PR #7: Adapter Registry Updates
**Branch:** `feature/qlora-adapter-registry`  
**Aufwand:** 1 Tag  
**Owner:** Backend Team

**Files Modified:**
```
include/llm/lora_framework/
└── lora_adapter_manager.h    # Add QLoRA metadata fields

src/llm/lora_framework/
└── lora_adapter_manager.cpp

include/storage/
└── adapter_storage.h         # Schema migration

migrations/
└── 001_add_qlora_metadata.sql # NEW: DB migration
```

**Schema Changes:**
```sql
ALTER TABLE lora_adapters ADD COLUMN quantization_bits INTEGER DEFAULT 16;
ALTER TABLE lora_adapters ADD COLUMN quantization_type VARCHAR(20);
ALTER TABLE lora_adapters ADD COLUMN double_quantization BOOLEAN DEFAULT FALSE;
ALTER TABLE lora_adapters ADD COLUMN training_method VARCHAR(50) DEFAULT 'lora';
```

**Acceptance Criteria:**
- [ ] Migration runs successfully
- [ ] Can store/retrieve QLoRA metadata
- [ ] Backward compatible with existing adapters
- [ ] Migration tests pass

---

#### PR #8: Configuration & Documentation
**Branch:** `feature/qlora-config-docs`  
**Aufwand:** 2 Tage  
**Owner:** Docs Team

**Files Created:**
```
config/
└── qlora_defaults.yml        # Default QLoRA configuration

docs/en/llm/
├── QLORA_QUICKSTART.md       # Quick start guide
├── QLORA_ADVANCED.md         # Advanced topics
└── QLORA_TROUBLESHOOTING.md  # Troubleshooting guide

docs/de/llm/
├── QLORA_SCHNELLSTART.md     # German quick start
└── QLORA_ERWEITERT.md        # German advanced guide

examples/
├── qlora_training_example.py # Training example
└── qlora_inference_example.cpp # Inference example
```

**Documentation Topics:**
- Quick Start (5 minutes to first training)
- Configuration Reference
- Best Practices
- Troubleshooting
- API Reference
- Example Code

**Acceptance Criteria:**
- [ ] Documentation is complete
- [ ] Examples work
- [ ] Screenshots/diagrams included
- [ ] Reviewed by stakeholders

---

### Sprint 3: Hardening (Woche 5, 5 Tage)

#### PR #9: Monitoring & Logging
**Branch:** `feature/qlora-monitoring`  
**Aufwand:** 2 Tage  
**Owner:** DevOps Team

**Files Created:**
```
src/monitoring/
├── training_monitor.cpp      # Training metrics collector
└── training_logger.cpp       # Training event logger

include/monitoring/
├── training_monitor.h
└── training_logger.h

grafana/dashboards/
└── qlora_training.json       # Grafana dashboard

prometheus/
└── qlora_metrics.yml         # Prometheus rules
```

**Metrics:**
- Training loss (current, average, min)
- Training progress (steps, epochs)
- GPU utilization (VRAM, compute)
- Training speed (steps/sec, tokens/sec)
- Error rate
- Job queue length

**Logs:**
- Training started/completed/failed
- Configuration snapshot
- Error details with stack traces
- Performance warnings (OOM, slow training)

**Acceptance Criteria:**
- [ ] Metrics exported to Prometheus
- [ ] Logs structured (JSON)
- [ ] Grafana dashboard works
- [ ] Alerts configured

---

#### PR #10: Error Handling & Resilience
**Branch:** `feature/qlora-error-handling`  
**Aufwand:** 2 Tage  
**Owner:** Backend Team

**Files Modified:**
```
src/python/themisdb_trainer/
├── error_handler.py          # NEW: Error handling
└── checkpoint_manager.py     # NEW: Checkpoint resumption

src/api/handlers/
└── lora_training_handler.cpp # Add error recovery

config/
└── retry_policies.yml        # Retry configuration
```

**Error Scenarios:**
1. **OOM (Out of Memory)**
   - Detection: Monitor CUDA errors
   - Recovery: Reduce batch size, enable gradient checkpointing
   
2. **Training Instability**
   - Detection: Loss NaN/Inf, gradient explosion
   - Recovery: Reduce learning rate, gradient clipping
   
3. **Conversion Failure**
   - Detection: Hash mismatch, invalid tensors
   - Recovery: Retry conversion, fallback to HF format

4. **Checkpoint Corruption**
   - Detection: File integrity check
   - Recovery: Resume from previous checkpoint

**Acceptance Criteria:**
- [ ] OOM detection and recovery works
- [ ] Training can resume from checkpoint
- [ ] Conversion errors handled gracefully
- [ ] All error tests pass

---

#### PR #11: Performance Optimization
**Branch:** `feature/qlora-performance`  
**Aufwand:** 1 Tag  
**Owner:** Backend Team

**Optimizations:**
1. **Data Loading**
   - Parallel data loading (4-8 workers)
   - Prefetch next batch
   - Memory-mapped files

2. **Training**
   - Gradient accumulation (effective batch size)
   - Mixed precision (FP16/BF16)
   - Gradient checkpointing (for large models)

3. **Conversion**
   - Parallel tensor conversion
   - Stream processing (don't load full model)

4. **Inference**
   - KV cache optimization
   - Batch inference

**Benchmarks:**
```
Before Optimization:
- Training: 60 min (1000 steps)
- Conversion: 120 sec
- Inference: 34 ms/token

After Optimization:
- Training: 45 min (25% faster)
- Conversion: 60 sec (50% faster)
- Inference: 32 ms/token (6% faster)
```

**Acceptance Criteria:**
- [ ] Training 20%+ faster
- [ ] Conversion 40%+ faster
- [ ] Inference 5%+ faster
- [ ] No quality degradation

---

## Code Review Checklist

For each PR, reviewers should verify:

### Code Quality
- [ ] Follows ThemisDB coding standards
- [ ] Proper error handling
- [ ] Memory safety (no leaks, proper cleanup)
- [ ] Thread safety where applicable
- [ ] No hardcoded values (use config)

### Testing
- [ ] Unit tests (>80% coverage)
- [ ] Integration tests
- [ ] Performance benchmarks
- [ ] Edge cases covered
- [ ] Error scenarios tested

### Documentation
- [ ] API documentation (Doxygen/Sphinx)
- [ ] User guide updates
- [ ] Example code
- [ ] Changelog entry
- [ ] Migration guide (if breaking changes)

### Security
- [ ] Input validation
- [ ] No credential exposure
- [ ] Audit logging
- [ ] RBAC integration
- [ ] Dependency vulnerabilities checked

### Performance
- [ ] No regressions (benchmarks pass)
- [ ] Memory usage acceptable
- [ ] No blocking operations in critical paths

---

## Deployment Plan

### Phase 1: Alpha (Internal, Week 6)
**Environment:** Staging  
**Users:** 3-5 internal developers  
**Duration:** 1 week

**Goals:**
- Verify end-to-end workflow
- Collect initial metrics
- Fix critical bugs

**Success Criteria:**
- [ ] Can train adapter successfully
- [ ] Can load and query adapter
- [ ] No critical bugs
- [ ] Performance meets expectations

---

### Phase 2: Beta (Early Adopters, Week 7-8)
**Environment:** Beta channel  
**Users:** 20-50 early adopters  
**Duration:** 2 weeks

**Goals:**
- Gather production metrics
- Validate various use cases
- Collect user feedback
- Monitor resource usage

**Success Criteria:**
- [ ] 10+ successful training jobs
- [ ] Error rate < 5%
- [ ] Positive user feedback
- [ ] No data loss incidents

---

### Phase 3: GA (General Availability, Week 9)
**Environment:** Production  
**Users:** All users  
**Duration:** Ongoing

**Goals:**
- Stable production release
- Documentation complete
- Support channels ready

**Release Checklist:**
- [ ] All PRs merged
- [ ] Documentation published
- [ ] Marketing announcement ready
- [ ] Support team trained
- [ ] Monitoring dashboards live
- [ ] Rollback plan tested

---

## Rollback Strategy

**Feature Flag:**
```yaml
features:
  qlora_training: false  # Default: disabled
```

**Rollback Steps:**
1. Disable feature flag
2. Stop all running training jobs
3. Revert API changes (if needed)
4. Database rollback (if schema changed)
5. Notify users

**Rollback Triggers:**
- Error rate > 10%
- P1 bug discovered
- Performance degradation > 20%
- Security vulnerability

---

## Resource Requirements

### Development
- **Team:** 1 Backend Developer (full-time, 3 weeks)
- **Hardware:** 1x NVIDIA RTX 4090 or A6000 (for testing)
- **Budget:** €1.800 (GPU) + €15.000 (developer time) = €16.800

### Production
- **Additional Infrastructure:**
  - Python runtime environment
  - CUDA 11.8+ support
  - Additional storage: 100 GB (for models/adapters)
  - Monitoring (Prometheus/Grafana already available)

---

## Success Metrics

### Technical Metrics
- [ ] VRAM usage: < 10 GB (for Mistral-7B QLoRA)
- [ ] Training time: < 60 min (1000 steps, RTX 4090)
- [ ] Conversion time: < 120 sec
- [ ] Inference latency: < 40 ms/token
- [ ] Quality (BLEU): > 0.75

### Business Metrics
- [ ] Training jobs per week: > 50
- [ ] Successful training rate: > 95%
- [ ] User satisfaction: > 4.0/5.0
- [ ] Hardware cost reduction: > 50%

---

## Contact & Support

**For Questions:**
- **Technical:** GitHub Issues
- **Implementation:** Slack #themisdb-qlora
- **General:** themisdb-dev@example.com

**Code Owners:**
- Python Wrapper: @backend-team
- GGUF Format: @cpp-team
- AQL Integration: @aql-team
- API: @backend-team
- Documentation: @docs-team

---

**Document Version:** 1.0  
**Last Updated:** 6. April 2026  
**Status:** ✅ Ready for Implementation
