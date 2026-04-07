# LoRA Framework Documentation Hub

**Version:** 1.0  
**Date:** 2026-01-11  
**Status:** Complete

---

## Welcome to LoRA Framework Documentation

This is the central hub for all documentation related to the ThemisDB LoRA (Low-Rank Adaptation) framework and its applications, particularly **themis_help_lora**, the intelligent documentation assistant.

---

## 📚 Documentation Overview

### For End Users

| Document | Description | Audience |
|----------|-------------|----------|
| [**themis_help_lora User Guide**](THEMIS_HELP_LORA_USER_GUIDE.md) | How to use the documentation assistant | End Users |
| [Quick Start Guide](#quick-start) | Get started in 5 minutes | End Users |
| [FAQ](#faq) | Frequently asked questions | Everyone |

### For Developers

| Document | Description | Audience |
|----------|-------------|----------|
| [**LoRA Framework Developer Guide**](LORA_FRAMEWORK_DEVELOPER_GUIDE.md) | Complete API reference and architecture | Developers |
| [**Integration Examples**](LORA_INTEGRATION_EXAMPLES.md) | Code examples in C++, Python, REST, AQL | Developers |
| [**Training Guide**](LORA_TRAINING_GUIDE.md) | How to train and fine-tune adapters | ML Engineers |
| [**Scheduled Weights Guide**](LORA_SCHEDULED_WEIGHTS.md) | Time-based adapter scheduling and transitions | Developers |

### Reference Documentation

| Document | Description | Audience |
|----------|-------------|----------|
| [AQL Function Reference](../../../LORA_AQL_REFERENCE.md) | Complete AQL function reference | Developers |
| [Architecture Overview](../../../LLM_LORA_UNIFIED_ARCHITECTURE.md) | System architecture and design | Architects |
| [Usage Examples](../../../LORA_USAGE_EXAMPLES.md) | Practical examples and patterns | Developers |

---

## 🚀 Quick Start

### For End Users (Using themis_help_lora)

**5-Minute Setup:**

1. **Access the Web UI**
   - Navigate to your ThemisDB Admin UI
   - Click "Documentation Assistant" in the sidebar

2. **Ask a Question**
   ```
   Example: "How do I enable sharding in ThemisDB?"
   ```

3. **Get Your Answer**
   - Read the generated response
   - Copy code examples if provided

4. **Provide Feedback** (Optional)
   - Click 👍 if helpful
   - Click 👎 and provide correction if not

**That's it!** You're now using themis_help_lora.

### For Developers (Using LoRA Framework)

**Quick Integration:**

```cpp
#include "llm/lora_framework/lora_orchestrator.h"

// Initialize
LoRAOrchestrator::Config config;
config.db = database_wrapper;
auto orchestrator = std::make_shared<LoRAOrchestrator>(config);

// Create adapter
TrainingData data;
data.base_model = "llama-2-7b";
data.samples = {/* your training data */};

LoRAHyperparameters params;
params.rank = 8;
params.alpha = 16;

std::string job_id = orchestrator->createAdapter(
    "my_adapter", "llama-2-7b", data, params
);

// Done!
```

---

## 📖 Learning Paths

### Path 1: End User → Power User

1. Start with [themis_help_lora User Guide](THEMIS_HELP_LORA_USER_GUIDE.md)
2. Learn to provide effective feedback
3. Explore AQL queries (basic level)
4. Read FAQ for common questions

**Time Investment:** 30 minutes  
**Outcome:** Efficiently use documentation assistant

### Path 2: Developer → Integration

1. Read [Developer Guide](LORA_FRAMEWORK_DEVELOPER_GUIDE.md) (Architecture & Core Components)
2. Follow [Integration Examples](LORA_INTEGRATION_EXAMPLES.md) (C++ or REST API)
3. Test with sample code
4. Deploy to your application

**Time Investment:** 2-4 hours  
**Outcome:** Integrated LoRA framework in your app

### Path 3: ML Engineer → Training Expert

1. Read [Developer Guide](LORA_FRAMEWORK_DEVELOPER_GUIDE.md) (Full document)
2. Study [Training Guide](LORA_TRAINING_GUIDE.md)
3. Experiment with hyperparameters
4. Train your first adapter
5. Monitor and optimize

**Time Investment:** 4-8 hours  
**Outcome:** Can train and optimize LoRA adapters

---

## 🎯 Common Use Cases

### 1. Using themis_help_lora for Documentation Queries

**What you need:**
- Access to ThemisDB instance
- JWT token for authentication

**Resources:**
- [User Guide](THEMIS_HELP_LORA_USER_GUIDE.md)
- [REST API Examples](LORA_INTEGRATION_EXAMPLES.md#rest-api-examples)

### 2. Training a Custom Documentation Assistant

**What you need:**
- Training data (Q&A pairs)
- GPU with 16+ GB VRAM (recommended)
- Basic understanding of ML concepts

**Resources:**
- [Training Guide](LORA_TRAINING_GUIDE.md)
- [Hyperparameter Tuning](LORA_TRAINING_GUIDE.md#hyperparameter-tuning)

### 3. Integrating LoRA in Your Application

**What you need:**
- C++ development environment
- ThemisDB SDK
- Familiarity with LLMs

**Resources:**
- [Developer Guide](LORA_FRAMEWORK_DEVELOPER_GUIDE.md)
- [Integration Examples](LORA_INTEGRATION_EXAMPLES.md)

### 4. Building a Domain-Specific Assistant

**What you need:**
- Domain expertise
- Training data collection strategy
- Evaluation metrics

**Resources:**
- [Training Guide](LORA_TRAINING_GUIDE.md#preparing-training-data)
- [Best Practices](LORA_TRAINING_GUIDE.md#best-practices)

---

## 💡 Key Concepts

### What is LoRA?

**LoRA (Low-Rank Adaptation)** is a technique for efficiently fine-tuning Large Language Models:

✅ **Efficient**: Trains small adapter layers instead of entire model  
✅ **Fast**: Much faster than full fine-tuning  
✅ **Portable**: Adapters are small (10-100 MB vs GB for full models)  
✅ **Reversible**: Can switch between adapters easily

### What is themis_help_lora?

**themis_help_lora** is the first application of the LoRA framework:

- **Purpose**: Documentation assistant for ThemisDB
- **Training**: Trained on ThemisDB documentation
- **Learning**: Improves from user feedback
- **Goal**: Reduce hallucinations and improve accuracy

### Architecture Overview

```
┌─────────────────────────────────────────┐
│          Applications Layer              │
│  (themis_help_lora, custom assistants)  │
├─────────────────────────────────────────┤
│          LoRA Framework                  │
│  (Orchestrator, Training, Storage)      │
├─────────────────────────────────────────┤
│          LLM Infrastructure              │
│  (llama.cpp, Model Management)          │
├─────────────────────────────────────────┤
│          ThemisDB Core                   │
│  (Database, Storage, Security)          │
└─────────────────────────────────────────┘
```

---

## 🔧 API Quick Reference

### REST API

```bash
# Query documentation assistant
POST /api/v1/llm/docs/query
{
  "question": "How do I enable sharding?"
}

# Provide feedback
POST /api/v1/llm/docs/feedback
{
  "question": "...",
  "answer": "...",
  "feedback_type": "positive"
}

# List adapters
GET /api/v1/llm/lora/list

# Create adapter
POST /api/v1/llm/lora/create
{
  "adapter_id": "my_adapter",
  "base_model": "llama-2-7b",
  "training_data": {...},
  "hyperparameters": {...}
}
```

### AQL Functions

```aql
// Train adapter
LORA_TRAIN(adapter_id, base_model, data, params)

// Query with adapter
LORA_QUERY(base_model, adapter_id, question, options)

// Find similar adapters
LORA_SIMILAR(adapter_id, limit, threshold)

// Get adapter statistics
LORA_STATS(adapter_id, metrics)

// Get adapter lineage
LORA_LINEAGE(adapter_id, depth)

// Recommend best adapter
LORA_RECOMMEND(query, base_model, category, criteria)

// Find adaptation path
LORA_PATH(from_model, to_model, max_depth)
```

### C++ API

```cpp
// Orchestrator (Main Interface)
auto orchestrator = std::make_shared<LoRAOrchestrator>(config);
orchestrator->createAdapter(id, model, data, params);
orchestrator->getAdapter(id);
orchestrator->updateAdapter(id, data);
orchestrator->deleteAdapter(id);

// Application
auto assistant = std::make_shared<ThemisHelpLoRA>(config);
assistant->query(question, user_id);
assistant->addPositiveFeedback(q, a, user_id);
assistant->trainFromFeedback();
```

---

## 📊 Feature Matrix

### LoRA Framework Capabilities

| Feature | Status | Description |
|---------|--------|-------------|
| **Training** | ✅ Complete | Train adapters from data |
| **Incremental Training** | ✅ Complete | Retrain from feedback |
| **Multi-Model Support** | ✅ Complete | Support multiple base models |
| **Storage Backends** | ✅ Complete | Filesystem, ThemisDB, S3 |
| **Version Management** | ✅ Complete | Semantic versioning & rollback |
| **Security** | ✅ Complete | Encryption, signatures, audit logs |
| **REST API** | ✅ Complete | Full HTTP API |
| **AQL Integration** | ✅ Complete | 7 AQL functions |
| **Graph Support** | ✅ Complete | Adapter lineage tracking |
| **Vector Search** | ✅ Complete | Semantic similarity |

### themis_help_lora Features

| Feature | Status | Description |
|---------|--------|-------------|
| **Documentation Q&A** | ✅ Complete | Answer ThemisDB questions |
| **Feedback Collection** | ✅ Complete | Learn from corrections |
| **Auto-Retraining** | ✅ Complete | Retrain when threshold reached |
| **Version Management** | ✅ Complete | Track improvements |
| **Web UI** | ✅ Complete | Admin UI integration |
| **REST API** | ✅ Complete | Programmatic access |
| **AQL Functions** | ✅ Complete | Query from database |
| **Audit Logging** | ✅ Complete | Complete traceability |

---

## 🐛 Troubleshooting

### Common Issues

| Issue | Solution | Reference |
|-------|----------|-----------|
| Adapter not found | Check adapter ID and list available adapters | [User Guide](THEMIS_HELP_LORA_USER_GUIDE.md#troubleshooting) |
| Training fails | Validate training data, check hyperparameters | [Training Guide](LORA_TRAINING_GUIDE.md#troubleshooting) |
| Out of memory | Reduce batch size or enable gradient checkpointing | [Training Guide](LORA_TRAINING_GUIDE.md#issue-out-of-memory) |
| Slow inference | Enable caching, use quantization | [Developer Guide](LORA_FRAMEWORK_DEVELOPER_GUIDE.md#troubleshooting) |
| Authentication failed | Refresh JWT token | [User Guide](THEMIS_HELP_LORA_USER_GUIDE.md#issue-authentication-failed) |

---

## 📞 Getting Help

### Resources

1. **Documentation**: Start here - most questions are answered in the guides
2. **Examples**: Check [Integration Examples](LORA_INTEGRATION_EXAMPLES.md) for code
3. **FAQ**: See below for common questions
4. **GitHub Issues**: Report bugs or request features
5. **Community Forum**: Discuss with other users

### Support Channels

- 📧 **Email**: support@themisdb.io
- 💬 **Forum**: https://forum.themisdb.io
- 🐛 **Issues**: https://github.com/makr-code/ThemisDB/issues
- 📚 **Docs**: https://docs.themisdb.io

---

## ❓ FAQ

### General

**Q: What is the difference between LoRA framework and themis_help_lora?**  
A: LoRA framework is the infrastructure for creating and managing adapters. themis_help_lora is the first application built on that framework - a documentation assistant.

**Q: Do I need to understand machine learning to use themis_help_lora?**  
A: No! End users just need to ask questions and provide feedback. ML knowledge is only needed if you want to train custom adapters.

**Q: Can I create my own adapters?**  
A: Yes! Follow the [Training Guide](LORA_TRAINING_GUIDE.md) to learn how.

### Technical

**Q: What base models are supported?**  
A: Currently Llama-2 (7B, 13B, 70B) and Mistral (7B). More models can be added.

**Q: How much GPU memory do I need for training?**  
A: Minimum 16 GB for 7B models, 24+ GB recommended. See [Training Guide](LORA_TRAINING_GUIDE.md) for details.

**Q: Can I run without GPU?**  
A: Inference works on CPU (slower). Training requires GPU.

**Q: How long does training take?**  
A: Typically 15-60 minutes for 500-1000 samples on a single GPU.

### Deployment

**Q: Is themis_help_lora production-ready?**  
A: Yes! It includes security, audit logging, version management, and rollback capabilities.

**Q: How do I update an adapter in production?**  
A: Use canary deployment or A/B testing. See [Developer Guide](LORA_FRAMEWORK_DEVELOPER_GUIDE.md#best-practices).

**Q: What's the rollback strategy?**  
A: Automatic rollback if quality drops, or manual rollback to any previous version.

---

## 🗺️ Roadmap

### Current Version (v1.0)

✅ Complete LoRA framework  
✅ themis_help_lora application  
✅ REST API & AQL functions  
✅ Training pipeline  
✅ Documentation complete

### Planned Features (v1.1)

- [ ] Multi-language support (DE, FR, ES)
- [ ] Streaming responses
- [ ] Advanced caching strategies
- [ ] More base models (GPT, Claude)

### Future (v2.0)

- [ ] Multi-modal support (vision, audio)
- [ ] Federated learning
- [ ] AutoML for hyperparameter tuning
- [ ] Interactive training UI

---

## 📄 License

ThemisDB LoRA Framework is part of ThemisDB and follows the same license.

See [LICENSE](../../../LICENSE) for details.

---

## 📝 Contributing

We welcome contributions! See [CONTRIBUTING.md](../../../CONTRIBUTING.md) for guidelines.

**Important**: This project does not accept AI-generated pull requests. See guidelines for details.

---

## 🙏 Acknowledgments

- **llama.cpp**: Efficient LLM inference
- **LoRA**: Original paper and technique
- **ThemisDB Team**: Framework development
- **Community**: Feedback and testing

---

## 📚 Additional Resources

### Related Documentation

- [LLM Infrastructure](README.md)
- [Model Management](../../architecture/llm_management.md)
- [Security Guide](../../security/llm_security.md)
- [Performance Optimization](../../performance/llm_optimization.md)

### External Resources

- [LoRA Paper](https://arxiv.org/abs/2106.09685)
- [llama.cpp Documentation](https://github.com/ggerganov/llama.cpp)
- [Fine-tuning Best Practices](https://huggingface.co/docs/transformers/training)

---

**Welcome to the ThemisDB LoRA ecosystem!**

Whether you're a user asking documentation questions or a developer building custom adapters, we hope these guides help you get the most out of the framework.

**Happy coding!** 🚀

---

**Last Updated**: 2026-04-06  
**Version**: 1.0
