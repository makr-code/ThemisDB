"""
RESPO Step Executor

Executes individual plan steps with appropriate tools.
"""

from dataclasses import dataclass, field
from typing import Any, Optional

import structlog

from respo.agents.planner import PlanStep

logger = structlog.get_logger(__name__)


@dataclass
class ExecutionResult:
    """Result from executing a step."""
    success: bool
    output: str
    artifacts: list[dict] = field(default_factory=list)
    metadata: dict[str, Any] = field(default_factory=dict)


@dataclass
class ExecutionContext:
    """Context for step execution."""
    codebase_path: Optional[str] = None
    language: str = "python"
    previous_results: dict[str, str] = field(default_factory=dict)


class StepExecutor:
    """Executor for plan steps."""

    def __init__(self, llm_client: Any, vector_store: Any = None, rag_pipeline: Any = None):
        self.llm = llm_client
        self.vector_store = vector_store
        self.rag = rag_pipeline

    async def execute(self, step: PlanStep, context: dict[str, Any]) -> ExecutionResult:
        """Execute a plan step."""
        handlers = {
            "search": self._execute_search,
            "analyze": self._execute_analyze,
            "implement": self._execute_implement,
            "test": self._execute_test,
            "review": self._execute_review,
            "document": self._execute_document,
        }
        handler = handlers.get(step.action, self._execute_generic)
        return await handler(step, context)

    async def _execute_search(self, step: PlanStep, context: dict) -> ExecutionResult:
        if self.rag:
            results = await self.rag.search(query=step.description, limit=10)
            output = "\n".join(f"- {r.get('file', 'unknown')}: {r.get('content', '')[:200]}" for r in results)
            return ExecutionResult(success=True, output=output)
        prompt = f"Simulate code search for: {step.description}"
        response = await self.llm.generate(prompt=prompt, max_tokens=1024)
        return ExecutionResult(success=True, output=response.text)

    async def _execute_analyze(self, step: PlanStep, context: dict) -> ExecutionResult:
        prev = "\n".join(f"- {k}: {str(v)[:200]}" for k, v in context.items() if not k.startswith("_"))
        prompt = f"Analyze: {step.description}\n\nContext:\n{prev}"
        response = await self.llm.generate(prompt=prompt, max_tokens=1500)
        return ExecutionResult(success=True, output=response.text)

    async def _execute_implement(self, step: PlanStep, context: dict) -> ExecutionResult:
        lang = context.get("language", "python")
        prompt = f"Implement: {step.description}\n\nLanguage: {lang}"
        response = await self.llm.generate(prompt=prompt, max_tokens=2048)
        return ExecutionResult(success=True, output=response.text, artifacts=[{"type": "code", "language": lang}])

    async def _execute_test(self, step: PlanStep, context: dict) -> ExecutionResult:
        prompt = f"Create tests for: {step.description}"
        response = await self.llm.generate(prompt=prompt, max_tokens=1500)
        return ExecutionResult(success=True, output=response.text)

    async def _execute_review(self, step: PlanStep, context: dict) -> ExecutionResult:
        prompt = f"Review code: {step.description}"
        response = await self.llm.generate(prompt=prompt, max_tokens=1500)
        return ExecutionResult(success=True, output=response.text)

    async def _execute_document(self, step: PlanStep, context: dict) -> ExecutionResult:
        prompt = f"Create documentation: {step.description}"
        response = await self.llm.generate(prompt=prompt, max_tokens=2000)
        return ExecutionResult(success=True, output=response.text)

    async def _execute_generic(self, step: PlanStep, context: dict) -> ExecutionResult:
        prompt = f"Complete task: {step.title}\n\n{step.description}"
        response = await self.llm.generate(prompt=prompt, max_tokens=1500)
        return ExecutionResult(success=True, output=response.text)
