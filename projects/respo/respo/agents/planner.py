"""
RESPO Agentic Planner

Decomposes complex problems into smaller, solvable steps.
Inspired by Google Gemini Deep Research and VS Code Copilot.
"""

import asyncio
import json
import uuid
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from typing import Any, AsyncIterator, Optional

import structlog

logger = structlog.get_logger(__name__)


class StepStatus(str, Enum):
    """Status of a plan step."""
    PENDING = "pending"
    IN_PROGRESS = "in_progress"
    COMPLETED = "completed"
    FAILED = "failed"
    SKIPPED = "skipped"


@dataclass
class PlanStep:
    """A single step in an execution plan."""
    id: str
    title: str
    description: str
    action: str  # search, analyze, implement, test, review
    dependencies: list[str] = field(default_factory=list)
    status: StepStatus = StepStatus.PENDING
    result: Optional[str] = None
    error: Optional[str] = None
    started_at: Optional[datetime] = None
    completed_at: Optional[datetime] = None
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict:
        return {
            "id": self.id,
            "title": self.title,
            "description": self.description,
            "action": self.action,
            "dependencies": self.dependencies,
            "status": self.status.value,
            "result": self.result,
            "error": self.error,
            "started_at": self.started_at.isoformat() if self.started_at else None,
            "completed_at": self.completed_at.isoformat() if self.completed_at else None,
            "metadata": self.metadata,
        }


@dataclass
class ExecutionPlan:
    """Complete execution plan for a complex problem."""
    id: str
    query: str
    goal: str
    steps: list[PlanStep]
    created_at: datetime = field(default_factory=datetime.now)
    status: StepStatus = StepStatus.PENDING
    final_answer: Optional[str] = None
    context: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict:
        return {
            "id": self.id,
            "query": self.query,
            "goal": self.goal,
            "steps": [s.to_dict() for s in self.steps],
            "created_at": self.created_at.isoformat(),
            "status": self.status.value,
            "final_answer": self.final_answer,
            "context": self.context,
        }

    def get_pending_steps(self) -> list[PlanStep]:
        """Get steps that are ready to execute."""
        completed_ids = {s.id for s in self.steps if s.status == StepStatus.COMPLETED}
        return [
            s for s in self.steps
            if s.status == StepStatus.PENDING
            and all(dep in completed_ids for dep in s.dependencies)
        ]

    def get_progress(self) -> float:
        """Get plan completion progress (0.0 to 1.0)."""
        if not self.steps:
            return 0.0
        completed = sum(1 for s in self.steps if s.status in (StepStatus.COMPLETED, StepStatus.SKIPPED))
        return completed / len(self.steps)


@dataclass
class PlannerConfig:
    """Configuration for the agentic planner."""
    max_steps: int = 10
    max_depth: int = 3
    parallel_execution: bool = True
    refinement_iterations: int = 2
    include_code_search: bool = True
    language: str = "python"


DECOMPOSITION_PROMPT = '''You are an expert software architect. Decompose complex problems into actionable steps.

## Problem
{query}

## Available Actions
- search: Search codebase for relevant code
- analyze: Analyze code structure or dependencies
- implement: Write or modify code
- test: Create or run tests
- review: Review code for issues
- document: Create documentation

## Output Format (JSON)
{{
    "goal": "High-level goal",
    "steps": [
        {{"id": "step_1", "title": "Brief title", "description": "Details", "action": "search", "dependencies": []}}
    ]
}}

Respond only with valid JSON.'''


class AgenticPlanner:
    """Agentic planner for decomposing complex problems."""

    def __init__(self, llm_client: Any, config: Optional[PlannerConfig] = None):
        self.llm = llm_client
        self.config = config or PlannerConfig()
        self._plans: dict[str, ExecutionPlan] = {}

    async def create_plan(self, query: str, context: Optional[dict] = None) -> ExecutionPlan:
        """Create an execution plan for a complex problem."""
        prompt = DECOMPOSITION_PROMPT.format(query=query)
        response = await self.llm.generate(prompt=prompt, max_tokens=2048, temperature=0.3)

        try:
            plan_data = json.loads(response.text)
        except json.JSONDecodeError:
            import re
            json_match = re.search(r'\{[\s\S]*\}', response.text)
            if json_match:
                plan_data = json.loads(json_match.group())
            else:
                raise ValueError("Failed to parse plan")

        plan_id = str(uuid.uuid4())[:8]
        steps = [
            PlanStep(
                id=step.get("id", f"step_{i}"),
                title=step["title"],
                description=step["description"],
                action=step["action"],
                dependencies=step.get("dependencies", []),
            )
            for i, step in enumerate(plan_data.get("steps", []))
        ]

        plan = ExecutionPlan(
            id=plan_id,
            query=query,
            goal=plan_data.get("goal", "Complete the task"),
            steps=steps[:self.config.max_steps],
            context=context or {},
        )
        self._plans[plan_id] = plan
        return plan

    async def stream_plan_execution(self, plan: ExecutionPlan, executor: Any) -> AsyncIterator[dict]:
        """Stream plan execution progress via SSE events."""
        plan.status = StepStatus.IN_PROGRESS
        yield {"event": "plan_start", "data": plan.to_dict()}

        while True:
            pending = plan.get_pending_steps()
            if not pending:
                break

            for step in pending:
                async for event in self._execute_step(plan, step, executor):
                    yield event

        plan.status = StepStatus.COMPLETED
        plan.final_answer = await self._generate_final_answer(plan)
        yield {"event": "plan_complete", "data": {"plan_id": plan.id, "final_answer": plan.final_answer}}

    async def _execute_step(self, plan: ExecutionPlan, step: PlanStep, executor: Any) -> AsyncIterator[dict]:
        """Execute a single step."""
        step.status = StepStatus.IN_PROGRESS
        step.started_at = datetime.now()
        yield {"event": "step_start", "data": {"plan_id": plan.id, "step": step.to_dict()}}

        try:
            result = await executor.execute(step, plan.context)
            step.status = StepStatus.COMPLETED
            step.result = result.output
            step.completed_at = datetime.now()
            plan.context[step.id] = result.output
            yield {"event": "step_complete", "data": {"plan_id": plan.id, "step": step.to_dict()}}
        except Exception as e:
            step.status = StepStatus.FAILED
            step.error = str(e)
            yield {"event": "step_error", "data": {"plan_id": plan.id, "step": step.to_dict(), "error": str(e)}}

    async def _generate_final_answer(self, plan: ExecutionPlan) -> str:
        """Generate final answer from step results."""
        results = "\n\n".join(f"### {s.title}\n{s.result or 'No result'}" for s in plan.steps if s.status == StepStatus.COMPLETED)
        prompt = f"Synthesize results for: {plan.query}\n\n{results}"
        response = await self.llm.generate(prompt=prompt, max_tokens=4096, temperature=0.3)
        return response.text

    def get_plan(self, plan_id: str) -> Optional[ExecutionPlan]:
        return self._plans.get(plan_id)
