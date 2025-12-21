# ThemisDB LLM Python SDK

Python client library for ThemisDB LLM operations with Bearer Token authentication.

## Installation

```bash
pip install themis-llm
```

## Quick Start

```python
from themis_llm import ThemisLLMClient

# Initialize client with bearer token
client = ThemisLLMClient(
    base_url="http://localhost:8080",
    bearer_token="your-jwt-token"
)

# Simple inference
response = client.infer(
    prompt="What is ThemisDB?",
    model="mistral-7b",
    max_tokens=512
)
print(response.text)

# RAG inference
rag_response = client.rag(
    query="Explain legal clause",
    collection="legal_docs",
    top_k=5,
    lora="legal-qa"
)

# Streaming
for token in client.stream_infer(prompt="Tell me a story", model="mistral-7b"):
    print(token, end="", flush=True)

# Model management
client.load_model("mistral-7b", "/models/mistral-7b.gguf")
models = client.list_models()
```

## Features

- ✅ Bearer Token (JWT) authentication
- ✅ Inference, RAG, and embedding generation
- ✅ Real-time streaming with Server-Sent Events
- ✅ Model and LoRA management
- ✅ Statistics and health checks
- ✅ Comprehensive error handling
- ✅ Context manager support

## License

Apache 2.0
