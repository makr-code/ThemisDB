"""
RESPO vLLM Client

Client for communicating with vLLM server (OpenAI-compatible API).
"""

import asyncio
from dataclasses import dataclass
from typing import AsyncIterator, Optional

import structlog
from openai import AsyncOpenAI

logger = structlog.get_logger(__name__)


@dataclass
class GenerationConfig:
    """Configuration for text generation."""

    max_tokens: int = 2048
    temperature: float = 0.1
    top_p: float = 0.95
    stop: Optional[list[str]] = None
    stream: bool = True


@dataclass
class GenerationResult:
    """Result from text generation."""

    text: str
    prompt_tokens: int
    completion_tokens: int
    model: str
    finish_reason: str


class VLLMClient:
    """
    Client for vLLM server with OpenAI-compatible API.

    Features:
    - Async streaming generation
    - LoRA adapter selection
    - Automatic retries
    """

    def __init__(
        self,
        base_url: str = "http://localhost:8000/v1",
        api_key: str = "EMPTY",
        model: str = "codellama/CodeLlama-13b-Instruct-hf",
        timeout: float = 120.0,
    ) -> None:
        """
        Initialize vLLM client.

        Args:
            base_url: vLLM server URL
            api_key: API key (usually "EMPTY" for local vLLM)
            model: Model name
            timeout: Request timeout in seconds
        """
        self.base_url = base_url
        self.model = model
        self._client = AsyncOpenAI(
            base_url=base_url,
            api_key=api_key,
            timeout=timeout,
        )

    async def generate(
        self,
        prompt: str,
        system_prompt: Optional[str] = None,
        config: Optional[GenerationConfig] = None,
        lora_adapter: Optional[str] = None,
    ) -> GenerationResult:
        """
        Generate text completion.

        Args:
            prompt: User prompt
            system_prompt: Optional system prompt
            config: Generation configuration
            lora_adapter: Optional LoRA adapter name

        Returns:
            Generation result
        """
        config = config or GenerationConfig()

        messages = []
        if system_prompt:
            messages.append({"role": "system", "content": system_prompt})
        messages.append({"role": "user", "content": prompt})

        # Use LoRA adapter if specified
        model = f"{self.model}:{lora_adapter}" if lora_adapter else self.model

        logger.debug(
            "Generating completion",
            model=model,
            prompt_length=len(prompt),
            max_tokens=config.max_tokens,
        )

        response = await self._client.chat.completions.create(
            model=model,
            messages=messages,
            max_tokens=config.max_tokens,
            temperature=config.temperature,
            top_p=config.top_p,
            stop=config.stop,
            stream=False,
        )

        choice = response.choices[0]
        usage = response.usage

        return GenerationResult(
            text=choice.message.content or "",
            prompt_tokens=usage.prompt_tokens if usage else 0,
            completion_tokens=usage.completion_tokens if usage else 0,
            model=response.model,
            finish_reason=choice.finish_reason or "unknown",
        )

    async def generate_stream(
        self,
        prompt: str,
        system_prompt: Optional[str] = None,
        config: Optional[GenerationConfig] = None,
        lora_adapter: Optional[str] = None,
    ) -> AsyncIterator[str]:
        """
        Generate text completion with streaming.

        Args:
            prompt: User prompt
            system_prompt: Optional system prompt
            config: Generation configuration
            lora_adapter: Optional LoRA adapter name

        Yields:
            Text chunks as they are generated
        """
        config = config or GenerationConfig()

        messages = []
        if system_prompt:
            messages.append({"role": "system", "content": system_prompt})
        messages.append({"role": "user", "content": prompt})

        model = f"{self.model}:{lora_adapter}" if lora_adapter else self.model

        logger.debug(
            "Streaming completion",
            model=model,
            prompt_length=len(prompt),
        )

        stream = await self._client.chat.completions.create(
            model=model,
            messages=messages,
            max_tokens=config.max_tokens,
            temperature=config.temperature,
            top_p=config.top_p,
            stop=config.stop,
            stream=True,
        )

        async for chunk in stream:
            if chunk.choices and chunk.choices[0].delta.content:
                yield chunk.choices[0].delta.content

    async def health_check(self) -> bool:
        """Check if vLLM server is available."""
        try:
            models = await self._client.models.list()
            return len(models.data) > 0
        except Exception as e:
            logger.warning("vLLM health check failed", error=str(e))
            return False

    async def list_models(self) -> list[str]:
        """List available models on the server."""
        try:
            models = await self._client.models.list()
            return [m.id for m in models.data]
        except Exception as e:
            logger.error("Failed to list models", error=str(e))
            return []

    async def close(self) -> None:
        """Close the client."""
        await self._client.close()
