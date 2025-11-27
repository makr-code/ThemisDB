"""
RESPO Deep Research Agent

Inspired by Google Gemini Deep Research - iterative research and synthesis.
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import Any, AsyncIterator, Optional

import structlog

from respo.agents.executor import StepExecutor
from respo.agents.planner import AgenticPlanner, ExecutionPlan, PlannerConfig, StepStatus

logger = structlog.get_logger(__name__)


@dataclass
class ResearchConfig:
    """Configuration for deep research."""
    max_iterations: int = 5
    max_sources: int = 20
    include_code_examples: bool = True
    target_language: str = "python"


@dataclass
class ResearchResult:
    """Result from deep research."""
    query: str
    summary: str
    findings: list[dict] = field(default_factory=list)
    code_examples: list[dict] = field(default_factory=list)
    sources: list[dict] = field(default_factory=list)
    confidence: float = 0.0
    iterations: int = 0
    duration_seconds: float = 0.0
    plan: Optional[ExecutionPlan] = None

    def to_dict(self) -> dict:
        return {
            "query": self.query,
            "summary": self.summary,
            "findings": self.findings,
            "code_examples": self.code_examples,
            "sources": self.sources,
            "confidence": self.confidence,
            "iterations": self.iterations,
            "duration_seconds": self.duration_seconds,
        }


class DeepResearchAgent:
    """Deep research agent for comprehensive code research."""

    def __init__(self, llm_client: Any, vector_store: Any = None, rag_pipeline: Any = None, config: Optional[ResearchConfig] = None):
        self.llm = llm_client
        self.config = config or ResearchConfig()
        planner_config = PlannerConfig(max_steps=8, parallel_execution=True, language=self.config.target_language)
        self.planner = AgenticPlanner(llm_client, planner_config)
        self.executor = StepExecutor(llm_client, vector_store, rag_pipeline)

    async def research(self, query: str, context: Optional[str] = None) -> ResearchResult:
        """Perform deep research on a query."""
        start_time = datetime.now()
        findings, code_examples, sources = [], [], []

        plan = await self.planner.create_plan(query, {"user_context": context} if context else None)

        async for event in self.planner.stream_plan_execution(plan, self.executor):
            if event["event"] == "step_complete":
                step = event["data"]["step"]
                if step["action"] == "search":
                    sources.append({"title": step["title"], "result": step.get("result", "")[:500]})
                elif step["action"] == "implement":
                    code_examples.append({"title": step["title"], "code": step.get("result", "")})
                else:
                    findings.append({"title": step["title"], "content": step.get("result", "")[:500]})

        return ResearchResult(
            query=query,
            summary=plan.final_answer or "",
            findings=findings,
            code_examples=code_examples,
            sources=sources,
            confidence=self._calc_confidence(plan),
            iterations=len(plan.steps),
            duration_seconds=(datetime.now() - start_time).total_seconds(),
            plan=plan,
        )

    async def stream_research(self, query: str, context: Optional[str] = None) -> AsyncIterator[dict]:
        """Stream research progress via SSE events."""
        yield {"event": "research_start", "data": {"query": query}}
        plan = await self.planner.create_plan(query, {"user_context": context} if context else None)
        yield {"event": "plan_created", "data": {"plan_id": plan.id, "steps": len(plan.steps)}}

        async for event in self.planner.stream_plan_execution(plan, self.executor):
            yield event

        yield {"event": "research_complete", "data": {"summary": plan.final_answer}}

    def _calc_confidence(self, plan: ExecutionPlan) -> float:
        if not plan.steps:
            return 0.0
        completed = sum(1 for s in plan.steps if s.status == StepStatus.COMPLETED)
        return round(completed / len(plan.steps), 2)
