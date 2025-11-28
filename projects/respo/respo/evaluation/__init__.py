"""
RESPO Evaluation Module

LLM-as-Judge evaluation system for code quality assessment.

Features:
- LLMJudge: Use GPT-4/Claude to evaluate code quality
- Metrics: Static code analysis (complexity, maintainability)
- Benchmark: Standardized benchmark suite
- Comparators: Compare against Copilot, GPT-4, Claude, etc.

Usage:
    from respo.evaluation import LLMJudge, JudgeConfig

    judge = LLMJudge(JudgeConfig(model="gpt-4"))
    
    # Evaluate single response
    result = await judge.evaluate(
        code="def hello(): return 'world'",
        task_description="Write a hello function",
        language="python"
    )
    
    # Compare two responses
    comparison = await judge.compare(
        response_a=respo_output,
        response_b=copilot_output,
        task_description="Implement LRU cache"
    )
"""

from respo.evaluation.judge import (
    LLMJudge,
    JudgeConfig,
    JudgeResult,
    JudgeCriteria,
    ComparisonResult,
    CriterionScore,
)
from respo.evaluation.metrics import (
    CodeQualityMetrics,
    calculate_code_metrics,
    calculate_similarity,
)
from respo.evaluation.benchmark import (
    Benchmark,
    BenchmarkTask,
    BenchmarkCategory,
    BenchmarkResult,
    BenchmarkSuite,
    TaskResult,
    run_benchmark,
)
from respo.evaluation.comparators import (
    BaseComparator,
    CompletionRequest,
    CompletionResponse,
    CopilotComparator,
    GPT4Comparator,
    ClaudeComparator,
    ReferenceComparator,
    VLLMComparator,
)

__all__ = [
    # Judge
    "LLMJudge",
    "JudgeConfig",
    "JudgeResult",
    "JudgeCriteria",
    "ComparisonResult",
    "CriterionScore",
    # Metrics
    "CodeQualityMetrics",
    "calculate_code_metrics",
    "calculate_similarity",
    # Benchmark
    "Benchmark",
    "BenchmarkTask",
    "BenchmarkCategory",
    "BenchmarkResult",
    "BenchmarkSuite",
    "TaskResult",
    "run_benchmark",
    # Comparators
    "BaseComparator",
    "CompletionRequest",
    "CompletionResponse",
    "CopilotComparator",
    "GPT4Comparator",
    "ClaudeComparator",
    "ReferenceComparator",
    "VLLMComparator",
]
