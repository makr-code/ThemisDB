> **Status:** Integrationsannahmen gegen aktuellen Sourcecode verifizieren. Abweichungen mit `<!-- TODO -->` markiert.

# LLM Integration - Drohnenbild-Analyse

## 📋 Übersicht

Integration von Large Language Models (LLMs) für automatische Bildbeschreibungen, Visual Question Answering (VQA) und kontextuelle Bildanalyse.

## 🔧 llama.cpp Setup

### Installation

\`\`\`bash
# llama.cpp Repository klonen
git clone https://github.com/ggerganov/llama.cpp
cd llama.cpp

# Kompilieren mit CUDA Support (optional)
make LLAMA_CUBLAS=1

# Python Bindings installieren
pip install llama-cpp-python
\`\`\`

### Model Download

\`\`\`bash
# LLaVA Model (Vision + Language)
wget https://huggingface.co/mys/ggml_llava-v1.5-7b/resolve/main/ggml-model-q5_k.gguf
wget https://huggingface.co/mys/ggml_llava-v1.5-7b/resolve/main/mmproj-model-f16.gguf

# CLIP Model (Image Embeddings)
wget https://huggingface.co/openai/clip-vit-base-patch32/resolve/main/pytorch_model.bin
\`\`\`

## 🤖 LLM-Backends

### 1. llama.cpp Backend

\`\`\`python
from llama_cpp import Llama
from llama_cpp.llama_chat_format import Llava15ChatHandler
from PIL import Image

class LlamaCppVision:
    """llama.cpp Backend für Vision-Language Models."""
    
    def __init__(
        self,
        model_path: str,
        clip_model_path: str,
        n_ctx: int = 2048,
        n_gpu_layers: int = -1
    ):
        # Initialize Chat Handler
        self.chat_handler = Llava15ChatHandler(
            clip_model_path=clip_model_path
        )
        
        # Initialize Model
        self.llm = Llama(
            model_path=model_path,
            chat_handler=self.chat_handler,
            n_ctx=n_ctx,
            n_gpu_layers=n_gpu_layers,  # -1 = all on GPU
            logits_all=True
        )
    
    def generate_description(
        self,
        image_path: str,
        prompt: str = "Describe this image in detail."
    ) -> str:
        """Generiert Bildbeschreibung."""
        # Encode image as data URI
        with open(image_path, "rb") as f:
            image_data = base64.b64encode(f.read()).decode("utf-8")
        
        data_uri = f"data:image/jpeg;base64,{image_data}"
        
        # Generate
        response = self.llm.create_chat_completion(
            messages=[
                {
                    "role": "system",
                    "content": "You are an assistant analyzing drone images."
                },
                {
                    "role": "user",
                    "content": [
                        {"type": "image_url", "image_url": {"url": data_uri}},
                        {"type": "text", "text": prompt}
                    ]
                }
            ]
        )
        
        return response["choices"][0]["message"]["content"]
\`\`\`
<!-- TODO: verify interface against current source -->

### 2. CLIP für Embeddings

\`\`\`python
import torch
import clip
from PIL import Image

class CLIPEmbedder:
    """CLIP für Image Embeddings."""
    
    def __init__(self, model_name: str = "ViT-B/32"):
        self.device = "cuda" if torch.cuda.is_available() else "cpu"
        self.model, self.preprocess = clip.load(model_name, self.device)
    
    def encode_image(self, image_path: str) -> list:
        """Generiert Embedding für Bild."""
        image = Image.open(image_path)
        image_input = self.preprocess(image).unsqueeze(0).to(self.device)
        
        with torch.no_grad():
            image_features = self.model.encode_image(image_input)
            image_features /= image_features.norm(dim=-1, keepdim=True)
        
        return image_features.cpu().numpy().tolist()[0]
\`\`\`
<!-- TODO: verify interface against current source -->

## 📝 Prompt Engineering

### Beschreibungs-Prompts

\`\`\`python
DESCRIPTION_PROMPTS = {
    "general": "Describe this drone image in detail. Include information about the terrain, objects, weather conditions, and any notable features.",
    
    "construction": "Analyze this construction site image. Identify equipment, building progress, materials visible, and any safety concerns.",
    
    "agriculture": "Describe this agricultural field. Include crop type, growth stage, irrigation, and any visible issues like pest damage or nutrient deficiencies.",
    
    "disaster": "Analyze this disaster area image. Identify damage types, affected structures, accessibility, and priority areas for emergency response."
}
\`\`\`

### VQA (Visual Question Answering)

\`\`\`python
def generate_vqa_prompt(question: str, context: dict = None) -> str:
    """Generiert VQA Prompt."""
    base_prompt = f"Answer the following question about this image: {question}"
    
    if context:
        base_prompt += f"\n\nContext: {context.get('description', '')}"
    
    base_prompt += "\n\nProvide a concise, factual answer."
    
    return base_prompt
\`\`\`

## 🎯 Use Cases

### 1. Automatische Bildkategorisierung

\`\`\`python
class ImageClassifier:
    """LLM-basierte Bildklassifizierung."""
    
    def __init__(self, llm):
        self.llm = llm
        self.categories = [
            "urban", "rural", "construction", "agriculture",
            "water", "forest", "disaster", "infrastructure"
        ]
    
    def classify(self, image_path: str) -> dict:
        """Klassifiziert Bild."""
        prompt = f"Classify this image into one of these categories: {', '.join(self.categories)}. Only respond with the category name."
        
        response = self.llm.generate_description(image_path, prompt)
        category = response.strip().lower()
        
        # Validate
        if category not in self.categories:
            category = "unknown"
        
        return {
            "category": category,
            "confidence": 0.8,  # Would need calibration
            "raw_response": response
        }
\`\`\`
<!-- TODO: verify interface against current source -->

### 2. Change Detection

\`\`\`python
class ChangeDetector:
    """Erkennt Änderungen zwischen Bildern."""
    
    def __init__(self, llm):
        self.llm = llm
    
    def detect_changes(
        self,
        image_before: str,
        image_after: str
    ) -> dict:
        """Erkennt Änderungen."""
        # Beschreibe beide Bilder
        desc_before = self.llm.generate_description(
            image_before,
            "Describe this image focusing on structures and major features."
        )
        
        desc_after = self.llm.generate_description(
            image_after,
            "Describe this image focusing on structures and major features."
        )
        
        # Vergleiche
        comparison_prompt = f"Compare these two descriptions and identify changes:\n\nBefore: {desc_before}\n\nAfter: {desc_after}\n\nList the key changes."
        
        # Note: This would need a text-only model for comparison
        changes = self._compare_descriptions(desc_before, desc_after)
        
        return {
            "description_before": desc_before,
            "description_after": desc_after,
            "changes": changes
        }
\`\`\`
<!-- TODO: verify interface against current source -->

## ⚡ Performance Optimization

### Model Quantization

\`\`\`python
# Verwende quantisierte Modelle für schnellere Inference
# q4_0: 4-bit quantization (kleinster, schnellster)
# q5_k: 5-bit quantization (balance)
# f16: 16-bit float (höchste Qualität, langsamster)

llm = Llama(
    model_path="model-q5_k.gguf",  # 5-bit quantized
    n_gpu_layers=-1,  # Use GPU
    n_ctx=2048
)
\`\`\`

### Batch Processing

\`\`\`python
class BatchLLMProcessor:
    """Batch-Verarbeitung von Bildern."""
    
    def __init__(self, llm, batch_size: int = 4):
        self.llm = llm
        self.batch_size = batch_size
    
    async def process_batch(self, image_paths: list) -> list:
        """Verarbeitet Batch von Bildern."""
        results = []
        
        for i in range(0, len(image_paths), self.batch_size):
            batch = image_paths[i:i + self.batch_size]
            
            # Process batch in parallel
            tasks = [
                self._process_single(path)
                for path in batch
            ]
            
            batch_results = await asyncio.gather(*tasks)
            results.extend(batch_results)
        
        return results
\`\`\`

### Caching

\`\`\`python
from functools import lru_cache
import hashlib

class CachedLLM:
    """LLM mit Caching."""
    
    def __init__(self, llm):
        self.llm = llm
        self.cache = {}
    
    def generate_description(
        self,
        image_path: str,
        prompt: str
    ) -> str:
        """Generiert Beschreibung mit Caching."""
        # Cache key: Hash von Bild + Prompt
        with open(image_path, 'rb') as f:
            image_hash = hashlib.md5(f.read()).hexdigest()
        
        prompt_hash = hashlib.md5(prompt.encode()).hexdigest()
        cache_key = f"{image_hash}_{prompt_hash}"
        
        if cache_key in self.cache:
            return self.cache[cache_key]
        
        # Generate
        result = self.llm.generate_description(image_path, prompt)
        self.cache[cache_key] = result
        
        return result
\`\`\`

## 🎓 Best Practices

1. **Model Selection**
   - LLaVA 7B: Gute Balance, schnell
   - LLaVA 13B: Bessere Qualität, langsamer
   - LLaVA 34B: Beste Qualität, sehr langsam

2. **Prompt Design**
   - Sei spezifisch
   - Gib Kontext
   - Verwende Few-Shot Examples

3. **Performance**
   - Nutze Quantisierung (q5_k empfohlen)
   - GPU-Beschleunigung aktivieren
   - Batch-Processing für mehrere Bilder

4. **Quality Control**
   - Validiere Outputs
   - Confidence Scores nutzen
   - Human-in-the-Loop für kritische Entscheidungen

## 📚 Weitere Dokumentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - System-Design
- [IMAGE_PROCESSING.md](IMAGE_PROCESSING.md) - CV Pipeline
- [PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md) - Optimierung
