"""Tests for Agentic Planning and Deep Research."""

import pytest
from unittest.mock import AsyncMock, MagicMock, patch

from respo.agents.planner import AgenticPlanner, ExecutionPlan, PlanStep
from respo.agents.executor import StepExecutor
from respo.agents.deep_research import DeepResearchAgent, ResearchResult


class TestAgenticPlanner:
    """Tests for AgenticPlanner."""

    @pytest.fixture
    def mock_llm(self):
        """Create a mock LLM client."""
        llm = AsyncMock()
        llm.generate = AsyncMock(return_value=MagicMock(
            choices=[MagicMock(message=MagicMock(content="""
{
    "goal": "Implement OAuth2 with PKCE",
    "steps": [
        {
            "id": "step_1",
            "title": "Research OAuth2 PKCE flow",
            "description": "Understand the PKCE extension",
            "type": "research",
            "dependencies": []
        },
        {
            "id": "step_2",
            "title": "Implement code verifier",
            "description": "Generate cryptographic code verifier",
            "type": "implement",
            "dependencies": ["step_1"]
        }
    ],
    "estimated_time": "2 hours"
}
"""))]
        ))
        return llm

    @pytest.mark.asyncio
    async def test_create_plan(self, mock_llm):
        """Test creating an execution plan."""
        planner = AgenticPlanner(mock_llm)
        plan = await planner.create_plan("Implement OAuth2 with PKCE")
        
        assert plan is not None
        assert plan.goal == "Implement OAuth2 with PKCE"
        assert len(plan.steps) == 2
        assert plan.steps[0].id == "step_1"
        assert plan.steps[0].type == "research"

    @pytest.mark.asyncio
    async def test_plan_with_context(self, mock_llm):
        """Test creating a plan with additional context."""
        planner = AgenticPlanner(mock_llm)
        plan = await planner.create_plan(
            "Implement OAuth2",
            context={"language": "python", "framework": "fastapi"}
        )
        
        assert plan is not None
        mock_llm.generate.assert_called_once()


class TestStepExecutor:
    """Tests for StepExecutor."""

    @pytest.fixture
    def mock_llm(self):
        """Create a mock LLM client."""
        llm = AsyncMock()
        llm.generate = AsyncMock(return_value=MagicMock(
            choices=[MagicMock(message=MagicMock(content="""
{
    "result": "Code verifier implementation completed",
    "code": "import secrets\\ndef generate_verifier(): return secrets.token_urlsafe(32)",
    "artifacts": ["verifier.py"],
    "next_action": null
}
"""))]
        ))
        return llm

    @pytest.fixture
    def sample_step(self):
        """Create a sample plan step."""
        return PlanStep(
            id="step_1",
            title="Implement code verifier",
            description="Generate cryptographic code verifier",
            type="implement",
            dependencies=[]
        )

    @pytest.mark.asyncio
    async def test_execute_step(self, mock_llm, sample_step):
        """Test executing a single step."""
        executor = StepExecutor(mock_llm)
        result = await executor.execute(sample_step)
        
        assert result is not None
        assert "result" in result
        assert "code" in result

    @pytest.mark.asyncio
    async def test_execute_with_context(self, mock_llm, sample_step):
        """Test executing a step with context from previous steps."""
        executor = StepExecutor(mock_llm)
        context = {"previous_results": {"step_0": "Research completed"}}
        result = await executor.execute(sample_step, context=context)
        
        assert result is not None


class TestDeepResearchAgent:
    """Tests for DeepResearchAgent."""

    @pytest.fixture
    def mock_llm(self):
        """Create a mock LLM client."""
        llm = AsyncMock()
        # Mock for plan creation
        llm.generate = AsyncMock(side_effect=[
            # First call: create research plan
            MagicMock(choices=[MagicMock(message=MagicMock(content="""
{
    "goal": "Research LRU Cache",
    "steps": [
        {"id": "s1", "title": "Search", "description": "Find examples", "type": "search", "dependencies": []}
    ],
    "estimated_time": "30 min"
}
"""))]),
            # Second call: execute step
            MagicMock(choices=[MagicMock(message=MagicMock(content="""
{
    "result": "Found LRU cache implementations",
    "findings": ["Use OrderedDict", "Use collections.deque"],
    "code": "from collections import OrderedDict"
}
"""))]),
            # Third call: synthesize
            MagicMock(choices=[MagicMock(message=MagicMock(content="""
{
    "summary": "LRU Cache best practices",
    "key_findings": ["Use OrderedDict for O(1) operations"],
    "code_examples": [{"title": "Basic LRU", "code": "class LRU: pass", "language": "python"}],
    "recommendations": ["Use maxsize parameter"],
    "confidence": 0.9
}
"""))])
        ])
        return llm

    @pytest.mark.asyncio
    async def test_research(self, mock_llm):
        """Test deep research on a topic."""
        agent = DeepResearchAgent(mock_llm)
        result = await agent.research("Best practices for LRU Cache")
        
        assert result is not None
        assert isinstance(result, ResearchResult)
        assert result.summary is not None

    @pytest.mark.asyncio
    async def test_research_with_max_depth(self, mock_llm):
        """Test research with limited depth."""
        agent = DeepResearchAgent(mock_llm, max_depth=1)
        result = await agent.research("LRU Cache", max_iterations=1)
        
        assert result is not None


class TestPlanStep:
    """Tests for PlanStep dataclass."""

    def test_step_creation(self):
        """Test creating a plan step."""
        step = PlanStep(
            id="step_1",
            title="Research",
            description="Research the topic",
            type="research",
            dependencies=[]
        )
        
        assert step.id == "step_1"
        assert step.title == "Research"
        assert step.type == "research"
        assert step.dependencies == []

    def test_step_with_dependencies(self):
        """Test step with dependencies."""
        step = PlanStep(
            id="step_2",
            title="Implement",
            description="Implement based on research",
            type="implement",
            dependencies=["step_1"]
        )
        
        assert "step_1" in step.dependencies


class TestExecutionPlan:
    """Tests for ExecutionPlan dataclass."""

    def test_plan_creation(self):
        """Test creating an execution plan."""
        steps = [
            PlanStep(id="s1", title="Step 1", description="", type="research", dependencies=[]),
            PlanStep(id="s2", title="Step 2", description="", type="implement", dependencies=["s1"]),
        ]
        plan = ExecutionPlan(
            goal="Test goal",
            steps=steps,
            estimated_time="1 hour"
        )
        
        assert plan.goal == "Test goal"
        assert len(plan.steps) == 2
        assert plan.estimated_time == "1 hour"

    def test_plan_get_executable_steps(self):
        """Test getting steps that can be executed."""
        steps = [
            PlanStep(id="s1", title="Step 1", description="", type="research", dependencies=[]),
            PlanStep(id="s2", title="Step 2", description="", type="implement", dependencies=["s1"]),
        ]
        plan = ExecutionPlan(goal="Test", steps=steps, estimated_time="1h")
        
        # Only s1 should be executable initially (no dependencies)
        executable = plan.get_executable_steps(completed=[])
        assert len(executable) == 1
        assert executable[0].id == "s1"
        
        # After s1 is completed, s2 should be executable
        executable = plan.get_executable_steps(completed=["s1"])
        assert len(executable) == 1
        assert executable[0].id == "s2"
