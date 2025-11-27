"""
Benchmark System

Standardized benchmarks for evaluating code generation quality.
"""

import asyncio
import json
import time
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Any, Optional

import structlog

from respo.evaluation.judge import LLMJudge, JudgeConfig, JudgeResult, ComparisonResult
from respo.evaluation.metrics import calculate_code_metrics, calculate_similarity, CodeQualityMetrics
from respo.evaluation.comparators import BaseComparator, CompletionRequest

logger = structlog.get_logger(__name__)


class BenchmarkCategory(Enum):
    """Categories of coding tasks."""
    
    COMPLETION = "completion"
    EXPLANATION = "explanation"
    DEBUGGING = "debugging"
    REFACTORING = "refactoring"
    TESTING = "testing"
    DOCUMENTATION = "documentation"
    ALGORITHM = "algorithm"
    API_USAGE = "api_usage"


@dataclass
class BenchmarkTask:
    """A single benchmark task."""
    
    id: str
    category: BenchmarkCategory
    prompt: str
    language: str
    
    # Optional reference solution
    reference_solution: Optional[str] = None
    
    # Expected behavior
    expected_output: Optional[str] = None
    test_cases: list[dict] = field(default_factory=list)
    
    # Metadata
    difficulty: str = "medium"  # easy, medium, hard
    tags: list[str] = field(default_factory=list)
    source: Optional[str] = None


@dataclass
class TaskResult:
    """Result for a single benchmark task."""
    
    task_id: str
    response: str
    
    # Evaluation
    judge_result: Optional[JudgeResult] = None
    comparison_result: Optional[ComparisonResult] = None
    metrics: Optional[CodeQualityMetrics] = None
    
    # Test results (if applicable)
    tests_passed: Optional[int] = None
    tests_total: Optional[int] = None
    
    # Timing
    generation_time_ms: float = 0
    evaluation_time_ms: float = 0
    
    def to_dict(self) -> dict:
        return {
            "task_id": self.task_id,
            "response": self.response,
            "judge_result": self.judge_result.to_dict() if self.judge_result else None,
            "comparison_result": self.comparison_result.to_dict() if self.comparison_result else None,
            "metrics": self.metrics.to_dict() if self.metrics else None,
            "tests_passed": self.tests_passed,
            "tests_total": self.tests_total,
            "generation_time_ms": self.generation_time_ms,
            "evaluation_time_ms": self.evaluation_time_ms,
        }


@dataclass
class BenchmarkResult:
    """Aggregate result for a benchmark run."""
    
    benchmark_name: str
    model_name: str
    timestamp: str
    
    # Aggregate scores
    average_score: float
    median_score: float
    
    # Per-category scores
    category_scores: dict[str, float]
    
    # Individual results
    task_results: list[TaskResult]
    
    # Comparison (if applicable)
    comparison_summary: Optional[dict] = None
    
    # Statistics
    total_tasks: int = 0
    successful_tasks: int = 0
    failed_tasks: int = 0
    total_time_ms: float = 0
    
    def to_dict(self) -> dict:
        return {
            "benchmark_name": self.benchmark_name,
            "model_name": self.model_name,
            "timestamp": self.timestamp,
            "average_score": self.average_score,
            "median_score": self.median_score,
            "category_scores": self.category_scores,
            "task_results": [tr.to_dict() for tr in self.task_results],
            "comparison_summary": self.comparison_summary,
            "total_tasks": self.total_tasks,
            "successful_tasks": self.successful_tasks,
            "failed_tasks": self.failed_tasks,
            "total_time_ms": self.total_time_ms,
        }
    
    def save(self, filepath: str):
        """Save results to JSON file."""
        Path(filepath).write_text(json.dumps(self.to_dict(), indent=2))


class Benchmark:
    """A benchmark consisting of multiple tasks."""
    
    def __init__(
        self,
        name: str,
        tasks: list[BenchmarkTask],
        judge_config: Optional[JudgeConfig] = None,
    ):
        self.name = name
        self.tasks = tasks
        self.judge = LLMJudge(judge_config or JudgeConfig())
    
    async def run(
        self,
        model: BaseComparator,
        reference: Optional[BaseComparator] = None,
        parallel: int = 1,
    ) -> BenchmarkResult:
        """
        Run the benchmark.
        
        Args:
            model: The model to evaluate
            reference: Optional reference model for comparison
            parallel: Number of parallel tasks
        """
        from datetime import datetime
        
        logger.info("Starting benchmark", name=self.name, tasks=len(self.tasks))
        start_time = time.time()
        
        task_results = []
        
        # Process tasks
        semaphore = asyncio.Semaphore(parallel)
        
        async def process_task(task: BenchmarkTask) -> TaskResult:
            async with semaphore:
                return await self._evaluate_task(task, model, reference)
        
        # Run all tasks
        results = await asyncio.gather(
            *[process_task(task) for task in self.tasks],
            return_exceptions=True,
        )
        
        for result in results:
            if isinstance(result, Exception):
                logger.error("Task failed", error=str(result))
            else:
                task_results.append(result)
        
        # Calculate aggregate scores
        scores = [
            tr.judge_result.overall_score 
            for tr in task_results 
            if tr.judge_result
        ]
        
        avg_score = sum(scores) / len(scores) if scores else 0
        sorted_scores = sorted(scores)
        median_score = sorted_scores[len(sorted_scores) // 2] if sorted_scores else 0
        
        # Per-category scores
        category_scores = {}
        for category in BenchmarkCategory:
            cat_scores = [
                tr.judge_result.overall_score
                for tr in task_results
                if tr.judge_result and self._get_task(tr.task_id).category == category
            ]
            if cat_scores:
                category_scores[category.value] = sum(cat_scores) / len(cat_scores)
        
        # Comparison summary
        comparison_summary = None
        if reference:
            wins = sum(1 for tr in task_results if tr.comparison_result and tr.comparison_result.winner == "A")
            losses = sum(1 for tr in task_results if tr.comparison_result and tr.comparison_result.winner == "B")
            ties = sum(1 for tr in task_results if tr.comparison_result and tr.comparison_result.winner == "tie")
            
            comparison_summary = {
                "model_wins": wins,
                "reference_wins": losses,
                "ties": ties,
                "win_rate": wins / len(task_results) if task_results else 0,
            }
        
        total_time = (time.time() - start_time) * 1000
        
        return BenchmarkResult(
            benchmark_name=self.name,
            model_name=model.name,
            timestamp=datetime.now().isoformat(),
            average_score=avg_score,
            median_score=median_score,
            category_scores=category_scores,
            task_results=task_results,
            comparison_summary=comparison_summary,
            total_tasks=len(self.tasks),
            successful_tasks=len(task_results),
            failed_tasks=len(self.tasks) - len(task_results),
            total_time_ms=total_time,
        )
    
    async def _evaluate_task(
        self,
        task: BenchmarkTask,
        model: BaseComparator,
        reference: Optional[BaseComparator],
    ) -> TaskResult:
        """Evaluate a single task."""
        # Generate response
        gen_start = time.time()
        response = await model.complete(CompletionRequest(
            prompt=task.prompt,
            language=task.language,
        ))
        gen_time = (time.time() - gen_start) * 1000
        
        # Evaluate with judge
        eval_start = time.time()
        judge_result = await self.judge.evaluate(
            code=response.code,
            task_description=task.prompt,
            language=task.language,
        )
        
        # Compare with reference if available
        comparison_result = None
        if reference:
            ref_response = await reference.complete(CompletionRequest(
                prompt=task.prompt,
                language=task.language,
            ))
            
            comparison_result = await self.judge.compare(
                response_a=response.code,
                response_b=ref_response.code,
                task_description=task.prompt,
                language=task.language,
                labels=(model.name, reference.name),
            )
        
        eval_time = (time.time() - eval_start) * 1000
        
        # Calculate static metrics
        metrics = calculate_code_metrics(response.code, task.language)
        
        return TaskResult(
            task_id=task.id,
            response=response.code,
            judge_result=judge_result,
            comparison_result=comparison_result,
            metrics=metrics,
            generation_time_ms=gen_time,
            evaluation_time_ms=eval_time,
        )
    
    def _get_task(self, task_id: str) -> Optional[BenchmarkTask]:
        """Get task by ID."""
        for task in self.tasks:
            if task.id == task_id:
                return task
        return None
    
    @classmethod
    def from_file(cls, filepath: str, judge_config: Optional[JudgeConfig] = None) -> "Benchmark":
        """
        Load benchmark from JSON file.
        
        Expected format:
        {
            "name": "benchmark_name",
            "tasks": [
                {
                    "id": "task1",
                    "category": "completion",
                    "prompt": "...",
                    "language": "python",
                    "reference_solution": "...",
                    "difficulty": "medium",
                    "tags": ["list", "algorithm"]
                }
            ]
        }
        """
        data = json.loads(Path(filepath).read_text())
        
        tasks = [
            BenchmarkTask(
                id=t["id"],
                category=BenchmarkCategory(t["category"]),
                prompt=t["prompt"],
                language=t.get("language", "python"),
                reference_solution=t.get("reference_solution"),
                expected_output=t.get("expected_output"),
                test_cases=t.get("test_cases", []),
                difficulty=t.get("difficulty", "medium"),
                tags=t.get("tags", []),
                source=t.get("source"),
            )
            for t in data["tasks"]
        ]
        
        return cls(name=data["name"], tasks=tasks, judge_config=judge_config)


class BenchmarkSuite:
    """Collection of benchmarks."""
    
    def __init__(self, benchmarks: list[Benchmark]):
        self.benchmarks = benchmarks
    
    async def run_all(
        self,
        model: BaseComparator,
        reference: Optional[BaseComparator] = None,
    ) -> list[BenchmarkResult]:
        """Run all benchmarks."""
        results = []
        for benchmark in self.benchmarks:
            result = await benchmark.run(model, reference)
            results.append(result)
        return results
    
    @classmethod
    def standard_suite(cls, judge_config: Optional[JudgeConfig] = None) -> "BenchmarkSuite":
        """Create standard benchmark suite with common tasks."""
        
        # Python completion tasks
        python_tasks = [
            BenchmarkTask(
                id="py_fibonacci",
                category=BenchmarkCategory.ALGORITHM,
                prompt="Write a Python function to calculate the nth Fibonacci number efficiently using memoization.",
                language="python",
                difficulty="easy",
                tags=["recursion", "memoization", "dynamic_programming"],
            ),
            BenchmarkTask(
                id="py_binary_search",
                category=BenchmarkCategory.ALGORITHM,
                prompt="Implement binary search in Python that returns the index of the target element or -1 if not found.",
                language="python",
                difficulty="easy",
                tags=["search", "algorithm"],
            ),
            BenchmarkTask(
                id="py_lru_cache",
                category=BenchmarkCategory.ALGORITHM,
                prompt="Implement an LRU (Least Recently Used) cache class in Python with get and put methods. Capacity should be configurable.",
                language="python",
                difficulty="medium",
                tags=["cache", "data_structure"],
            ),
            BenchmarkTask(
                id="py_async_http",
                category=BenchmarkCategory.API_USAGE,
                prompt="Write an async Python function that fetches data from multiple URLs concurrently using aiohttp and returns results as a list.",
                language="python",
                difficulty="medium",
                tags=["async", "http", "concurrency"],
            ),
            BenchmarkTask(
                id="py_decorator",
                category=BenchmarkCategory.COMPLETION,
                prompt="Write a Python decorator that caches function results based on arguments, with an optional TTL (time-to-live) in seconds.",
                language="python",
                difficulty="medium",
                tags=["decorator", "caching"],
            ),
        ]
        
        # JavaScript tasks
        js_tasks = [
            BenchmarkTask(
                id="js_debounce",
                category=BenchmarkCategory.COMPLETION,
                prompt="Implement a debounce function in JavaScript that delays invoking a function until after a specified wait time has elapsed since the last call.",
                language="javascript",
                difficulty="medium",
                tags=["utility", "timing"],
            ),
            BenchmarkTask(
                id="js_promise_all",
                category=BenchmarkCategory.ALGORITHM,
                prompt="Implement a simplified version of Promise.all in JavaScript that resolves when all promises resolve or rejects if any promise rejects.",
                language="javascript",
                difficulty="medium",
                tags=["promise", "async"],
            ),
        ]
        
        # Debugging tasks
        debug_tasks = [
            BenchmarkTask(
                id="debug_off_by_one",
                category=BenchmarkCategory.DEBUGGING,
                prompt="""Fix the bug in this Python code:
```python
def get_last_n_elements(lst, n):
    return lst[len(lst) - n - 1:]
```
The function should return the last n elements of the list.""",
                language="python",
                difficulty="easy",
                tags=["debugging", "indexing"],
            ),
        ]
        
        # Explanation tasks
        explain_tasks = [
            BenchmarkTask(
                id="explain_decorator",
                category=BenchmarkCategory.EXPLANATION,
                prompt="""Explain what this Python decorator does and how it works:
```python
def retry(max_attempts=3, delay=1):
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            for attempt in range(max_attempts):
                try:
                    return func(*args, **kwargs)
                except Exception as e:
                    if attempt == max_attempts - 1:
                        raise
                    time.sleep(delay * (2 ** attempt))
            return wrapper
        return decorator
    return decorator
```""",
                language="python",
                difficulty="medium",
                tags=["explanation", "decorator"],
            ),
        ]
        
        all_tasks = python_tasks + js_tasks + debug_tasks + explain_tasks
        
        benchmark = Benchmark(
            name="RESPO Standard Benchmark",
            tasks=all_tasks,
            judge_config=judge_config,
        )
        
        return cls(benchmarks=[benchmark])


async def run_benchmark(
    model: BaseComparator,
    reference: Optional[BaseComparator] = None,
    benchmark_file: Optional[str] = None,
    use_standard: bool = True,
    output_file: Optional[str] = None,
) -> BenchmarkResult:
    """
    Convenience function to run benchmarks.
    
    Args:
        model: Model to evaluate
        reference: Optional reference for comparison
        benchmark_file: Path to benchmark JSON file
        use_standard: Use standard benchmark suite if no file provided
        output_file: Save results to this file
    
    Returns:
        BenchmarkResult
    """
    if benchmark_file:
        benchmark = Benchmark.from_file(benchmark_file)
    elif use_standard:
        suite = BenchmarkSuite.standard_suite()
        benchmark = suite.benchmarks[0]
    else:
        raise ValueError("Must provide benchmark_file or use_standard=True")
    
    result = await benchmark.run(model, reference)
    
    if output_file:
        result.save(output_file)
        logger.info("Results saved", file=output_file)
    
    return result
