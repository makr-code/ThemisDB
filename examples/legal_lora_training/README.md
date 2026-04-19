> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Legal LoRA Training Pipeline - Examples

This directory contains examples demonstrating the Legal LoRA Training Pipeline for ThemisDB.

## Files

### train_legal_lora.cpp
Complete end-to-end example showing:
- Multi-source document ingestion (HuggingFace + filesystem)
- Auto-labeling with Legal Modality Analyzer (PR #1)
- Knowledge graph enrichment
- LoRA adapter training
- Deployment with A/B testing

### test_auto_labeler_basic.cpp
Basic test demonstrating:
- Auto-labeler configuration
- Document labeling with German legal text
- Integration with `NlpTextAnalyzer::extractLegalModalities()`
- Sample generation with confidence scoring

## Building

To build the examples:

```bash
cmake -B build \
  -DTHEMIS_ENABLE_LEGAL_TRAINING=ON \
  -DTHEMIS_BUILD_EXAMPLES=ON

cmake --build build --target train_legal_lora
cmake --build build --target test_auto_labeler_basic
```

## Running

### Basic Auto-Labeler Test
```bash
./build/test_auto_labeler_basic
```

This will:
1. Create a LegalAutoLabeler instance
2. Process sample German legal text
3. Generate training samples with detected modal verbs
4. Display results

Expected output:
```
=== Legal Auto-Labeler Basic Test ===

Creating LegalAutoLabeler...
✓ LegalAutoLabeler created successfully

Testing document labeling...
✓ Document labeling completed
  Generated 3 training samples

Sample Training Data:
---------------------
Sample 1:
  Category: obligation
  Confidence: 0.95
  Input: Analyze the legal modality in: "Die Behörde muss die Genehmigung erteilen..."
  Output: Category: obligation, Deontic Logic: O(φ), Interpretation: Bindende Rechtspf...

...
```

### Full Pipeline
```bash
./build/train_legal_lora --config config/lora/legal_german_training.yaml
```

## Configuration

Configure data sources in `config/ingestion/sources.yaml`:

```yaml
filesystem_sources:
  - source_id: "my_docs"
    enabled: true
    location: "/path/to/legal/documents"
    priority: 10
    options:
      ocr_enabled: true
      ocr_language: "deu"
```

Configure training in `config/lora/legal_german_training.yaml`:

```yaml
lora:
  rank: 16
  alpha: 32.0

training:
  hyperparameters:
    learning_rate: 0.0003
    batch_size: 4
    num_epochs: 3
```

## Sample Data

For testing, you can create sample German legal documents:

```bash
mkdir -p sample_docs
cat > sample_docs/verwaltungsvorschrift.txt << 'EOF'
Die Behörde muss die Genehmigung erteilen, wenn alle Voraussetzungen erfüllt sind.
Sie soll die Entscheidung innerhalb von vier Wochen treffen.
Sie kann die Frist verlängern, wenn besondere Umstände vorliegen.
EOF
```

Then configure the filesystem ingester to use this directory.

## Documentation

- **Architecture**: `../../docs/LEGAL_LORA_TRAINING_PIPELINE.md`
- **Tutorial**: `../../docs/tutorials/CUSTOM_DOCUMENT_INGESTION.md`
- **API Reference**: See header files in `../../include/ingestion/` and `../../include/training/`

## Integration with PR #1

The auto-labeler uses PR #1's Legal Modality Analyzer to detect German modal verbs:

- **"muss"** → Binding obligation (O(φ))
- **"soll"** → Default rule (O_default(φ))
- **"kann"** → Discretionary permission (P(φ))

This analysis is performed by:
```cpp
analytics::NlpTextAnalyzer analyzer;
auto modalities = analyzer.extractLegalModalities(text, "de");
```

Each detected modality is converted into a training sample with:
- Input: The legal text containing the modal verb
- Output: Category, deontic logic, and interpretation
- Confidence: Normative strength [0.0, 1.0]

## Troubleshooting

### Build Errors

If you get linker errors:
```bash
# Ensure Legal Training is enabled
cmake -B build -DTHEMIS_ENABLE_LEGAL_TRAINING=ON
```

### Missing Analytics Module

If `NlpTextAnalyzer` is not found:
```bash
# Ensure analytics module is enabled
cmake -B build -DTHEMIS_ENABLE_ANALYTICS=ON
```

### No Samples Generated

If the auto-labeler generates no samples:
- Check that your documents contain German modal verbs ("muss", "soll", "kann")
- Verify language_code is set to "de"
- Check min_confidence threshold (lower if needed)

## Next Steps

1. Add your own legal documents to `sample_docs/`
2. Configure sources in `config/ingestion/sources.yaml`
3. Run ingestion: `./build/train_legal_lora --phase ingestion`
4. Run auto-labeling: `./build/train_legal_lora --phase labeling`
5. Review generated samples in database
6. Configure and run training

## Support

For issues or questions:
- See main documentation: `../../docs/LEGAL_LORA_TRAINING_PIPELINE.md`
- Check tutorials: `../../docs/tutorials/CUSTOM_DOCUMENT_INGESTION.md`
- Open issue: https://github.com/makr-code/ThemisDB/issues
