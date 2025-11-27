"""
Comparators for different AI code assistants.

Provides interfaces to get responses from Copilot, GPT-4, Claude, etc.
for comparison purposes.
"""

import asyncio
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Optional

import aiohttp
import structlog

logger = structlog.get_logger(__name__)


@dataclass
class CompletionRequest:
    """Request for code completion."""
    
    prompt: str
    language: str = "python"
    max_tokens: int = 500
    temperature: float = 0.2
    stop_sequences: Optional[list[str]] = None


@dataclass
class CompletionResponse:
    """Response from a code assistant."""
    
    code: str
    model: str
    finish_reason: str
    usage: dict
    latency_ms: float


class BaseComparator(ABC):
    """Base class for AI code assistant comparators."""
    
    @abstractmethod
    async def complete(self, request: CompletionRequest) -> CompletionResponse:
        """Get a code completion from the assistant."""
        pass
    
    @abstractmethod
    async def chat(self, message: str, context: Optional[str] = None) -> CompletionResponse:
        """Get a chat response from the assistant."""
        pass
    
    @property
    @abstractmethod
    def name(self) -> str:
        """Name of the assistant."""
        pass


class CopilotComparator(BaseComparator):
    """
    GitHub Copilot comparator.
    
    Note: Requires Copilot API access or uses the Copilot proxy.
    For evaluation, we typically use saved Copilot responses.
    """
    
    def __init__(
        self,
        api_key: Optional[str] = None,
        endpoint: str = "https://api.github.com/copilot",
    ):
        self.api_key = api_key
        self.endpoint = endpoint
        self._session: Optional[aiohttp.ClientSession] = None
    
    @property
    def name(self) -> str:
        return "GitHub Copilot"
    
    async def _get_session(self) -> aiohttp.ClientSession:
        if self._session is None or self._session.closed:
            headers = {}
            if self.api_key:
                headers["Authorization"] = f"Bearer {self.api_key}"
            self._session = aiohttp.ClientSession(headers=headers)
        return self._session
    
    async def complete(self, request: CompletionRequest) -> CompletionResponse:
        """
        Get completion from Copilot.
        
        Note: This is a placeholder - actual Copilot API requires special access.
        For evaluation, use ReferenceComparator with saved Copilot responses.
        """
        import time
        start = time.time()
        
        # Placeholder - in practice, you'd call the actual Copilot API
        # or use saved Copilot responses
        logger.warning("Copilot API not implemented - use ReferenceComparator for saved responses")
        
        return CompletionResponse(
            code="# Copilot response placeholder",
            model="copilot",
            finish_reason="not_implemented",
            usage={},
            latency_ms=(time.time() - start) * 1000,
        )
    
    async def chat(self, message: str, context: Optional[str] = None) -> CompletionResponse:
        """Get chat response from Copilot."""
        return await self.complete(CompletionRequest(prompt=message))
    
    async def close(self):
        if self._session and not self._session.closed:
            await self._session.close()


class GPT4Comparator(BaseComparator):
    """
    GPT-4 comparator using OpenAI API.
    """
    
    def __init__(
        self,
        api_key: Optional[str] = None,
        model: str = "gpt-4-turbo-preview",
        base_url: Optional[str] = None,
    ):
        self.api_key = api_key
        self.model = model
        self.base_url = base_url
        self._client = None
    
    @property
    def name(self) -> str:
        return f"OpenAI {self.model}"
    
    async def _get_client(self):
        if self._client is None:
            try:
                import openai
                self._client = openai.AsyncOpenAI(
                    api_key=self.api_key,
                    base_url=self.base_url,
                )
            except ImportError:
                raise ImportError("openai package required")
        return self._client
    
    async def complete(self, request: CompletionRequest) -> CompletionResponse:
        """Get completion from GPT-4."""
        import time
        start = time.time()
        
        client = await self._get_client()
        
        system_prompt = f"""You are an expert {request.language} programmer.
Generate clean, efficient, well-documented code.
Only output the code, no explanations."""

        response = await client.chat.completions.create(
            model=self.model,
            messages=[
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": request.prompt},
            ],
            max_tokens=request.max_tokens,
            temperature=request.temperature,
            stop=request.stop_sequences,
        )
        
        return CompletionResponse(
            code=response.choices[0].message.content,
            model=self.model,
            finish_reason=response.choices[0].finish_reason,
            usage={
                "prompt_tokens": response.usage.prompt_tokens,
                "completion_tokens": response.usage.completion_tokens,
            },
            latency_ms=(time.time() - start) * 1000,
        )
    
    async def chat(self, message: str, context: Optional[str] = None) -> CompletionResponse:
        """Get chat response from GPT-4."""
        prompt = message
        if context:
            prompt = f"Context:\n{context}\n\nQuestion: {message}"
        
        return await self.complete(CompletionRequest(prompt=prompt))


class ClaudeComparator(BaseComparator):
    """
    Claude comparator using Anthropic API.
    """
    
    def __init__(
        self,
        api_key: Optional[str] = None,
        model: str = "claude-3-opus-20240229",
    ):
        self.api_key = api_key
        self.model = model
        self._client = None
    
    @property
    def name(self) -> str:
        return f"Anthropic {self.model}"
    
    async def _get_client(self):
        if self._client is None:
            try:
                import anthropic
                self._client = anthropic.AsyncAnthropic(api_key=self.api_key)
            except ImportError:
                raise ImportError("anthropic package required")
        return self._client
    
    async def complete(self, request: CompletionRequest) -> CompletionResponse:
        """Get completion from Claude."""
        import time
        start = time.time()
        
        client = await self._get_client()
        
        system_prompt = f"""You are an expert {request.language} programmer.
Generate clean, efficient, well-documented code.
Only output the code, no explanations."""

        response = await client.messages.create(
            model=self.model,
            max_tokens=request.max_tokens,
            system=system_prompt,
            messages=[
                {"role": "user", "content": request.prompt},
            ],
        )
        
        return CompletionResponse(
            code=response.content[0].text,
            model=self.model,
            finish_reason=response.stop_reason,
            usage={
                "input_tokens": response.usage.input_tokens,
                "output_tokens": response.usage.output_tokens,
            },
            latency_ms=(time.time() - start) * 1000,
        )
    
    async def chat(self, message: str, context: Optional[str] = None) -> CompletionResponse:
        """Get chat response from Claude."""
        prompt = message
        if context:
            prompt = f"Context:\n{context}\n\nQuestion: {message}"
        
        return await self.complete(CompletionRequest(prompt=prompt))


class ReferenceComparator(BaseComparator):
    """
    Comparator for pre-saved reference responses.
    
    Use this to compare against saved Copilot, human expert, or other responses.
    """
    
    def __init__(
        self,
        responses: dict[str, str],
        source_name: str = "Reference",
    ):
        """
        Args:
            responses: Dict mapping prompts/questions to responses
            source_name: Name of the reference source
        """
        self.responses = responses
        self.source_name = source_name
    
    @property
    def name(self) -> str:
        return self.source_name
    
    async def complete(self, request: CompletionRequest) -> CompletionResponse:
        """Get pre-saved response."""
        import time
        
        # Find closest matching prompt
        response = self.responses.get(request.prompt)
        
        if not response:
            # Try fuzzy matching
            for prompt, resp in self.responses.items():
                if request.prompt.lower() in prompt.lower() or prompt.lower() in request.prompt.lower():
                    response = resp
                    break
        
        if not response:
            response = "# No reference response available"
        
        return CompletionResponse(
            code=response,
            model=self.source_name,
            finish_reason="reference",
            usage={},
            latency_ms=0,
        )
    
    async def chat(self, message: str, context: Optional[str] = None) -> CompletionResponse:
        """Get pre-saved chat response."""
        return await self.complete(CompletionRequest(prompt=message))
    
    @classmethod
    def from_file(cls, filepath: str, source_name: str = "Reference") -> "ReferenceComparator":
        """
        Load reference responses from a JSON file.
        
        Expected format:
        {
            "prompt1": "response1",
            "prompt2": "response2"
        }
        """
        import json
        from pathlib import Path
        
        data = json.loads(Path(filepath).read_text())
        return cls(responses=data, source_name=source_name)
    
    @classmethod
    def from_jsonl(cls, filepath: str, source_name: str = "Reference") -> "ReferenceComparator":
        """
        Load reference responses from a JSONL file.
        
        Expected format (one per line):
        {"prompt": "...", "response": "..."}
        """
        import json
        from pathlib import Path
        
        responses = {}
        for line in Path(filepath).read_text().strip().split("\n"):
            item = json.loads(line)
            responses[item["prompt"]] = item["response"]
        
        return cls(responses=responses, source_name=source_name)


class VLLMComparator(BaseComparator):
    """
    vLLM comparator for local models.
    
    Use this to compare against your own vLLM-served models.
    """
    
    def __init__(
        self,
        base_url: str = "http://localhost:8000/v1",
        model: str = "codellama/CodeLlama-13b-Instruct-hf",
        api_key: str = "none",
    ):
        self.base_url = base_url
        self.model = model
        self.api_key = api_key
        self._client = None
    
    @property
    def name(self) -> str:
        return f"vLLM {self.model}"
    
    async def _get_client(self):
        if self._client is None:
            try:
                import openai
                self._client = openai.AsyncOpenAI(
                    api_key=self.api_key,
                    base_url=self.base_url,
                )
            except ImportError:
                raise ImportError("openai package required")
        return self._client
    
    async def complete(self, request: CompletionRequest) -> CompletionResponse:
        """Get completion from vLLM."""
        import time
        start = time.time()
        
        client = await self._get_client()
        
        response = await client.chat.completions.create(
            model=self.model,
            messages=[
                {"role": "user", "content": request.prompt},
            ],
            max_tokens=request.max_tokens,
            temperature=request.temperature,
            stop=request.stop_sequences,
        )
        
        return CompletionResponse(
            code=response.choices[0].message.content,
            model=self.model,
            finish_reason=response.choices[0].finish_reason or "stop",
            usage={
                "prompt_tokens": response.usage.prompt_tokens if response.usage else 0,
                "completion_tokens": response.usage.completion_tokens if response.usage else 0,
            },
            latency_ms=(time.time() - start) * 1000,
        )
    
    async def chat(self, message: str, context: Optional[str] = None) -> CompletionResponse:
        """Get chat response from vLLM."""
        prompt = message
        if context:
            prompt = f"Context:\n{context}\n\nQuestion: {message}"
        
        return await self.complete(CompletionRequest(prompt=prompt))
