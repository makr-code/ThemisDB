# RAG Training Data Governance

## Overview

This document describes the dataset snapshot manifest and selection policy layer for managing reproducible, auditable RAG training data in ThemisDB. It implements Phase 2 of the DatasetSnapshot & Selection-Policy Layer, providing deterministic data selection, versioned policies, and strict train/val/test separation.

## Problem Statement

Training data quality and reproducibility are critical for reliable LLM fine-tuning. Previous iterations lacked:

- **Reproducibility**: No way to reconstruct which data was used for a training run
- **Auditability**: No lineage tracking from raw data → training data
- **Policy versioning**: No way to enforce consistent eligibility criteria
- **Split management**: Risk of leakage between train/val/test sets
- **Determinism**: Random splits made validation and comparison difficult

## Solution Architecture

### 1. Dataset Snapshot Manifest

A versioned, immutable record of a complete dataset snapshot used for training.

**Key Components:**

- **Metadata**: Snapshot ID, creation timestamp, checksums, configuration hashes
- **Eligibility Policy**: The exact criteria applied to select training data
- **Lineage Information**: Source documents, processing versions, transformations
- **Split Assignments**: Exact train/val/test split for each sample
- **Statistics**: Quality scores, domain distribution, split ratios
- **Audit Trail**: Complete record of all operations on this dataset

**Serialization:**

- JSON format for easy parsing and storage
- YAML format for human readability
- SHA-256 checksums for integrity verification

Example manifest:

```yaml
# ThemisDB Dataset Snapshot Manifest v1
snapshot_id: snapshot-001-training-run-42
name: Legal Domain Training Data v2
created_at: 2026-07-01T10:30:00Z
content_checksum: a1b2c3d4e5f6...

data_statistics:
  total_samples: 10000
  train_samples: 7000
  validation_samples: 1500
  test_samples: 1500
  avg_quality_score: 0.82
  avg_difficulty_score: 0.52

eligibility_policy:
  policy_version: "1.0"
  min_quality_score: 0.5
  max_difficulty_score: 0.95
  pii_handling: reject
  required_languages: [en]
  eligible_domains: [legal]
```

### 2. Eligibility Policy Engine

Enforces which samples are eligible for training based on versioned policies.

**Responsibilities:**

- Evaluate sample eligibility against policy criteria
- Track lineage of accepted samples
- Detect and prevent duplicates
- Maintain policy version history
- Audit all evaluation decisions

**Policy Criteria:**

```cpp
struct EligibilityPolicy {
    double min_quality_score;        // [0..1]
    double max_difficulty_score;     // [0..1]
    std::vector<std::string> required_languages;
    std::vector<std::string> eligible_domains;
    double dedup_threshold;          // Jaccard similarity threshold
    std::string pii_handling;        // "reject", "redact", "allow"
    bool toxicity_check_enabled;
    double max_toxicity_score;
};
```

**Usage Example:**

```cpp
// 1. Define policy
EligibilityPolicy policy;
policy.min_quality_score = 0.5;
policy.max_difficulty_score = 0.95;
policy.required_languages = {"en"};
policy.eligible_domains = {"legal"};

// 2. Create engine
EligibilityPolicyEngine engine(policy);

// 3. Evaluate samples
for (const auto& sample : raw_samples) {
    auto result = engine.evaluateSample(sample);
    if (result.is_eligible) {
        // Record lineage for accepted sample
        SampleLineage lineage;
        lineage.sample_id = sample.id;
        lineage.source_document_id = "doc-123";
        engine.recordSampleLineage(sample.id, lineage);
    } else {
        // Log rejection
        // Optionally implement remediation
    }
}

// 4. Access audit trail
auto log = engine.getAuditLog();
auto stats = engine.getEligibilityStatistics();
```

### 3. Dataset Split Manager

Generates deterministic, reproducible train/val/test splits with strict separation.

**Features:**

- **Deterministic Splits**: Same seed + same input = same split assignment
- **Stratified Sampling**: Optional stratification by domain or difficulty
- **K-Fold Support**: Cross-validation fold generation
- **Leakage Prevention**: Strict validation that no sample appears in multiple splits
- **Reproducibility**: Complete audit trail of split operations

**Split Configuration:**

```cpp
struct SplitConfig {
    double train_ratio = 0.7;           // [0..1]
    double validation_ratio = 0.15;     // [0..1]
    double test_ratio = 0.15;           // [0..1]
    uint64_t random_seed = 0;           // 0 = use timestamp
    bool stratify_by_domain = false;
    bool stratify_by_difficulty = false;
    uint32_t num_folds = 0;             // 0 = no cross-validation
    bool shuffle = true;
};
```

**Usage Example:**

```cpp
// 1. Configure splits
SplitConfig config;
config.train_ratio = 0.8;
config.validation_ratio = 0.1;
config.test_ratio = 0.1;
config.random_seed = 42;  // Fixed for reproducibility
config.stratify_by_domain = true;

// 2. Create manager
DatasetSplitManager manager(config);

// 3. Generate splits
auto result = manager.generateSplits(eligible_samples);

// 4. Verify integrity
if (!manager.verifySplitIntegrity(result)) {
    throw std::runtime_error("Split integrity check failed - possible leakage!");
}

// 5. Get split assignments
auto train = manager.getSamplesInSplit(result, "train");
auto val = manager.getSamplesInSplit(result, "validation");
auto test = manager.getSamplesInSplit(result, "test");

// 6. Export for reproducibility
manager.exportSplitsToJSON(result, "splits_run_42.json");
```

## Integration Workflow

### Complete Pipeline

```
1. Load raw data
   ↓
2. Apply DataSelectionPipeline (quality filtering, deduplication, scoring)
   ↓
3. Evaluate eligibility with EligibilityPolicyEngine
   ↓
4. Record lineage for accepted samples
   ↓
5. Generate splits with DatasetSplitManager
   ↓
6. Verify split integrity (no leakage)
   ↓
7. Create DatasetSnapshotManifest
   ↓
8. Save manifest and splits for reproducibility
   ↓
9. Begin training with tracked data provenance
```

### Example Code

```cpp
#include "training/dataset_snapshot_manifest.h"
#include "training/eligibility_policy_engine.h"
#include "training/dataset_split_manager.h"
#include "training/lora_data_selection.h"

// Step 1: Load and filter raw data
LoRADataSelectionConfig selection_config;
selection_config.min_length_tokens = 50;
selection_config.max_length_tokens = 10000;

DataSelectionPipeline pipeline(selection_config);
auto selection_result = pipeline.run(raw_samples);

// Step 2: Create eligibility policy
EligibilityPolicy eligibility_policy;
eligibility_policy.min_quality_score = 0.5;
eligibility_policy.max_difficulty_score = 0.95;
eligibility_policy.required_languages = {"en"};

// Step 3: Evaluate eligibility
EligibilityPolicyEngine policy_engine(eligibility_policy);
std::vector<DataSample> eligible_samples;

for (const auto& sample : selection_result.selected_samples) {
    auto eval_result = policy_engine.evaluateSample(sample);
    if (eval_result.is_eligible) {
        eligible_samples.push_back(sample);
        
        // Record lineage
        SampleLineage lineage;
        lineage.sample_id = sample.id;
        lineage.source_document_id = "doc-" + std::to_string(random());
        policy_engine.recordSampleLineage(sample.id, lineage);
    }
}

// Step 4: Generate splits
SplitConfig split_config;
split_config.train_ratio = 0.8;
split_config.validation_ratio = 0.1;
split_config.test_ratio = 0.1;
split_config.random_seed = 42;  // Fixed seed

DatasetSplitManager split_manager(split_config);
auto split_result = split_manager.generateSplits(eligible_samples);

// Verify no leakage
assert(split_manager.verifySplitIntegrity(split_result));

// Step 5: Create manifest
DatasetSnapshotManifest manifest;
manifest.snapshot_id = "snapshot-" + current_timestamp();
manifest.name = "Legal Domain Training Data v2";
manifest.eligibility_policy = eligibility_policy;
manifest.total_samples = eligible_samples.size();

auto stats = split_manager.getSplitStatistics(split_result);
manifest.train_samples = stats["train"];
manifest.validation_samples = stats["validation"];
manifest.test_samples = stats["test"];

// Add lineage information
for (size_t i = 0; i < eligible_samples.size(); ++i) {
    auto lineages = policy_engine.getLineageHistory(eligible_samples[i].id);
    if (!lineages.empty()) {
        manifest.sample_lineages.push_back(lineages[0]);
    }
}

// Add split assignments
for (const auto& assign : split_result.assignments) {
    manifest.split_assignments.push_back(assign);
}

// Compute checksums for integrity
manifest.updateChecksum();

// Step 6: Save for reproducibility
manifest.saveToFile("manifest_run_42.json");
split_manager.exportSplitsToJSON(split_result, "splits_run_42.json");

// Step 7: Begin training
auto train_samples = split_manager.getSamplesInSplit(split_result, "train");
// ... pass to LoRA trainer
```

## Key Design Principles

### 1. Reproducibility

- Fixed random seeds enable identical splits across runs
- All configuration is versioned and recorded
- Checksums enable integrity verification

### 2. Auditability

- Complete lineage tracking from raw data → training data
- All policy decisions are logged
- Rejection reasons and remediation suggestions provided

### 3. Determinism

- No randomness without explicit seeds
- Deterministic hashing for deduplication
- Predictable split algorithms

### 4. Strict Separation

- No sample can appear in multiple splits
- Validation checks enforce separation
- Clear error messages on leakage detection

### 5. Versioning

- Policy versions enable consistent application
- Policy history tracks evolution
- Manifests record exact policy used

## Quality Gate Checklist

Before using a dataset snapshot for training:

- [ ] Manifest created and checksum verified
- [ ] Split integrity check passed (no leakage)
- [ ] All samples have lineage records
- [ ] Policy version matches documented requirements
- [ ] Audit log contains no unexpected rejections
- [ ] Sample count distributions are reasonable
- [ ] Domain distribution matches expectations
- [ ] Quality/difficulty scores are within acceptable ranges

## API Reference

### DatasetSnapshotManifest

```cpp
class DatasetSnapshotManifest {
    // Serialization
    std::string toJSON() const;
    std::string toYAML() const;
    static DatasetSnapshotManifest fromJSON(const std::string& json_str);
    static DatasetSnapshotManifest fromYAML(const std::string& yaml_str);
    
    // File I/O
    bool saveToFile(const std::string& file_path) const;
    static DatasetSnapshotManifest loadFromFile(const std::string& file_path);
    
    // Integrity
    bool verifyIntegrity() const;
    void updateChecksum();
    
    // Statistics
    std::string getSplitStatistics() const;
    std::string getDomainStatistics() const;
};
```

### EligibilityPolicyEngine

```cpp
class EligibilityPolicyEngine {
    // Evaluation
    EligibilityResult evaluateSample(const DataSample& sample) const;
    
    // Lineage
    bool recordSampleLineage(const std::string& sample_id,
                            const SampleLineage& lineage);
    std::vector<SampleLineage> getLineageHistory(const std::string& sample_id) const;
    
    // Policies
    void updatePolicy(const EligibilityPolicy& new_policy);
    const EligibilityPolicy& getCurrentPolicy() const;
    std::vector<std::pair<std::string, EligibilityPolicy>> getPolicyHistory() const;
    
    // Audit
    std::vector<std::string> getAuditLog(size_t limit = 0) const;
    std::map<std::string, size_t> getEligibilityStatistics() const;
};
```

### DatasetSplitManager

```cpp
class DatasetSplitManager {
    // Split generation
    SplitResult generateSplits(const std::vector<DataSample>& samples);
    SplitResult generateSplitsFromIds(const std::vector<std::string>& sample_ids, ...);
    
    // Verification
    bool verifySplitIntegrity(const SplitResult& result) const;
    
    // Access
    std::vector<std::string> getSamplesInSplit(const SplitResult& result,
                                              const std::string& split_name) const;
    
    // Cross-validation
    SplitResult createCrossValidationFold(const SplitResult& result,
                                         uint32_t fold_index) const;
    
    // I/O
    bool exportSplitsToJSON(const SplitResult& result,
                           const std::string& file_path) const;
    static SplitResult importSplitsFromJSON(const std::string& file_path);
};
```

## Future Enhancements

1. **Machine Learning-based Difficulty Scoring**: Use actual perplexity models instead of heuristics
2. **Advanced Deduplication**: Use MinHash or semantic similarity instead of simple text hashing
3. **Stratified Sampling**: Implement true stratified sampling by multiple dimensions
4. **PII Redaction**: Integrate actual PII detection and redaction services
5. **Toxicity Detection**: Use production toxicity detection APIs
6. **Database Integration**: Store manifests and lineage in ThemisDB
7. **Distributed Split Management**: Handle splits across multiple data sources
8. **Policy Learning**: Automatically optimize policies based on training outcomes

## Troubleshooting

### "Duplicate sample detected"

**Problem**: Sample was rejected as duplicate.
**Solution**: Check if the sample is truly similar. If intentional, increase `dedup_threshold`.

### "Split integrity check failed"

**Problem**: Leakage detected between train/val/test sets.
**Solution**: This is a critical error - do not proceed with training. Regenerate splits.

### "Policy version mismatch"

**Problem**: Trying to mix samples with different policy versions.
**Solution**: All samples must be evaluated with the same policy version.

### "Lineage record not found"

**Problem**: Sample lacks lineage information.
**Solution**: All accepted samples must have lineage recorded before manifest creation.

## References

- Phase 1: DataSelectionPipeline (lora_data_selection.h)
- ProvenanceTracker (provenance_tracker.h) - for detailed lineage
- ProjectVersioning (project_versioning.h) - for snapshot infrastructure
