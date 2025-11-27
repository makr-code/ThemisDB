"""
RESPO CLI

Command-line interface for the RAG-Enhanced Software Programming Optimizer.
"""

import argparse
import asyncio
import json
import sys
from pathlib import Path
from typing import Optional

import structlog

from respo.ingestion import GitHubScraper, ScraperConfig, parse_github_url

logger = structlog.get_logger(__name__)


# Console output colors
class Colors:
    """ANSI color codes for console output."""
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'


def print_colored(text: str, color: str = "") -> None:
    """Print colored text."""
    print(f"{color}{text}{Colors.END}")


def setup_logging(verbose: bool = False) -> None:
    """Configure logging based on verbosity."""
    import logging

    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format="%(message)s",
    )

    if verbose:
        structlog.configure(
            processors=[
                structlog.stdlib.filter_by_level,
                structlog.dev.ConsoleRenderer(),
            ],
            wrapper_class=structlog.stdlib.BoundLogger,
            context_class=dict,
            logger_factory=structlog.stdlib.LoggerFactory(),
        )


async def cmd_scrape(args: argparse.Namespace) -> int:
    """Scrape a GitHub repository."""
    setup_logging(args.verbose)

    # Parse repository URL or owner/repo
    try:
        owner, repo = parse_github_url(args.repository)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print(f"Scraping repository: {owner}/{repo}")

    # Configure scraper
    config = ScraperConfig(
        github_token=args.token,
        max_file_size_kb=args.max_file_size,
        clone_depth=args.depth,
    )

    if args.extensions:
        config.include_extensions = [
            ext if ext.startswith(".") else f".{ext}"
            for ext in args.extensions.split(",")
        ]

    # Output setup
    output_path = Path(args.output) if args.output else None
    if output_path:
        output_path.mkdir(parents=True, exist_ok=True)

    # Scrape repository
    file_count = 0
    total_size = 0
    files_data = []

    async with GitHubScraper(config) as scraper:
        async for code_file in scraper.scrape_repository(
            owner=owner,
            repo=repo,
            branch=args.branch,
        ):
            file_count += 1
            total_size += code_file.size_bytes

            if args.verbose:
                print(f"  [{code_file.language}] {code_file.path} ({code_file.size_bytes} bytes)")

            if output_path:
                # Save file content
                file_output = output_path / code_file.path
                file_output.parent.mkdir(parents=True, exist_ok=True)
                file_output.write_text(code_file.content)

            if args.json_output:
                files_data.append({
                    "path": code_file.path,
                    "language": code_file.language,
                    "size_bytes": code_file.size_bytes,
                    "repo": code_file.repo,
                    "branch": code_file.branch,
                    "commit_sha": code_file.commit_sha,
                    "license": code_file.license,
                    "metadata": code_file.metadata,
                })

    # Summary
    print(f"\nScraped {file_count} files ({total_size / 1024:.1f} KB)")

    if args.json_output:
        json_path = Path(args.json_output)
        json_path.write_text(json.dumps(files_data, indent=2))
        print(f"Metadata saved to: {json_path}")

    if output_path:
        print(f"Files saved to: {output_path}")

    return 0


async def cmd_search(args: argparse.Namespace) -> int:
    """Search for GitHub repositories."""
    setup_logging(args.verbose)

    config = ScraperConfig(github_token=args.token)

    print(f"Searching for: {args.query}")
    if args.language:
        print(f"Language filter: {args.language}")
    if args.min_stars:
        print(f"Minimum stars: {args.min_stars}")

    async with GitHubScraper(config) as scraper:
        results = await scraper.search_repositories(
            query=args.query,
            language=args.language,
            min_stars=args.min_stars,
            max_results=args.limit,
        )

    print(f"\nFound {len(results)} repositories:\n")

    for repo in results:
        stars = repo["stars"]
        forks = repo["forks"]
        lang = repo.get("language", "Unknown")
        desc = repo.get("description", "No description")[:60]
        if len(repo.get("description", "")) > 60:
            desc += "..."

        print(f"  ⭐ {stars:>6} | 🍴 {forks:>5} | [{lang:>12}] {repo['full_name']}")
        print(f"           {desc}")
        print()

    if args.json_output:
        json_path = Path(args.json_output)
        json_path.write_text(json.dumps(results, indent=2))
        print(f"Results saved to: {json_path}")

    return 0


async def cmd_batch_scrape(args: argparse.Namespace) -> int:
    """Batch scrape multiple repositories from a file."""
    setup_logging(args.verbose)

    # Read repository list
    repos_file = Path(args.repos_file)
    if not repos_file.exists():
        print(f"Error: File not found: {repos_file}", file=sys.stderr)
        return 1

    repos = []
    for line in repos_file.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        try:
            owner, repo = parse_github_url(line)
            repos.append((owner, repo))
        except ValueError as e:
            print(f"Warning: Skipping invalid entry: {line} ({e})")

    print(f"Batch scraping {len(repos)} repositories")

    config = ScraperConfig(
        github_token=args.token,
        max_file_size_kb=args.max_file_size,
    )

    output_path = Path(args.output) if args.output else None
    if output_path:
        output_path.mkdir(parents=True, exist_ok=True)

    total_files = 0
    total_size = 0
    all_files_data = []

    async with GitHubScraper(config) as scraper:
        for owner, repo in repos:
            print(f"\nScraping: {owner}/{repo}")
            file_count = 0

            try:
                async for code_file in scraper.scrape_repository(owner, repo):
                    file_count += 1
                    total_files += 1
                    total_size += code_file.size_bytes

                    if output_path:
                        file_output = output_path / code_file.repo / code_file.path
                        file_output.parent.mkdir(parents=True, exist_ok=True)
                        file_output.write_text(code_file.content)

                    if args.json_output:
                        all_files_data.append({
                            "path": code_file.path,
                            "language": code_file.language,
                            "size_bytes": code_file.size_bytes,
                            "repo": code_file.repo,
                            "branch": code_file.branch,
                            "commit_sha": code_file.commit_sha,
                            "license": code_file.license,
                        })

                print(f"  → {file_count} files")

            except Exception as e:
                print(f"  → Error: {e}")

    print(f"\n{'='*50}")
    print(f"Total: {total_files} files ({total_size / 1024 / 1024:.1f} MB)")

    if args.json_output:
        json_path = Path(args.json_output)
        json_path.write_text(json.dumps(all_files_data, indent=2))
        print(f"Metadata saved to: {json_path}")

    return 0


async def cmd_ingest(args: argparse.Namespace) -> int:
    """Ingest code into the vector store."""
    setup_logging(args.verbose)

    source = args.source
    source_type = args.type

    # Auto-detect source type
    if source_type == "auto":
        if source.startswith("http") or "/" in source and not Path(source).exists():
            source_type = "github"
        else:
            source_type = "directory"

    print(f"Ingesting from {source_type}: {source}")

    # Initialize components
    from respo.config import get_settings
    from respo.vectorstore.base import VectorStoreFactory
    from respo.embedding import CodeEmbedder
    from respo.ingestion.pipeline import IngestionPipeline

    settings = get_settings()

    try:
        vector_store = VectorStoreFactory.create(
            settings.vector_store.backend,
            persist_directory=settings.vector_store.chroma_persist_dir,
        )
    except Exception as e:
        print(f"Error initializing vector store: {e}", file=sys.stderr)
        return 1

    try:
        embedder = CodeEmbedder(
            model_name=settings.embedding.model,
            device=settings.embedding.device,
        )
    except Exception as e:
        print(f"Error initializing embedder: {e}", file=sys.stderr)
        return 1

    pipeline = IngestionPipeline(
        vector_store=vector_store,
        embedder=embedder,
    )

    # Ingest based on source type
    if source_type == "github":
        try:
            owner, repo = parse_github_url(source)
        except ValueError as e:
            print(f"Error: {e}", file=sys.stderr)
            return 1

        result = await pipeline.ingest_github_repo(
            owner=owner,
            repo=repo,
            token=args.token,
        )
    else:
        source_path = Path(source)
        if not source_path.exists():
            print(f"Error: Path not found: {source}", file=sys.stderr)
            return 1

        extensions = None
        if args.extensions:
            extensions = [
                ext if ext.startswith(".") else f".{ext}"
                for ext in args.extensions.split(",")
            ]

        result = await pipeline.ingest_directory(
            path=source_path,
            extensions=extensions,
        )

    print(f"\n{Colors.GREEN}✓ Ingestion complete{Colors.END}")
    print(f"  Processed: {result.files_processed} files")
    print(f"  Chunks: {result.chunks_created}")
    print(f"  Duration: {result.duration_seconds:.2f}s")

    if result.errors:
        print(f"\n{Colors.YELLOW}Warnings:{Colors.END}")
        for error in result.errors[:5]:
            print(f"  - {error}")
        if len(result.errors) > 5:
            print(f"  ... and {len(result.errors) - 5} more")

    await vector_store.close()
    return 0


async def cmd_evaluate(args: argparse.Namespace) -> int:
    """Evaluate code quality using LLM-as-Judge."""
    setup_logging(args.verbose)

    # Get code to evaluate
    if args.code:
        code = args.code
    elif args.code_file:
        code_path = Path(args.code_file)
        if not code_path.exists():
            print(f"Error: File not found: {args.code_file}", file=sys.stderr)
            return 1
        code = code_path.read_text()
    else:
        print("Error: Provide --code or a code file", file=sys.stderr)
        return 1

    print(f"Evaluating code ({len(code)} chars) with {args.model}...")

    from respo.evaluation import LLMJudge, JudgeConfig

    config = JudgeConfig(model=args.model)
    judge = LLMJudge(config)

    try:
        result = await judge.evaluate(
            code=code,
            task_description=args.task,
            language=args.language,
        )
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    # Display results
    print(f"\n{'='*50}")
    print(f"{Colors.BOLD}Evaluation Results{Colors.END}")
    print(f"{'='*50}")

    grade_color = Colors.GREEN if result.overall_score >= 7 else (Colors.YELLOW if result.overall_score >= 5 else Colors.RED)
    print(f"\n{Colors.BOLD}Overall Score:{Colors.END} {grade_color}{result.overall_score:.1f}/10 ({result.grade}){Colors.END}")

    print(f"\n{Colors.BOLD}Summary:{Colors.END}")
    print(f"  {result.summary}")

    if result.criterion_scores:
        print(f"\n{Colors.BOLD}Criterion Scores:{Colors.END}")
        for cs in result.criterion_scores:
            score_color = Colors.GREEN if cs.score >= 7 else (Colors.YELLOW if cs.score >= 5 else Colors.RED)
            print(f"  {cs.criterion.value:20} {score_color}{cs.score:.1f}/10{Colors.END}")
            if args.verbose and cs.explanation:
                print(f"    {cs.explanation}")

    if result.strengths:
        print(f"\n{Colors.GREEN}Strengths:{Colors.END}")
        for s in result.strengths:
            print(f"  ✓ {s}")

    if result.weaknesses:
        print(f"\n{Colors.YELLOW}Weaknesses:{Colors.END}")
        for w in result.weaknesses:
            print(f"  ✗ {w}")

    if result.improvements:
        print(f"\n{Colors.CYAN}Improvements:{Colors.END}")
        for i in result.improvements:
            print(f"  → {i}")

    if args.json_output:
        json_path = Path(args.json_output)
        json_path.write_text(json.dumps(result.to_dict(), indent=2))
        print(f"\nResults saved to: {json_path}")

    return 0


async def cmd_compare(args: argparse.Namespace) -> int:
    """Compare two code files using LLM-as-Judge."""
    setup_logging(args.verbose)

    # Read files
    file_a = Path(args.file_a)
    file_b = Path(args.file_b)

    if not file_a.exists():
        print(f"Error: File not found: {args.file_a}", file=sys.stderr)
        return 1
    if not file_b.exists():
        print(f"Error: File not found: {args.file_b}", file=sys.stderr)
        return 1

    code_a = file_a.read_text()
    code_b = file_b.read_text()

    print(f"Comparing {file_a.name} vs {file_b.name} with {args.model}...")

    from respo.evaluation import LLMJudge, JudgeConfig

    config = JudgeConfig(model=args.model)
    judge = LLMJudge(config)

    try:
        result = await judge.compare(
            response_a=code_a,
            response_b=code_b,
            task_description=args.task,
            language=args.language,
            labels=(file_a.name, file_b.name),
        )
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    # Display results
    print(f"\n{'='*50}")
    print(f"{Colors.BOLD}Comparison Results{Colors.END}")
    print(f"{'='*50}")

    winner_label = file_a.name if result.winner == "A" else (file_b.name if result.winner == "B" else "Tie")
    winner_color = Colors.GREEN if result.winner != "tie" else Colors.YELLOW
    print(f"\n{Colors.BOLD}Winner:{Colors.END} {winner_color}{winner_label}{Colors.END} (confidence: {result.confidence:.0%})")

    print(f"\n{Colors.BOLD}Scores:{Colors.END}")
    print(f"  {file_a.name}: {result.response_a_score:.1f}/10")
    print(f"  {file_b.name}: {result.response_b_score:.1f}/10")

    print(f"\n{Colors.BOLD}Summary:{Colors.END}")
    print(f"  {result.summary}")

    if result.key_differences:
        print(f"\n{Colors.CYAN}Key Differences:{Colors.END}")
        for diff in result.key_differences:
            print(f"  • {diff}")

    if result.a_better_at:
        print(f"\n{Colors.GREEN}{file_a.name} better at:{Colors.END}")
        for item in result.a_better_at:
            print(f"  ✓ {item}")

    if result.b_better_at:
        print(f"\n{Colors.GREEN}{file_b.name} better at:{Colors.END}")
        for item in result.b_better_at:
            print(f"  ✓ {item}")

    if args.json_output:
        json_path = Path(args.json_output)
        json_path.write_text(json.dumps(result.to_dict(), indent=2))
        print(f"\nResults saved to: {json_path}")

    return 0


async def cmd_benchmark(args: argparse.Namespace) -> int:
    """Run benchmark suite."""
    setup_logging(args.verbose)

    print(f"{Colors.BOLD}RESPO Benchmark Suite{Colors.END}")
    print(f"Model: {args.model_name}")
    if args.reference != "none":
        print(f"Reference: {args.reference}")

    from respo.evaluation import (
        Benchmark,
        BenchmarkSuite,
        VLLMComparator,
        CopilotComparator,
        GPT4Comparator,
        ClaudeComparator,
        run_benchmark,
    )

    # Create model
    model = VLLMComparator(
        name="RESPO",
        base_url=args.model_url,
        model=args.model_name,
    )

    # Create reference if specified
    reference = None
    if args.reference == "copilot":
        reference = CopilotComparator()
    elif args.reference == "gpt4":
        reference = GPT4Comparator()
    elif args.reference == "claude":
        reference = ClaudeComparator()

    # Load or create benchmark
    if args.benchmark_file:
        print(f"Loading benchmark from: {args.benchmark_file}")
        benchmark = Benchmark.from_file(args.benchmark_file)
    else:
        print("Using standard benchmark suite")
        suite = BenchmarkSuite.standard_suite()
        benchmark = suite.benchmarks[0]

    print(f"Tasks: {len(benchmark.tasks)}")
    print(f"\n{'='*50}")

    try:
        result = await benchmark.run(model, reference)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    # Display results
    print(f"\n{Colors.BOLD}Benchmark Results: {result.benchmark_name}{Colors.END}")
    print(f"{'='*50}")

    score_color = Colors.GREEN if result.average_score >= 7 else (Colors.YELLOW if result.average_score >= 5 else Colors.RED)
    print(f"\n{Colors.BOLD}Overall:{Colors.END}")
    print(f"  Average Score: {score_color}{result.average_score:.2f}/10{Colors.END}")
    print(f"  Median Score: {result.median_score:.2f}/10")
    print(f"  Tasks: {result.successful_tasks}/{result.total_tasks} successful")
    print(f"  Time: {result.total_time_ms/1000:.1f}s")

    if result.category_scores:
        print(f"\n{Colors.BOLD}Category Scores:{Colors.END}")
        for cat, score in sorted(result.category_scores.items()):
            cat_color = Colors.GREEN if score >= 7 else (Colors.YELLOW if score >= 5 else Colors.RED)
            print(f"  {cat:20} {cat_color}{score:.2f}/10{Colors.END}")

    if result.comparison_summary:
        print(f"\n{Colors.BOLD}Comparison vs {args.reference}:{Colors.END}")
        cs = result.comparison_summary
        print(f"  Model Wins: {cs['model_wins']}")
        print(f"  Reference Wins: {cs['reference_wins']}")
        print(f"  Ties: {cs['ties']}")
        print(f"  Win Rate: {cs['win_rate']:.0%}")

    if args.output:
        output_path = Path(args.output)
        result.save(str(output_path))
        print(f"\nResults saved to: {output_path}")

    return 0


def cmd_train(args: argparse.Namespace) -> int:
    """Fine-tune model with LoRA."""
    setup_logging(args.verbose)

    print(f"{Colors.BOLD}RESPO LoRA Training{Colors.END}")
    print(f"Base Model: {args.base_model}")
    print(f"Data: {args.data_path}")
    print(f"Output: {args.output}")

    from respo.training import LoRATrainer, TrainingConfig

    # Check if data exists
    data_path = Path(args.data_path)
    if not data_path.exists():
        print(f"Error: Data path not found: {args.data_path}", file=sys.stderr)
        return 1

    # Load or create config
    if args.config:
        config = TrainingConfig.from_yaml(args.config)
    else:
        config = TrainingConfig(
            base_model=args.base_model,
            output_dir=args.output,
            num_epochs=args.epochs,
            batch_size=args.batch_size,
            lora_r=args.lora_r,
            lora_alpha=args.lora_alpha,
        )

    print(f"\n{Colors.BOLD}Training Configuration:{Colors.END}")
    print(f"  Epochs: {config.num_epochs}")
    print(f"  Batch Size: {config.batch_size}")
    print(f"  LoRA Rank: {config.lora_r}")
    print(f"  LoRA Alpha: {config.lora_alpha}")

    trainer = LoRATrainer(config)

    print(f"\n{Colors.CYAN}Starting training...{Colors.END}")

    try:
        result = trainer.train(str(data_path))
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print(f"\n{Colors.GREEN}✓ Training complete{Colors.END}")
    print(f"  Final Loss: {result.final_loss:.4f}")
    print(f"  Training Time: {result.training_time_seconds/60:.1f} minutes")
    print(f"  Output: {result.output_path}")

    return 0


def cmd_ui(args: argparse.Namespace) -> int:
    """Start the web UI dashboard."""
    setup_logging(False)

    print(f"{Colors.BOLD}RESPO Web UI{Colors.END}")
    print(f"Starting on http://{args.host}:{args.port}")

    try:
        from respo.ui import create_app
        app = create_app()
        app.launch(
            server_name=args.host,
            server_port=args.port,
            share=args.share,
        )
    except ImportError:
        print("Error: Gradio not installed. Install with: pip install gradio", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    return 0


def main() -> None:
    """Main CLI entry point."""
    parser = argparse.ArgumentParser(
        prog="respo",
        description="RESPO - RAG-Enhanced Software Programming Optimizer",
    )
    parser.add_argument(
        "--version",
        action="version",
        version="%(prog)s 0.1.0",
    )

    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # Scrape command
    scrape_parser = subparsers.add_parser(
        "scrape",
        help="Scrape a GitHub repository",
    )
    scrape_parser.add_argument(
        "repository",
        help="GitHub repository (URL or owner/repo format)",
    )
    scrape_parser.add_argument(
        "-b", "--branch",
        help="Branch to scrape (default: default branch)",
    )
    scrape_parser.add_argument(
        "-o", "--output",
        help="Output directory for scraped files",
    )
    scrape_parser.add_argument(
        "-j", "--json-output",
        help="Output JSON file for metadata",
    )
    scrape_parser.add_argument(
        "-t", "--token",
        help="GitHub personal access token",
    )
    scrape_parser.add_argument(
        "-e", "--extensions",
        help="File extensions to include (comma-separated, e.g., py,js,ts)",
    )
    scrape_parser.add_argument(
        "--max-file-size",
        type=int,
        default=500,
        help="Maximum file size in KB (default: 500)",
    )
    scrape_parser.add_argument(
        "--depth",
        type=int,
        default=1,
        help="Git clone depth (default: 1 for shallow)",
    )
    scrape_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Search command
    search_parser = subparsers.add_parser(
        "search",
        help="Search for GitHub repositories",
    )
    search_parser.add_argument(
        "query",
        help="Search query",
    )
    search_parser.add_argument(
        "-l", "--language",
        help="Filter by language",
    )
    search_parser.add_argument(
        "-s", "--min-stars",
        type=int,
        default=0,
        help="Minimum stars",
    )
    search_parser.add_argument(
        "-n", "--limit",
        type=int,
        default=20,
        help="Maximum results (default: 20)",
    )
    search_parser.add_argument(
        "-j", "--json-output",
        help="Output JSON file",
    )
    search_parser.add_argument(
        "-t", "--token",
        help="GitHub personal access token",
    )
    search_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Batch scrape command
    batch_parser = subparsers.add_parser(
        "batch-scrape",
        help="Batch scrape repositories from a file",
    )
    batch_parser.add_argument(
        "repos_file",
        help="File with repository URLs (one per line)",
    )
    batch_parser.add_argument(
        "-o", "--output",
        help="Output directory for scraped files",
    )
    batch_parser.add_argument(
        "-j", "--json-output",
        help="Output JSON file for metadata",
    )
    batch_parser.add_argument(
        "-t", "--token",
        help="GitHub personal access token",
    )
    batch_parser.add_argument(
        "--max-file-size",
        type=int,
        default=500,
        help="Maximum file size in KB (default: 500)",
    )
    batch_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Server command
    server_parser = subparsers.add_parser(
        "server",
        help="Start the RESPO API server",
    )
    server_parser.add_argument(
        "-p", "--port",
        type=int,
        default=8080,
        help="Port to listen on (default: 8080)",
    )
    server_parser.add_argument(
        "--host",
        default="0.0.0.0",
        help="Host to bind to (default: 0.0.0.0)",
    )

    # Ingest command
    ingest_parser = subparsers.add_parser(
        "ingest",
        help="Ingest code into the vector store",
    )
    ingest_parser.add_argument(
        "source",
        help="Source to ingest (GitHub repo or directory path)",
    )
    ingest_parser.add_argument(
        "--type",
        choices=["github", "directory", "auto"],
        default="auto",
        help="Source type (default: auto-detect)",
    )
    ingest_parser.add_argument(
        "-t", "--token",
        help="GitHub personal access token (for GitHub sources)",
    )
    ingest_parser.add_argument(
        "-e", "--extensions",
        help="File extensions to include (comma-separated)",
    )
    ingest_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Evaluate command
    eval_parser = subparsers.add_parser(
        "evaluate",
        help="Evaluate code quality using LLM-as-Judge",
    )
    eval_parser.add_argument(
        "code_file",
        nargs="?",
        help="Code file to evaluate (or use --code for inline)",
    )
    eval_parser.add_argument(
        "-c", "--code",
        help="Inline code to evaluate",
    )
    eval_parser.add_argument(
        "-t", "--task",
        default="General code quality assessment",
        help="Task description",
    )
    eval_parser.add_argument(
        "-l", "--language",
        default="python",
        help="Programming language (default: python)",
    )
    eval_parser.add_argument(
        "-m", "--model",
        default="gpt-4",
        help="Judge model (default: gpt-4)",
    )
    eval_parser.add_argument(
        "-j", "--json-output",
        help="Output JSON file",
    )
    eval_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Compare command
    compare_parser = subparsers.add_parser(
        "compare",
        help="Compare two code files using LLM-as-Judge",
    )
    compare_parser.add_argument(
        "file_a",
        help="First code file",
    )
    compare_parser.add_argument(
        "file_b",
        help="Second code file",
    )
    compare_parser.add_argument(
        "-t", "--task",
        default="General code quality comparison",
        help="Task description",
    )
    compare_parser.add_argument(
        "-l", "--language",
        default="python",
        help="Programming language",
    )
    compare_parser.add_argument(
        "-m", "--model",
        default="gpt-4",
        help="Judge model",
    )
    compare_parser.add_argument(
        "-j", "--json-output",
        help="Output JSON file",
    )
    compare_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Benchmark command
    bench_parser = subparsers.add_parser(
        "benchmark",
        help="Run benchmark suite",
    )
    bench_parser.add_argument(
        "-b", "--benchmark-file",
        help="Custom benchmark JSON file",
    )
    bench_parser.add_argument(
        "--standard",
        action="store_true",
        default=True,
        help="Use standard benchmark suite (default)",
    )
    bench_parser.add_argument(
        "--model-url",
        default="http://localhost:8000/v1",
        help="vLLM model URL",
    )
    bench_parser.add_argument(
        "--model-name",
        default="codellama/CodeLlama-13b-Instruct-hf",
        help="Model name",
    )
    bench_parser.add_argument(
        "--reference",
        choices=["copilot", "gpt4", "claude", "none"],
        default="none",
        help="Reference model for comparison",
    )
    bench_parser.add_argument(
        "-o", "--output",
        help="Output file for results",
    )
    bench_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # Train command
    train_parser = subparsers.add_parser(
        "train",
        help="Fine-tune model with LoRA",
    )
    train_parser.add_argument(
        "data_path",
        help="Path to training data",
    )
    train_parser.add_argument(
        "-c", "--config",
        help="Training config YAML file",
    )
    train_parser.add_argument(
        "-m", "--base-model",
        default="codellama/CodeLlama-7b-Instruct-hf",
        help="Base model to fine-tune",
    )
    train_parser.add_argument(
        "-o", "--output",
        default="./output/lora",
        help="Output directory for LoRA weights",
    )
    train_parser.add_argument(
        "--epochs",
        type=int,
        default=3,
        help="Training epochs",
    )
    train_parser.add_argument(
        "--batch-size",
        type=int,
        default=4,
        help="Batch size",
    )
    train_parser.add_argument(
        "--lora-r",
        type=int,
        default=16,
        help="LoRA rank",
    )
    train_parser.add_argument(
        "--lora-alpha",
        type=int,
        default=32,
        help="LoRA alpha",
    )
    train_parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Verbose output",
    )

    # UI command
    ui_parser = subparsers.add_parser(
        "ui",
        help="Start the web UI dashboard",
    )
    ui_parser.add_argument(
        "-p", "--port",
        type=int,
        default=7860,
        help="Port to listen on (default: 7860)",
    )
    ui_parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="Host to bind to (default: 127.0.0.1)",
    )
    ui_parser.add_argument(
        "--share",
        action="store_true",
        help="Create public Gradio share link",
    )

    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        sys.exit(0)

    if args.command == "scrape":
        exit_code = asyncio.run(cmd_scrape(args))
    elif args.command == "search":
        exit_code = asyncio.run(cmd_search(args))
    elif args.command == "batch-scrape":
        exit_code = asyncio.run(cmd_batch_scrape(args))
    elif args.command == "server":
        from respo.api.app import run_server
        run_server()
        exit_code = 0
    elif args.command == "ingest":
        exit_code = asyncio.run(cmd_ingest(args))
    elif args.command == "evaluate":
        exit_code = asyncio.run(cmd_evaluate(args))
    elif args.command == "compare":
        exit_code = asyncio.run(cmd_compare(args))
    elif args.command == "benchmark":
        exit_code = asyncio.run(cmd_benchmark(args))
    elif args.command == "train":
        exit_code = cmd_train(args)
    elif args.command == "ui":
        exit_code = cmd_ui(args)
    else:
        parser.print_help()
        exit_code = 1

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
