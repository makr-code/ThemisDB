> **Status:** Integrationsannahmen gegen aktuellen Sourcecode verifizieren. Abweichungen mit `<!-- TODO -->` markiert.

# Multi-AI Backend Architecture

## Überblick

Das Moral Philosophy Debates System unterstützt mehrere AI-Backends, um verschiedene philosophische Perspektiven optimal zu repräsentieren. Die Integration von ThemisDB's embedded llama.cpp-Engine mit Cloud-APIs ermöglicht eine flexible, kosteneffiziente und hochqualitative Debattenführung.

## Architektur

```
┌─────────────────────────────────────────────────────────────┐
│                  MultiAIOrchestrator                         │
│  - Backend Selection Logic                                   │
│  - Fallback Chain Management                                │
│  - Cost Tracking & Optimization                             │
│  - Response Aggregation                                      │
└────────────────┬────────────────────────────────────────────┘
                 │
     ┌───────────┴───────────┬──────────────┬─────────────┬──────────┐
     │                       │              │             │          │
┌────▼──────┐  ┌─────────▼───────┐  ┌─────▼──────┐  ┌───▼──────┐ ┌▼───────────┐
│  llama.cpp │  │     Claude      │  │   GPT-4    │  │ Mistral  │ │  Ollama    │
│ (ThemisDB) │  │  (Anthropic)    │  │  (OpenAI)  │  │  (API)   │ │  (Local)   │
│   LOCAL    │  │     CLOUD       │  │   CLOUD    │  │  CLOUD   │ │   LOCAL    │
└────────────┘  └─────────────────┘  └────────────┘  └──────────┘ └────────────┘
     │                  │                  │               │             │
     └──────────────────┴──────────────────┴───────────────┴─────────────┘
                                    │
                         ┌──────────▼───────────┐
                         │ Philosophy Profiles  │
                         │  (37 Schools)        │
                         └──────────────────────┘
```

## Unterstützte Backends

### 1. ThemisDB llama.cpp (Primary, Local)

**Status**: ✅ Embedded in ThemisDB

**Vorteile**:
- Keine API-Kosten
- Vollständige Privatsphäre
- Keine Netzwerklatenz
- GPU-Beschleunigung
- Offline-Fähigkeit

**Modelle**:
- Llama-2 (7B, 13B, 70B)
- Mistral (7B)
- Phi-2/3
- Gemma
- CodeLlama
- Custom fine-tuned models

**API-Endpunkt**:
```
POST http://localhost:8529/api/llm/generate
{
  "model": "llama-2-7b-chat",
  "prompt": "...",
  "max_tokens": 1000,
  "temperature": 0.7
}
```

**Performance**:
- Latenz: 50-200ms (GPU)
- Throughput: 50-100 tokens/sec
- VRAM: 4-16GB je nach Modell

**Integration mit ThemisDB**:
```python
from llm_backends import LlamaCppBackend, BackendConfig, BackendType

config = BackendConfig(
    backend_type=BackendType.LLAMACPP,
    model_name="llama-2-7b-chat",
    api_url="http://localhost:8529/api/llm",
    max_tokens=1000,
    temperature=0.7
)

backend = LlamaCppBackend(config)
response = backend.generate(
    prompt="Analysiere die ethischen Implikationen...",
    system_prompt="Du bist ein Kantischer Ethiker."
)

print(response.text)
print(f"Latenz: {response.latency_ms:.0f}ms")
print(f"Kosten: ${response.cost_usd:.4f}")  # Always $0.00
```

### 2. Claude (Anthropic)

**Status**: ✅ API Integration

**Vorteile**:
- Constitutional AI (spezialisiert auf Ethik)
- Excellente Reasoning-Qualität
- Lange Context Windows (200K tokens)
- Harmfulness Reduction

**Modelle**:
- claude-3-opus-20240229 (Best Quality)
- claude-3-sonnet-20240229 (Balanced)
- claude-3-haiku-20240307 (Fast & Cheap)

**Kosten** (Stand 2024):
| Modell | Input | Output |
|--------|-------|--------|
| Opus   | $15/M | $75/M  |
| Sonnet | $3/M  | $15/M  |
| Haiku  | $0.25/M | $1.25/M |

**Integration**:
```python
from llm_backends import ClaudeBackend, BackendConfig, BackendType

config = BackendConfig(
    backend_type=BackendType.CLAUDE,
    model_name="claude-3-sonnet-20240229",
    api_key=os.getenv("ANTHROPIC_API_KEY"),
    max_tokens=1000,
    temperature=0.7,
    cost_per_1k_input=0.003,
    cost_per_1k_output=0.015
)

backend = ClaudeBackend(config)
response = backend.generate(prompt, system_prompt)
```

**Best Use Cases**:
- Komplexe ethische Dilemmata
- Nuancierte Abwägungen
- Constitutional AI Alignment
- Lange philosophische Texte

### 3. GPT-4 (OpenAI)

**Status**: ✅ API Integration

**Vorteile**:
- Breites philosophisches Wissen
- Starke allgemeine Reasoning-Fähigkeiten
- Multimodale Capabilities (Vision)
- Große Community und Ressourcen

**Modelle**:
- gpt-4-turbo (128K context)
- gpt-4 (8K context)
- gpt-4-vision (Multimodal)

**Kosten**:
| Modell | Input | Output |
|--------|-------|--------|
| GPT-4 Turbo | $10/M | $30/M |
| GPT-4 | $30/M | $60/M |

**Integration**:
```python
from llm_backends import GPT4Backend, BackendConfig, BackendType

config = BackendConfig(
    backend_type=BackendType.GPT4,
    model_name="gpt-4-turbo",
    api_key=os.getenv("OPENAI_API_KEY"),
    max_tokens=1000,
    temperature=0.7,
    cost_per_1k_input=0.01,
    cost_per_1k_output=0.03
)

backend = GPT4Backend(config)
response = backend.generate(prompt, system_prompt)
```

**Best Use Cases**:
- Breitgefächerte philosophische Analysen
- Interdisziplinäre Verbindungen
- Schnelle Prototyping
- Vergleichsbaseline

### 4. Mistral (European Alternative)

**Status**: ✅ API Integration

**Vorteile**:
- Europäische Datenschutz-Standards
- Open-Weights-Modelle
- Wettbewerbsfähige Pricing
- Mehrsprachig (Deutsch, Französisch)

**Modelle**:
- mistral-large-latest
- mistral-medium
- mistral-small

**Kosten**:
| Modell | Input | Output |
|--------|-------|--------|
| Large  | €10/M | €30/M  |
| Medium | €5/M  | €15/M  |
| Small  | €1/M  | €3/M   |

**Integration**:
```python
from llm_backends import MistralBackend, BackendConfig, BackendType

config = BackendConfig(
    backend_type=BackendType.MISTRAL,
    model_name="mistral-large-latest",
    api_key=os.getenv("MISTRAL_API_KEY"),
    max_tokens=1000,
    temperature=0.7,
    cost_per_1k_input=0.01,
    cost_per_1k_output=0.03
)

backend = MistralBackend(config)
response = backend.generate(prompt, system_prompt)
```

**Best Use Cases**:
- DSGVO-konforme Anwendungen
- Französische/europäische Philosophie
- Kosteneffiziente Alternative
- Mehrsprachige Debatten

### 5. Ollama (Local Alternative)

**Status**: ✅ Local Integration

**Vorteile**:
- Einfaches lokales Deployment
- Kompatibel mit llama.cpp Modellen
- Keine API-Kosten
- Docker-Support

**Modelle**:
- llama2 (7B, 13B, 70B)
- mistral
- phi
- gemma
- codellama

**Installation**:
```bash
# Install Ollama
curl https://ollama.ai/install.sh | sh

# Pull models
ollama pull llama2
ollama pull mistral
```

**Integration**:
```python
from llm_backends import OllamaBackend, BackendConfig, BackendType

config = BackendConfig(
    backend_type=BackendType.OLLAMA,
    model_name="llama2",
    api_url="http://localhost:11434",
    max_tokens=1000,
    temperature=0.7
)

backend = OllamaBackend(config)
response = backend.generate(prompt, system_prompt)
```

**Best Use Cases**:
- Lokale Entwicklung
- Fallback für llama.cpp
- Prototyping
- Resource-constrained environments

## Multi-Backend Orchestration

### Fallback-Chain

Das System implementiert eine automatische Fallback-Kette:

```
llama.cpp → Ollama → Claude → GPT-4 → Mistral
 (Primary)   (Local)  (Cloud)  (Cloud)  (Cloud)
```

**Logik**:
1. Versuche primäres Backend (llama.cpp)
2. Bei Fehler: Fallback zu Ollama (lokal)
3. Bei Fehler: Fallback zu Claude (Cloud, beste Ethik-Qualität)
4. Bei Fehler: Fallback zu GPT-4 (Cloud, breites Wissen)
5. Bei Fehler: Fallback zu Mistral (Cloud, EU-Alternative)

### Backend-Auswahl pro Philosoph

```python
# Assign backends based on philosophical school
PHILOSOPHER_BACKENDS = {
    PhilosophySchool.KANT: BackendType.CLAUDE,  # Constitutional AI passt zu Kant
    PhilosophySchool.UTILITARIANISM: BackendType.GPT4,  # Breite Abwägungen
    PhilosophySchool.CONTRACTUALISM: BackendType.CLAUDE,  # Reasoning-Stärke
    PhilosophySchool.VIRTUE_ETHICS: BackendType.MISTRAL,  # Europäische Tradition
    PhilosophySchool.SOCRATIC: BackendType.LLAMACPP,  # Schnelle Dialektik
    # ... weitere Zuordnungen
}
```

### Cost Optimization

**Strategie 1: Tiered Usage**
- Primär: llama.cpp (kostenlos)
- Fallback: Ollama (kostenlos)
- Selektiv: Claude für ethische Highlights
- Gelegentlich: GPT-4 für Vergleiche

**Strategie 2: Model Selection**
```python
# Use cheaper models for simple tasks
if task_complexity == "simple":
    backend = BackendType.LLAMACPP  # oder Haiku
elif task_complexity == "medium":
    backend = BackendType.OLLAMA  # oder Sonnet
else:
    backend = BackendType.CLAUDE  # Opus für komplexe Fälle
```

**Strategie 3: Caching**
```python
# Cache similar prompts to reduce API calls
cache_key = hashlib.sha256(f"{prompt}{system_prompt}".encode()).hexdigest()
if cache_key in response_cache:
    return response_cache[cache_key]
```

## Prompt Engineering per Backend

### llama.cpp / Ollama
```python
system_prompt = """Du bist ein Kantischer Ethiker, der strikt nach dem kategorischen Imperativ argumentiert.

Kernprinzipien:
1. Handle nur nach Maximen, die als allgemeines Gesetz taugen
2. Behandle Menschen nie nur als Mittel
3. Autonomie und Würde sind unantastbar

Antworte in deutscher Sprache, präzise und strukturiert."""

prompt = f"""Analyse folgende Nachricht aus Kantischer Perspektive:

Nachricht: {news_title}

Gib eine erste moralische Position in 3-5 Sätzen."""
```

### Claude
```python
system_prompt = """You are a Kantian ethicist deeply versed in the Critique of Practical Reason.

Your Constitutional Principles:
1. The Categorical Imperative in all its formulations
2. Respect for rational autonomy
3. The distinction between hypothetical and categorical imperatives
4. The primacy of duty over inclination

Respond thoughtfully, citing Kant where relevant. Use formal philosophical language."""

prompt = f"""Analyze this news item from a Kantian ethical perspective:

Title: {news_title}
Context: {news_summary}

Provide:
1. Your moral position
2. Reasoning grounded in the categorical imperative
3. Key principles applied
4. Potential objections from other perspectives"""
```

### GPT-4
```python
system_prompt = """You are a utilitarian ethicist in the tradition of Jeremy Bentham and John Stuart Mill.

Framework:
- Greatest Happiness Principle: Actions are right in proportion to their tendency to promote happiness
- Hedonistic Calculus: Consider intensity, duration, certainty, propinquity, fecundity, purity, extent
- Quality over Quantity: Mill's higher and lower pleasures distinction

Provide clear cost-benefit analysis of moral situations."""

prompt = f"""From a utilitarian perspective, evaluate:

Scenario: {news_title}
Details: {news_summary}

Structure your response:
POSITION: [Moral stance based on utility maximization]
CALCULATION: [Hedonic calculus breakdown]
PRINCIPLES: [Relevant utilitarian tenets]
COUNTERARGUMENTS: [Objections you anticipate]"""
```

## Performance Benchmarks

### Latency Comparison

| Backend | Model | Avg Latency | P95 Latency |
|---------|-------|-------------|-------------|
| llama.cpp | Llama-2-7B | 120ms | 250ms |
| llama.cpp | Llama-2-13B | 200ms | 400ms |
| Ollama | llama2 | 150ms | 300ms |
| Claude | Sonnet | 800ms | 1500ms |
| Claude | Opus | 1200ms | 2000ms |
| GPT-4 | Turbo | 900ms | 1600ms |
| Mistral | Large | 850ms | 1400ms |

### Quality Comparison

**Philosophical Depth** (1-10):
- Claude Opus: 9/10
- GPT-4: 8/10
- Claude Sonnet: 8/10
- Mistral Large: 7/10
- Llama-2-70B: 7/10
- Llama-2-13B: 6/10
- Llama-2-7B: 5/10

**Ethical Reasoning** (1-10):
- Claude Opus: 9/10
- Claude Sonnet: 8/10
- GPT-4: 7/10
- Mistral Large: 7/10
- Llama-2-70B: 6/10

### Cost per 1000 Arguments

**Annahme**: 500 input + 500 output tokens pro Argument

| Backend | Cost per 1K Args | Monthly (10K Args) |
|---------|------------------|--------------------|
| llama.cpp | $0 | $0 |
| Ollama | $0 | $0 |
| Claude Haiku | $0.75 | $7.50 |
| Claude Sonnet | $9 | $90 |
| Claude Opus | $45 | $450 |
| GPT-4 Turbo | $20 | $200 |
| Mistral Large | $20 | $200 |

## Best Practices

### 1. Backend-Auswahl

**Entscheidungskriterien**:
```python
def select_backend(
    complexity: str,
    privacy_required: bool,
    budget_available: bool,
    quality_priority: bool
) -> BackendType:
    
    if privacy_required:
        return BackendType.LLAMACPP
    
    if not budget_available:
        return BackendType.OLLAMA
    
    if quality_priority and budget_available:
        if complexity == "high":
            return BackendType.CLAUDE  # Opus
        else:
            return BackendType.CLAUDE  # Sonnet
    
    # Balanced default
    return BackendType.LLAMACPP
```
<!-- TODO: verify interface against current source -->

### 2. Error Handling

```python
def generate_with_retry(
    orchestrator: MultiAIOrchestrator,
    prompt: str,
    max_retries: int = 3
) -> BackendResponse:
    
    for attempt in range(max_retries):
        response = orchestrator.generate(prompt)
        
        if not response.error:
            return response
        
        # Log error and retry with next backend in chain
        logger.warning(f"Attempt {attempt + 1} failed: {response.error}")
        time.sleep(2 ** attempt)  # Exponential backoff
    
    raise RuntimeError("All backends failed after retries")
```
<!-- TODO: verify interface against current source -->

### 3. Response Validation

```python
def validate_philosophical_response(response: BackendResponse) -> bool:
    """Validate that response meets philosophical standards."""
    
    if len(response.text) < 50:
        return False  # Too short
    
    # Check for key philosophical terms
    required_terms = ["ethik", "moral", "prinzip", "argument"]
    if not any(term in response.text.lower() for term in required_terms):
        return False
    
    # Check structure
    if "POSITION:" not in response.text and "reasoning" not in response.text.lower():
        return False
    
    return True
```

### 4. Cost Monitoring

```python
def monitor_costs(orchestrator: MultiAIOrchestrator, alert_threshold: float = 10.0):
    """Monitor and alert on API costs."""
    
    stats = orchestrator.get_all_stats()
    
    total_cost = sum(
        backend_stats["total_cost_usd"]
        for backend_stats in stats.values()
    )
    
    if total_cost > alert_threshold:
        logger.warning(f"Cost threshold exceeded: ${total_cost:.2f}")
        # Send alert, switch to local backends, etc.
    
    return total_cost
```

## Integration mit Debate System

### Beispiel: Multi-Backend Debate

```python
from llm_backends import create_orchestrator_with_defaults
from models import PhilosophySchool

# Setup
orchestrator = create_orchestrator_with_defaults()

# Assign backends to philosophers
backend_assignments = {
    PhilosophySchool.KANT: BackendType.CLAUDE,
    PhilosophySchool.UTILITARIANISM: BackendType.GPT4,
    PhilosophySchool.CONTRACTUALISM: BackendType.CLAUDE,
    PhilosophySchool.VIRTUE_ETHICS: BackendType.MISTRAL,
    PhilosophySchool.SOCRATIC: BackendType.LLAMACPP,
}

# Generate arguments from different perspectives
for school, backend_type in backend_assignments.items():
    profile = PHILOSOPHY_PROFILES[school]
    
    prompt = create_argument_prompt(profile, news_article)
    system_prompt = create_system_prompt(profile)
    
    response = orchestrator.generate(
        prompt=prompt,
        system_prompt=system_prompt,
        preferred_backend=backend_type
    )
    
    if response.error:
        logger.error(f"{school.name} failed: {response.error}")
        continue
    
    print(f"{profile.philosopher_name} ({response.backend_type.value}):")
    print(response.text)
    print(f"Cost: ${response.cost_usd:.4f}, Latency: {response.latency_ms:.0f}ms")
    print()

# Print aggregate statistics
print("Debate Statistics:")
for backend_type, stats in orchestrator.get_all_stats().items():
    print(f"{backend_type}:")
    print(f"  Requests: {stats['request_count']}")
    print(f"  Total Cost: ${stats['total_cost_usd']:.2f}")
    print(f"  Tokens: {stats['total_tokens_input']} in, {stats['total_tokens_output']} out")
```

## Zusammenfassung

Die Multi-Backend-Architektur bietet:

✅ **Flexibilität**: Wähle optimal Backend pro Anwendungsfall
✅ **Zuverlässigkeit**: Automatische Fallbacks bei Ausfällen
✅ **Kosteneffizienz**: Mix aus lokalen (kostenlos) und Cloud-Backends
✅ **Qualität**: Nutze Stärken jedes Backends (Claude für Ethik, GPT-4 für Breite)
✅ **Privatsphäre**: Sensitive Daten bleiben lokal (llama.cpp/Ollama)
✅ **Performance**: Lokale Backends für niedrige Latenz

Die Integration von ThemisDB's llama.cpp als primäres Backend garantiert:
- Produktions-Ready ohne externe Abhängigkeiten
- Keine laufenden API-Kosten
- Vollständige Kontrolle über Inferenz
- GPU-Beschleunigung für hohen Durchsatz
- Offline-Fähigkeit für Air-Gapped Deployments
