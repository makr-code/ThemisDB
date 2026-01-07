# GitHub Issue Templates

This directory contains issue templates for ThemisDB feature requests and bug reports.

## Available Templates

### Feature Templates (from FEATURE_PROPOSALS_V1.4.md)

#### Phase 1 (v1.4.0) - High Priority

1. **[RAID 6 (Dual Parity)](01_raid6_dual_parity.md)**
   - Tolerates 2 simultaneous failures
   - 75% storage efficiency
   - Effort: 3-4 weeks

2. **[LoRA Quantization (INT8/INT4)](02_lora_quantization.md)**
   - 4-8× memory reduction
   - Load more adapters simultaneously
   - Effort: 3-4 weeks

3. **[Hot Spare Management](03_hot_spare_management.md)**
   - Automatic failover <5 seconds
   - Zero-downtime recovery
   - Effort: 2-3 weeks

4. **[Multi-GPU LoRA Support](04_multi_gpu_lora.md)**
   - Distributed inference across GPUs
   - Linear scaling with GPU count
   - Effort: 5-6 weeks

#### Phase 2 (v1.5.0) - Medium Priority

5. **[GPU-Accelerated Erasure Coding](05_gpu_erasure_coding.md)**
   - 10-50× speedup with CUDA/OpenCL
   - Offload CPU-intensive operations
   - Effort: 4-5 weeks

#### Phase 3 (v1.6.0) - Strategic

6. **[Predictive Failure Detection](06_predictive_failure_detection.md)**
   - ML-based failure prediction
   - Proactive shard replacement
   - Effort: 6-8 weeks

### General Templates

7. **[Bug Report](bug_report.md)** - Report bugs and issues
8. **[Feature Request](feature_request.md)** - Suggest new features

## Using These Templates

### Creating Issues from Templates

1. Go to https://github.com/makr-code/ThemisDB/issues/new/choose
2. Select the appropriate template
3. Fill in the required information
4. Submit the issue

### Manual Issue Creation

You can also create issues manually using the `gh` CLI:

```bash
# Example: Create RAID 6 feature issue
gh issue create \
  --title "[v1.4.0] Implement RAID 6 (Dual Parity) Support" \
  --label "enhancement,raid,high-priority,v1.4.0" \
  --body-file .github/ISSUE_TEMPLATE/01_raid6_dual_parity.md

# Example: Create LoRA Quantization feature issue
gh issue create \
  --title "[v1.4.0] Implement LoRA Quantization (INT8/INT4)" \
  --label "enhancement,lora,high-priority,v1.4.0" \
  --body-file .github/ISSUE_TEMPLATE/02_lora_quantization.md
```

### Batch Issue Creation

Create all feature issues at once:

```bash
# Navigate to repository root
cd /path/to/ThemisDB

# Create Phase 1 (v1.4.0) issues
gh issue create --title "[v1.4.0] Implement RAID 6 (Dual Parity) Support" \
  --label "enhancement,raid,high-priority,v1.4.0" \
  --body-file .github/ISSUE_TEMPLATE/01_raid6_dual_parity.md

gh issue create --title "[v1.4.0] Implement LoRA Quantization (INT8/INT4)" \
  --label "enhancement,lora,high-priority,v1.4.0" \
  --body-file .github/ISSUE_TEMPLATE/02_lora_quantization.md

gh issue create --title "[v1.4.0] Implement Hot Spare Management System" \
  --label "enhancement,raid,operations,high-priority,v1.4.0" \
  --body-file .github/ISSUE_TEMPLATE/03_hot_spare_management.md

gh issue create --title "[v1.4.0] Implement Multi-GPU LoRA Support" \
  --label "enhancement,lora,gpu,high-priority,v1.4.0" \
  --body-file .github/ISSUE_TEMPLATE/04_multi_gpu_lora.md

# Create Phase 2 (v1.5.0) issues
gh issue create --title "[v1.5.0] Implement GPU-Accelerated Erasure Coding" \
  --label "enhancement,raid,performance,v1.5.0" \
  --body-file .github/ISSUE_TEMPLATE/05_gpu_erasure_coding.md

# Create Phase 3 (v1.6.0) issues
gh issue create --title "[v1.6.0] Implement Predictive Failure Detection" \
  --label "enhancement,operations,ml,v1.6.0" \
  --body-file .github/ISSUE_TEMPLATE/06_predictive_failure_detection.md
```

## Template Structure

Each feature template includes:

- **Description**: What the feature does
- **Motivation**: Why it's needed
- **Implementation**: Technical approach and API design
- **Success Metrics**: Measurable goals
- **Use Cases**: Real-world scenarios
- **Estimated Effort**: Time and resources needed
- **Priority**: High/Medium/Low
- **References**: Links to documentation and research
- **Acceptance Criteria**: Definition of done

## Labels

Templates use the following label conventions:

- **Type**: `enhancement`, `bug`
- **Component**: `raid`, `lora`, `operations`, `gpu`, `ml`
- **Priority**: `high-priority`, `medium-priority`, `low-priority`
- **Version**: `v1.4.0`, `v1.5.0`, `v1.6.0`
- **Status**: `in-progress`, `blocked`, `needs-review`

## Customization

To create a new template:

1. Copy an existing template
2. Update the frontmatter (name, about, title, labels)
3. Fill in the template sections
4. Test by creating a sample issue

## Related Documentation

- [FEATURE_PROPOSALS_V1.4.md](../../FEATURE_PROPOSALS_V1.4.md) - Complete feature roadmap
- [CODE_REVIEW_REPORT.md](../../CODE_REVIEW_REPORT.md) - Code quality assessment
- [RAID Quick Start Guide](../../docs/en/guides/RAID_QUICK_START_GUIDE.md)
- [LoRA Adapter Guide](../../docs/en/guides/LORA_ADAPTER_GUIDE.md)

## Contributing

When adding new templates:

1. Follow the existing template structure
2. Include all required sections
3. Add meaningful examples
4. Link to relevant documentation
5. Update this README

## Support

For questions about issue templates:

- 📖 [Documentation](https://github.com/makr-code/ThemisDB/tree/main/docs)
- 💬 [Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 📊 [Feature Roadmap](https://github.com/makr-code/ThemisDB/blob/main/FEATURE_PROPOSALS_V1.4.md)
