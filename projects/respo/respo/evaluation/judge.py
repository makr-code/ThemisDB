"""
LLM-as-Judge Implementation

Uses an LLM to evaluate code quality and compare responses.
"""

import json
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Optional

import structlog

logger = structlog.get_logger(__name__)


class JudgeCriteria(Enum):
    """Evaluation criteria for code assessment."""
    
    CORRECTNESS = "correctness"
    EFFICIENCY = "efficiency"
    READABILITY = "readability"
    MAINTAINABILITY = "maintainability"
    SECURITY = "security"
    BEST_PRACTICES = "best_practices"
    DOCUMENTATION = "documentation"
    ERROR_HANDLING = "error_handling"
    TESTABILITY = "testability"
    COMPLETENESS = "completeness"


@dataclass
class JudgeConfig:
    """Configuration for the LLM Judge."""
    
    # Judge LLM settings
    model: str = "gpt-4"
    temperature: float = 0.1  # Low temperature for consistent judgments
    max_tokens: int = 2000
    
    # Evaluation settings
    criteria: list[JudgeCriteria] = field(default_factory=lambda: [
        JudgeCriteria.CORRECTNESS,
        JudgeCriteria.EFFICIENCY,
        JudgeCriteria.READABILITY,
        JudgeCriteria.BEST_PRACTICES,
    ])
    
    # Scoring
    score_scale: int = 10  # 1-10 scale
    require_explanation: bool = True
    
    # API settings
    api_base: Optional[str] = None
    api_key: Optional[str] = None


@dataclass
class CriterionScore:
    """Score for a single criterion."""
    
    criterion: JudgeCriteria
    score: float  # 0-10
    explanation: str
    suggestions: list[str] = field(default_factory=list)


@dataclass
class JudgeResult:
    """Result of a single code evaluation."""
    
    # Overall score
    overall_score: float  # 0-10
    grade: str  # A, B, C, D, F
    
    # Per-criterion scores
    criterion_scores: list[CriterionScore]
    
    # Summary
    summary: str
    strengths: list[str]
    weaknesses: list[str]
    improvements: list[str]
    
    # Metadata
    model_used: str
    evaluation_time_ms: float
    
    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary."""
        return {
            "overall_score": self.overall_score,
            "grade": self.grade,
            "criterion_scores": [
                {
                    "criterion": cs.criterion.value,
                    "score": cs.score,
                    "explanation": cs.explanation,
                    "suggestions": cs.suggestions,
                }
                for cs in self.criterion_scores
            ],
            "summary": self.summary,
            "strengths": self.strengths,
            "weaknesses": self.weaknesses,
            "improvements": self.improvements,
            "model_used": self.model_used,
            "evaluation_time_ms": self.evaluation_time_ms,
        }


@dataclass
class ComparisonResult:
    """Result of comparing two code responses."""
    
    # Winner
    winner: str  # "A", "B", or "tie"
    confidence: float  # 0-1
    
    # Individual evaluations
    response_a_score: float
    response_b_score: float
    
    # Comparison details
    criteria_comparison: dict[str, dict[str, Any]]
    
    # Summary
    summary: str
    key_differences: list[str]
    
    # Which is better for what
    a_better_at: list[str]
    b_better_at: list[str]
    
    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary."""
        return {
            "winner": self.winner,
            "confidence": self.confidence,
            "response_a_score": self.response_a_score,
            "response_b_score": self.response_b_score,
            "criteria_comparison": self.criteria_comparison,
            "summary": self.summary,
            "key_differences": self.key_differences,
            "a_better_at": self.a_better_at,
            "b_better_at": self.b_better_at,
        }


class LLMJudge:
    """
    LLM-based code quality judge.
    
    Uses a powerful LLM (GPT-4, Claude, etc.) to evaluate code quality
    and compare different responses.
    """
    
    def __init__(self, config: Optional[JudgeConfig] = None):
        """Initialize the judge."""
        self.config = config or JudgeConfig()
        self._client = None
    
    async def _get_client(self):
        """Get or create the LLM client."""
        if self._client is None:
            try:
                import openai
                self._client = openai.AsyncOpenAI(
                    api_key=self.config.api_key,
                    base_url=self.config.api_base,
                )
            except ImportError:
                raise ImportError("openai package required. Install with: pip install openai")
        return self._client
    
    async def _call_llm(self, system_prompt: str, user_prompt: str) -> str:
        """Make an LLM call."""
        client = await self._get_client()
        
        response = await client.chat.completions.create(
            model=self.config.model,
            messages=[
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": user_prompt},
            ],
            temperature=self.config.temperature,
            max_tokens=self.config.max_tokens,
        )
        
        return response.choices[0].message.content
    
    def _score_to_grade(self, score: float) -> str:
        """Convert numeric score to letter grade."""
        if score >= 9.0:
            return "A+"
        elif score >= 8.5:
            return "A"
        elif score >= 8.0:
            return "A-"
        elif score >= 7.5:
            return "B+"
        elif score >= 7.0:
            return "B"
        elif score >= 6.5:
            return "B-"
        elif score >= 6.0:
            return "C+"
        elif score >= 5.5:
            return "C"
        elif score >= 5.0:
            return "C-"
        elif score >= 4.0:
            return "D"
        else:
            return "F"
    
    async def evaluate(
        self,
        code: str,
        task_description: str,
        language: str = "python",
        context: Optional[str] = None,
    ) -> JudgeResult:
        """
        Evaluate a code response.
        
        Args:
            code: The code to evaluate
            task_description: What the code is supposed to do
            language: Programming language
            context: Additional context (e.g., original question)
        
        Returns:
            JudgeResult with scores and feedback
        """
        import time
        start_time = time.time()
        
        criteria_list = ", ".join([c.value for c in self.config.criteria])
        
        system_prompt = f"""You are an expert code reviewer and quality assessor.
Your task is to evaluate code based on specific criteria and provide detailed, actionable feedback.

Evaluation Criteria: {criteria_list}

Scoring Scale: 1-{self.config.score_scale}
- 9-10: Excellent, production-ready code
- 7-8: Good code with minor improvements possible
- 5-6: Acceptable but needs work
- 3-4: Poor, significant issues
- 1-2: Very poor, major problems

You MUST respond with valid JSON in this exact format:
{{
    "overall_score": <float>,
    "criterion_scores": [
        {{
            "criterion": "<criterion_name>",
            "score": <float>,
            "explanation": "<detailed explanation>",
            "suggestions": ["<improvement 1>", "<improvement 2>"]
        }}
    ],
    "summary": "<overall assessment>",
    "strengths": ["<strength 1>", "<strength 2>"],
    "weaknesses": ["<weakness 1>", "<weakness 2>"],
    "improvements": ["<actionable improvement 1>", "<improvement 2>"]
}}"""

        context_section = f"\n\nAdditional Context:\n{context}" if context else ""
        
        user_prompt = f"""Evaluate the following {language} code:

Task Description:
{task_description}
{context_section}

Code to Evaluate:
```{language}
{code}
```

Provide your evaluation as JSON."""

        response = await self._call_llm(system_prompt, user_prompt)
        
        # Parse JSON response
        try:
            # Extract JSON from response (handle markdown code blocks)
            json_str = response
            if "```json" in response:
                json_str = response.split("```json")[1].split("```")[0]
            elif "```" in response:
                json_str = response.split("```")[1].split("```")[0]
            
            data = json.loads(json_str)
        except json.JSONDecodeError:
            logger.error("Failed to parse judge response", response=response[:500])
            # Return a default result
            return JudgeResult(
                overall_score=5.0,
                grade="C",
                criterion_scores=[],
                summary="Failed to parse evaluation",
                strengths=[],
                weaknesses=["Evaluation parsing failed"],
                improvements=[],
                model_used=self.config.model,
                evaluation_time_ms=(time.time() - start_time) * 1000,
            )
        
        # Build result
        criterion_scores = []
        for cs in data.get("criterion_scores", []):
            try:
                criterion = JudgeCriteria(cs["criterion"])
            except ValueError:
                continue
            
            criterion_scores.append(CriterionScore(
                criterion=criterion,
                score=float(cs.get("score", 5.0)),
                explanation=cs.get("explanation", ""),
                suggestions=cs.get("suggestions", []),
            ))
        
        overall_score = float(data.get("overall_score", 5.0))
        
        return JudgeResult(
            overall_score=overall_score,
            grade=self._score_to_grade(overall_score),
            criterion_scores=criterion_scores,
            summary=data.get("summary", ""),
            strengths=data.get("strengths", []),
            weaknesses=data.get("weaknesses", []),
            improvements=data.get("improvements", []),
            model_used=self.config.model,
            evaluation_time_ms=(time.time() - start_time) * 1000,
        )
    
    async def compare(
        self,
        response_a: str,
        response_b: str,
        task_description: str,
        language: str = "python",
        labels: tuple[str, str] = ("Response A", "Response B"),
    ) -> ComparisonResult:
        """
        Compare two code responses.
        
        Args:
            response_a: First code response
            response_b: Second code response
            task_description: What the code should do
            language: Programming language
            labels: Labels for the responses (e.g., ("RESPO", "Copilot"))
        
        Returns:
            ComparisonResult with winner and details
        """
        import time
        start_time = time.time()
        
        criteria_list = ", ".join([c.value for c in self.config.criteria])
        
        system_prompt = f"""You are an expert code reviewer comparing two code responses.
Your task is to determine which response is better and explain why.

Evaluation Criteria: {criteria_list}

Be objective and thorough. Consider all criteria when making your judgment.

You MUST respond with valid JSON in this exact format:
{{
    "winner": "A" | "B" | "tie",
    "confidence": <float 0-1>,
    "response_a_score": <float 1-10>,
    "response_b_score": <float 1-10>,
    "criteria_comparison": {{
        "<criterion>": {{
            "a_score": <float>,
            "b_score": <float>,
            "winner": "A" | "B" | "tie",
            "explanation": "<why>"
        }}
    }},
    "summary": "<overall comparison summary>",
    "key_differences": ["<difference 1>", "<difference 2>"],
    "a_better_at": ["<aspect 1>"],
    "b_better_at": ["<aspect 1>"]
}}"""

        user_prompt = f"""Compare these two {language} code responses:

Task Description:
{task_description}

=== {labels[0]} ===
```{language}
{response_a}
```

=== {labels[1]} ===
```{language}
{response_b}
```

Which response is better? Provide your comparison as JSON."""

        response = await self._call_llm(system_prompt, user_prompt)
        
        # Parse JSON response
        try:
            json_str = response
            if "```json" in response:
                json_str = response.split("```json")[1].split("```")[0]
            elif "```" in response:
                json_str = response.split("```")[1].split("```")[0]
            
            data = json.loads(json_str)
        except json.JSONDecodeError:
            logger.error("Failed to parse comparison response", response=response[:500])
            return ComparisonResult(
                winner="tie",
                confidence=0.0,
                response_a_score=5.0,
                response_b_score=5.0,
                criteria_comparison={},
                summary="Failed to parse comparison",
                key_differences=[],
                a_better_at=[],
                b_better_at=[],
            )
        
        return ComparisonResult(
            winner=data.get("winner", "tie"),
            confidence=float(data.get("confidence", 0.5)),
            response_a_score=float(data.get("response_a_score", 5.0)),
            response_b_score=float(data.get("response_b_score", 5.0)),
            criteria_comparison=data.get("criteria_comparison", {}),
            summary=data.get("summary", ""),
            key_differences=data.get("key_differences", []),
            a_better_at=data.get("a_better_at", []),
            b_better_at=data.get("b_better_at", []),
        )
    
    async def evaluate_against_reference(
        self,
        candidate: str,
        reference: str,
        task_description: str,
        language: str = "python",
        reference_source: str = "Reference",
    ) -> ComparisonResult:
        """
        Evaluate a candidate response against a reference (e.g., Copilot output).
        
        Args:
            candidate: The code to evaluate (e.g., RESPO output)
            reference: The reference code (e.g., Copilot output)
            task_description: What the code should do
            language: Programming language
            reference_source: Name of reference source
        
        Returns:
            ComparisonResult
        """
        return await self.compare(
            response_a=candidate,
            response_b=reference,
            task_description=task_description,
            language=language,
            labels=("Candidate", reference_source),
        )
