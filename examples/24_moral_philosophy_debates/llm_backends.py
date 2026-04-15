"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_backends.py                                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     631                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Multi-AI Backend Support for Moral Philosophy Debates

This module provides integration with multiple AI backends including:
- ThemisDB llama.cpp (local embedded inference)
- Claude (Anthropic)
- GPT-4 (OpenAI)
- Mistral API
- Ollama (local)

Each backend has specialized prompt engineering and response handling.
"""

import os
import time
import json
import requests
from typing import Optional, Dict, List, Any
from dataclasses import dataclass, field
from enum import Enum
from abc import ABC, abstractmethod


class BackendType(Enum):
    """Supported AI backend types."""
    LLAMACPP = "llamacpp"  # ThemisDB embedded llama.cpp
    CLAUDE = "claude"
    GPT4 = "gpt4"
    MISTRAL = "mistral"
    OLLAMA = "ollama"


@dataclass
class BackendConfig:
    """Configuration for an AI backend."""
    backend_type: BackendType
    model_name: str
    api_key: Optional[str] = None
    api_url: Optional[str] = None
    max_tokens: int = 1000
    temperature: float = 0.7
    top_p: float = 0.9
    enabled: bool = True
    cost_per_1k_input: float = 0.0  # USD
    cost_per_1k_output: float = 0.0  # USD
    

@dataclass
class BackendResponse:
    """Response from an AI backend."""
    text: str
    backend_type: BackendType
    model_name: str
    tokens_used: int = 0
    latency_ms: float = 0.0
    cost_usd: float = 0.0
    error: Optional[str] = None
    metadata: Dict[str, Any] = field(default_factory=dict)


class AIBackend(ABC):
    """Abstract base class for AI backends."""
    
    def __init__(self, config: BackendConfig):
        self.config = config
        self.total_tokens_input = 0
        self.total_tokens_output = 0
        self.total_cost = 0.0
        self.request_count = 0
    
    @abstractmethod
    def generate(self, prompt: str, system_prompt: Optional[str] = None) -> BackendResponse:
        """Generate text from the backend."""
        pass
    
    def calculate_cost(self, input_tokens: int, output_tokens: int) -> float:
        """Calculate cost in USD."""
        input_cost = (input_tokens / 1000) * self.config.cost_per_1k_input
        output_cost = (output_tokens / 1000) * self.config.cost_per_1k_output
        return input_cost + output_cost
    
    def get_stats(self) -> Dict[str, Any]:
        """Get usage statistics."""
        return {
            "backend_type": self.config.backend_type.value,
            "model_name": self.config.model_name,
            "request_count": self.request_count,
            "total_tokens_input": self.total_tokens_input,
            "total_tokens_output": self.total_tokens_output,
            "total_cost_usd": self.total_cost
        }


class LlamaCppBackend(AIBackend):
    """ThemisDB embedded llama.cpp backend."""
    
    def __init__(self, config: BackendConfig):
        super().__init__(config)
        # Connection to ThemisDB's llama.cpp inference engine
        self.themis_url = config.api_url or "http://localhost:8529/api/llm"
    
    def generate(self, prompt: str, system_prompt: Optional[str] = None) -> BackendResponse:
        start_time = time.time()
        self.request_count += 1
        
        try:
            # Build request payload
            payload = {
                "model": self.config.model_name,
                "prompt": prompt,
                "max_tokens": self.config.max_tokens,
                "temperature": self.config.temperature,
                "top_p": self.config.top_p,
            }
            
            if system_prompt:
                payload["system_prompt"] = system_prompt
            
            # Call ThemisDB LLM endpoint
            response = requests.post(
                f"{self.themis_url}/generate",
                json=payload,
                timeout=120
            )
            response.raise_for_status()
            
            data = response.json()
            text = data.get("text", "")
            tokens_used = data.get("tokens", 0)
            
            self.total_tokens_input += len(prompt.split())  # Rough estimate
            self.total_tokens_output += tokens_used
            
            latency_ms = (time.time() - start_time) * 1000
            
            return BackendResponse(
                text=text,
                backend_type=BackendType.LLAMACPP,
                model_name=self.config.model_name,
                tokens_used=tokens_used,
                latency_ms=latency_ms,
                cost_usd=0.0,  # Local inference is free
                metadata={"themis_server": self.themis_url}
            )
        
        except Exception as e:
            latency_ms = (time.time() - start_time) * 1000
            return BackendResponse(
                text="",
                backend_type=BackendType.LLAMACPP,
                model_name=self.config.model_name,
                latency_ms=latency_ms,
                error=str(e)
            )


class ClaudeBackend(AIBackend):
    """Anthropic Claude backend."""
    
    def __init__(self, config: BackendConfig):
        super().__init__(config)
        self.api_url = "https://api.anthropic.com/v1/messages"
        self.api_key = config.api_key or os.getenv("ANTHROPIC_API_KEY")
    
    def generate(self, prompt: str, system_prompt: Optional[str] = None) -> BackendResponse:
        start_time = time.time()
        self.request_count += 1
        
        if not self.api_key:
            return BackendResponse(
                text="",
                backend_type=BackendType.CLAUDE,
                model_name=self.config.model_name,
                error="ANTHROPIC_API_KEY not set"
            )
        
        try:
            headers = {
                "x-api-key": self.api_key,
                "anthropic-version": "2023-06-01",
                "content-type": "application/json"
            }
            
            payload = {
                "model": self.config.model_name,
                "max_tokens": self.config.max_tokens,
                "temperature": self.config.temperature,
                "messages": [
                    {"role": "user", "content": prompt}
                ]
            }
            
            if system_prompt:
                payload["system"] = system_prompt
            
            response = requests.post(
                self.api_url,
                headers=headers,
                json=payload,
                timeout=120
            )
            response.raise_for_status()
            
            data = response.json()
            text = data["content"][0]["text"]
            input_tokens = data["usage"]["input_tokens"]
            output_tokens = data["usage"]["output_tokens"]
            
            self.total_tokens_input += input_tokens
            self.total_tokens_output += output_tokens
            
            cost = self.calculate_cost(input_tokens, output_tokens)
            self.total_cost += cost
            
            latency_ms = (time.time() - start_time) * 1000
            
            return BackendResponse(
                text=text,
                backend_type=BackendType.CLAUDE,
                model_name=self.config.model_name,
                tokens_used=input_tokens + output_tokens,
                latency_ms=latency_ms,
                cost_usd=cost,
                metadata={"input_tokens": input_tokens, "output_tokens": output_tokens}
            )
        
        except Exception as e:
            latency_ms = (time.time() - start_time) * 1000
            return BackendResponse(
                text="",
                backend_type=BackendType.CLAUDE,
                model_name=self.config.model_name,
                latency_ms=latency_ms,
                error=str(e)
            )


class GPT4Backend(AIBackend):
    """OpenAI GPT-4 backend."""
    
    def __init__(self, config: BackendConfig):
        super().__init__(config)
        self.api_url = "https://api.openai.com/v1/chat/completions"
        self.api_key = config.api_key or os.getenv("OPENAI_API_KEY")
    
    def generate(self, prompt: str, system_prompt: Optional[str] = None) -> BackendResponse:
        start_time = time.time()
        self.request_count += 1
        
        if not self.api_key:
            return BackendResponse(
                text="",
                backend_type=BackendType.GPT4,
                model_name=self.config.model_name,
                error="OPENAI_API_KEY not set"
            )
        
        try:
            headers = {
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json"
            }
            
            messages = []
            if system_prompt:
                messages.append({"role": "system", "content": system_prompt})
            messages.append({"role": "user", "content": prompt})
            
            payload = {
                "model": self.config.model_name,
                "messages": messages,
                "max_tokens": self.config.max_tokens,
                "temperature": self.config.temperature,
                "top_p": self.config.top_p
            }
            
            response = requests.post(
                self.api_url,
                headers=headers,
                json=payload,
                timeout=120
            )
            response.raise_for_status()
            
            data = response.json()
            text = data["choices"][0]["message"]["content"]
            input_tokens = data["usage"]["prompt_tokens"]
            output_tokens = data["usage"]["completion_tokens"]
            
            self.total_tokens_input += input_tokens
            self.total_tokens_output += output_tokens
            
            cost = self.calculate_cost(input_tokens, output_tokens)
            self.total_cost += cost
            
            latency_ms = (time.time() - start_time) * 1000
            
            return BackendResponse(
                text=text,
                backend_type=BackendType.GPT4,
                model_name=self.config.model_name,
                tokens_used=input_tokens + output_tokens,
                latency_ms=latency_ms,
                cost_usd=cost,
                metadata={"input_tokens": input_tokens, "output_tokens": output_tokens}
            )
        
        except Exception as e:
            latency_ms = (time.time() - start_time) * 1000
            return BackendResponse(
                text="",
                backend_type=BackendType.GPT4,
                model_name=self.config.model_name,
                latency_ms=latency_ms,
                error=str(e)
            )


class MistralBackend(AIBackend):
    """Mistral AI backend."""
    
    def __init__(self, config: BackendConfig):
        super().__init__(config)
        self.api_url = "https://api.mistral.ai/v1/chat/completions"
        self.api_key = config.api_key or os.getenv("MISTRAL_API_KEY")
    
    def generate(self, prompt: str, system_prompt: Optional[str] = None) -> BackendResponse:
        start_time = time.time()
        self.request_count += 1
        
        if not self.api_key:
            return BackendResponse(
                text="",
                backend_type=BackendType.MISTRAL,
                model_name=self.config.model_name,
                error="MISTRAL_API_KEY not set"
            )
        
        try:
            headers = {
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json"
            }
            
            messages = []
            if system_prompt:
                messages.append({"role": "system", "content": system_prompt})
            messages.append({"role": "user", "content": prompt})
            
            payload = {
                "model": self.config.model_name,
                "messages": messages,
                "max_tokens": self.config.max_tokens,
                "temperature": self.config.temperature,
                "top_p": self.config.top_p
            }
            
            response = requests.post(
                self.api_url,
                headers=headers,
                json=payload,
                timeout=120
            )
            response.raise_for_status()
            
            data = response.json()
            text = data["choices"][0]["message"]["content"]
            input_tokens = data["usage"]["prompt_tokens"]
            output_tokens = data["usage"]["completion_tokens"]
            
            self.total_tokens_input += input_tokens
            self.total_tokens_output += output_tokens
            
            cost = self.calculate_cost(input_tokens, output_tokens)
            self.total_cost += cost
            
            latency_ms = (time.time() - start_time) * 1000
            
            return BackendResponse(
                text=text,
                backend_type=BackendType.MISTRAL,
                model_name=self.config.model_name,
                tokens_used=input_tokens + output_tokens,
                latency_ms=latency_ms,
                cost_usd=cost,
                metadata={"input_tokens": input_tokens, "output_tokens": output_tokens}
            )
        
        except Exception as e:
            latency_ms = (time.time() - start_time) * 1000
            return BackendResponse(
                text="",
                backend_type=BackendType.MISTRAL,
                model_name=self.config.model_name,
                latency_ms=latency_ms,
                error=str(e)
            )


class OllamaBackend(AIBackend):
    """Ollama local backend."""
    
    def __init__(self, config: BackendConfig):
        super().__init__(config)
        self.api_url = config.api_url or "http://localhost:11434"
    
    def generate(self, prompt: str, system_prompt: Optional[str] = None) -> BackendResponse:
        start_time = time.time()
        self.request_count += 1
        
        try:
            payload = {
                "model": self.config.model_name,
                "prompt": prompt,
                "stream": False,
                "options": {
                    "temperature": self.config.temperature,
                    "top_p": self.config.top_p,
                    "num_predict": self.config.max_tokens
                }
            }
            
            if system_prompt:
                payload["system"] = system_prompt
            
            response = requests.post(
                f"{self.api_url}/api/generate",
                json=payload,
                timeout=120
            )
            response.raise_for_status()
            
            data = response.json()
            text = data.get("response", "")
            
            latency_ms = (time.time() - start_time) * 1000
            
            return BackendResponse(
                text=text,
                backend_type=BackendType.OLLAMA,
                model_name=self.config.model_name,
                latency_ms=latency_ms,
                cost_usd=0.0,  # Local is free
                metadata={"ollama_server": self.api_url}
            )
        
        except Exception as e:
            latency_ms = (time.time() - start_time) * 1000
            return BackendResponse(
                text="",
                backend_type=BackendType.OLLAMA,
                model_name=self.config.model_name,
                latency_ms=latency_ms,
                error=str(e)
            )


class MultiAIOrchestrator:
    """
    Orchestrates multiple AI backends for philosophical debates.
    
    Features:
    - Backend selection per philosopher
    - Fallback chain for reliability
    - Cost tracking and optimization
    - Response aggregation
    """
    
    def __init__(self):
        self.backends: Dict[BackendType, AIBackend] = {}
        self.default_backend = BackendType.LLAMACPP
        self.fallback_chain = [
            BackendType.LLAMACPP,
            BackendType.OLLAMA,
            BackendType.CLAUDE,
            BackendType.GPT4,
            BackendType.MISTRAL
        ]
    
    def register_backend(self, backend: AIBackend):
        """Register an AI backend."""
        self.backends[backend.config.backend_type] = backend
    
    def generate(
        self,
        prompt: str,
        system_prompt: Optional[str] = None,
        preferred_backend: Optional[BackendType] = None
    ) -> BackendResponse:
        """
        Generate response with automatic fallback.
        
        Args:
            prompt: User prompt
            system_prompt: Optional system prompt
            preferred_backend: Preferred backend to try first
        
        Returns:
            BackendResponse from first successful backend
        """
        # Determine backend order
        backends_to_try = []
        if preferred_backend and preferred_backend in self.backends:
            backends_to_try.append(preferred_backend)
        
        # Add fallback chain
        for backend_type in self.fallback_chain:
            if backend_type not in backends_to_try and backend_type in self.backends:
                backends_to_try.append(backend_type)
        
        # Try backends in order
        last_error = None
        for backend_type in backends_to_try:
            backend = self.backends[backend_type]
            if not backend.config.enabled:
                continue
            
            response = backend.generate(prompt, system_prompt)
            
            if response.error:
                last_error = response.error
                continue
            
            if response.text:
                return response
        
        # All backends failed
        return BackendResponse(
            text="",
            backend_type=self.default_backend,
            model_name="unavailable",
            error=f"All backends failed. Last error: {last_error}"
        )
    
    def get_all_stats(self) -> Dict[str, Any]:
        """Get statistics from all backends."""
        return {
            backend_type.value: backend.get_stats()
            for backend_type, backend in self.backends.items()
        }


# Default configurations
DEFAULT_CONFIGS = {
    BackendType.LLAMACPP: BackendConfig(
        backend_type=BackendType.LLAMACPP,
        model_name="llama-2-7b-chat",
        api_url="http://localhost:8529/api/llm",
        cost_per_1k_input=0.0,
        cost_per_1k_output=0.0
    ),
    BackendType.CLAUDE: BackendConfig(
        backend_type=BackendType.CLAUDE,
        model_name="claude-3-sonnet-20240229",
        cost_per_1k_input=0.003,  # $3 per million input tokens
        cost_per_1k_output=0.015  # $15 per million output tokens
    ),
    BackendType.GPT4: BackendConfig(
        backend_type=BackendType.GPT4,
        model_name="gpt-4-turbo",
        cost_per_1k_input=0.01,  # $10 per million
        cost_per_1k_output=0.03  # $30 per million
    ),
    BackendType.MISTRAL: BackendConfig(
        backend_type=BackendType.MISTRAL,
        model_name="mistral-large-latest",
        cost_per_1k_input=0.01,  # €10 per million
        cost_per_1k_output=0.03  # €30 per million
    ),
    BackendType.OLLAMA: BackendConfig(
        backend_type=BackendType.OLLAMA,
        model_name="llama2",
        api_url="http://localhost:11434",
        cost_per_1k_input=0.0,
        cost_per_1k_output=0.0
    )
}


def create_orchestrator_with_defaults() -> MultiAIOrchestrator:
    """Create orchestrator with default backend configurations."""
    orchestrator = MultiAIOrchestrator()
    
    # Register llama.cpp (primary)
    orchestrator.register_backend(
        LlamaCppBackend(DEFAULT_CONFIGS[BackendType.LLAMACPP])
    )
    
    # Register Ollama (local fallback)
    orchestrator.register_backend(
        OllamaBackend(DEFAULT_CONFIGS[BackendType.OLLAMA])
    )
    
    # Register Claude (cloud, ethical reasoning)
    orchestrator.register_backend(
        ClaudeBackend(DEFAULT_CONFIGS[BackendType.CLAUDE])
    )
    
    # Register GPT-4 (cloud, general knowledge)
    orchestrator.register_backend(
        GPT4Backend(DEFAULT_CONFIGS[BackendType.GPT4])
    )
    
    # Register Mistral (cloud, European alternative)
    orchestrator.register_backend(
        MistralBackend(DEFAULT_CONFIGS[BackendType.MISTRAL])
    )
    
    return orchestrator
