"""
RESPO Dashboard

Web-based UI for evaluation, benchmarking, and interactive coding assistance.
"""

import asyncio
import json
from typing import Optional

try:
    import gradio as gr
except ImportError:
    raise ImportError("Gradio not installed. Install with: pip install gradio")


def create_app():
    """Create the Gradio application."""
    
    # Theme
    theme = gr.themes.Soft(
        primary_hue="blue",
        secondary_hue="slate",
    )
    
    with gr.Blocks(
        title="RESPO - RAG Coding Assistant",
        theme=theme,
        css="""
        .output-box { font-family: 'Fira Code', monospace; }
        .score-high { color: #22c55e; font-weight: bold; }
        .score-medium { color: #eab308; font-weight: bold; }
        .score-low { color: #ef4444; font-weight: bold; }
        """
    ) as app:
        
        gr.Markdown("""
        # 🤖 RESPO - RAG-Enhanced Software Programming Optimizer
        
        On-premise LLM coding assistant with RAG-based code understanding and generation.
        """)
        
        with gr.Tabs():
            # Chat Tab
            with gr.Tab("💬 Chat"):
                create_chat_tab()
            
            # Evaluate Tab
            with gr.Tab("📊 Evaluate"):
                create_evaluate_tab()
            
            # Compare Tab
            with gr.Tab("⚖️ Compare"):
                create_compare_tab()
            
            # Benchmark Tab
            with gr.Tab("🏆 Benchmark"):
                create_benchmark_tab()
            
            # Search Tab
            with gr.Tab("🔍 Search"):
                create_search_tab()
            
            # Settings Tab
            with gr.Tab("⚙️ Settings"):
                create_settings_tab()
        
        gr.Markdown("""
        ---
        **RESPO** - Open-source, on-premise RAG coding assistant | [Documentation](https://github.com/respo)
        """)
    
    return app


def create_chat_tab():
    """Create the chat interface tab."""
    
    with gr.Row():
        with gr.Column(scale=3):
            chatbot = gr.Chatbot(
                label="Chat",
                height=500,
                show_copy_button=True,
            )
            
            with gr.Row():
                msg = gr.Textbox(
                    label="Message",
                    placeholder="Ask me about code...",
                    scale=4,
                    lines=3,
                )
                send_btn = gr.Button("Send", variant="primary", scale=1)
            
            with gr.Row():
                clear_btn = gr.Button("Clear")
                language = gr.Dropdown(
                    choices=["python", "javascript", "typescript", "go", "rust", "java"],
                    value="python",
                    label="Language",
                    scale=1,
                )
                task = gr.Dropdown(
                    choices=["chat", "implement", "explain", "review", "debug", "complete"],
                    value="chat",
                    label="Task",
                    scale=1,
                )
        
        with gr.Column(scale=1):
            gr.Markdown("### Context")
            context_files = gr.File(
                label="Upload Files for Context",
                file_count="multiple",
                type="filepath",
            )
            
            gr.Markdown("### Options")
            use_rag = gr.Checkbox(label="Use RAG", value=True)
            include_examples = gr.Checkbox(label="Include Examples", value=True)
            max_tokens = gr.Slider(
                minimum=256,
                maximum=4096,
                value=1024,
                step=256,
                label="Max Tokens",
            )
    
    async def respond(message, history, lang, task_type, use_rag_flag, max_tok):
        """Handle chat response."""
        if not message:
            return history
        
        try:
            from respo.config import get_settings
            from respo.llm import VLLMClient
            
            settings = get_settings()
            client = VLLMClient(
                base_url=f"{settings.vllm.url}/v1",
                model=settings.vllm.model,
            )
            
            # Generate response
            response = await client.generate(
                prompt=message,
                max_tokens=max_tok,
                temperature=0.7,
            )
            
            history.append((message, response))
            await client.close()
            
        except Exception as e:
            history.append((message, f"Error: {str(e)}"))
        
        return history
    
    send_btn.click(
        fn=respond,
        inputs=[msg, chatbot, language, task, use_rag, max_tokens],
        outputs=[chatbot],
    )
    
    msg.submit(
        fn=respond,
        inputs=[msg, chatbot, language, task, use_rag, max_tokens],
        outputs=[chatbot],
    )
    
    def clear_chat():
        """Clear the chat history."""
        return []
    
    clear_btn.click(fn=clear_chat, outputs=[chatbot])


def create_evaluate_tab():
    """Create the code evaluation tab."""
    
    with gr.Row():
        with gr.Column():
            code_input = gr.Code(
                label="Code to Evaluate",
                language="python",
                lines=20,
            )
            
            with gr.Row():
                eval_task = gr.Textbox(
                    label="Task Description",
                    placeholder="Describe what this code should do...",
                )
                eval_lang = gr.Dropdown(
                    choices=["python", "javascript", "typescript", "go", "rust", "java"],
                    value="python",
                    label="Language",
                )
            
            evaluate_btn = gr.Button("Evaluate", variant="primary")
        
        with gr.Column():
            score_display = gr.Markdown("### Evaluation Results")
            
            with gr.Row():
                overall_score = gr.Number(label="Overall Score", precision=1)
                grade = gr.Textbox(label="Grade")
            
            summary = gr.Textbox(label="Summary", lines=3)
            
            with gr.Accordion("Criterion Scores", open=False):
                criterion_table = gr.Dataframe(
                    headers=["Criterion", "Score", "Explanation"],
                    label="Detailed Scores",
                )
            
            with gr.Row():
                strengths = gr.Textbox(label="Strengths", lines=3)
                weaknesses = gr.Textbox(label="Weaknesses", lines=3)
            
            improvements = gr.Textbox(label="Suggested Improvements", lines=3)
    
    async def evaluate_code(code, task, lang):
        """Evaluate code quality."""
        if not code:
            return 0, "N/A", "Please enter code to evaluate", [], "", "", ""
        
        try:
            from respo.evaluation import LLMJudge, JudgeConfig
            
            config = JudgeConfig(model="gpt-4")
            judge = LLMJudge(config)
            
            result = await judge.evaluate(
                code=code,
                task_description=task or "General code quality assessment",
                language=lang,
            )
            
            # Format criterion scores
            criterion_data = [
                [cs.criterion.value, f"{cs.score:.1f}", cs.explanation[:100]]
                for cs in result.criterion_scores
            ]
            
            return (
                result.overall_score,
                result.grade,
                result.summary,
                criterion_data,
                "\n".join(f"✓ {s}" for s in result.strengths),
                "\n".join(f"✗ {w}" for w in result.weaknesses),
                "\n".join(f"→ {i}" for i in result.improvements),
            )
        
        except Exception as e:
            return 0, "Error", str(e), [], "", "", ""
    
    evaluate_btn.click(
        fn=evaluate_code,
        inputs=[code_input, eval_task, eval_lang],
        outputs=[overall_score, grade, summary, criterion_table, strengths, weaknesses, improvements],
    )


def create_compare_tab():
    """Create the code comparison tab."""
    
    with gr.Row():
        with gr.Column():
            gr.Markdown("### Code A")
            code_a = gr.Code(
                label="Code A",
                language="python",
                lines=15,
            )
            label_a = gr.Textbox(label="Label", value="Code A")
        
        with gr.Column():
            gr.Markdown("### Code B")
            code_b = gr.Code(
                label="Code B",
                language="python",
                lines=15,
            )
            label_b = gr.Textbox(label="Label", value="Code B")
    
    with gr.Row():
        compare_task = gr.Textbox(
            label="Task Description",
            placeholder="What should this code do?",
            scale=3,
        )
        compare_lang = gr.Dropdown(
            choices=["python", "javascript", "typescript", "go", "rust"],
            value="python",
            label="Language",
            scale=1,
        )
        compare_btn = gr.Button("Compare", variant="primary", scale=1)
    
    with gr.Row():
        with gr.Column():
            winner_display = gr.Markdown("### Winner: -")
            confidence = gr.Slider(
                label="Confidence",
                minimum=0,
                maximum=100,
                value=0,
                interactive=False,
            )
        
        with gr.Column():
            score_a = gr.Number(label="Score A", precision=1)
            score_b = gr.Number(label="Score B", precision=1)
    
    compare_summary = gr.Textbox(label="Summary", lines=3)
    
    with gr.Row():
        a_better = gr.Textbox(label="A Better At", lines=3)
        b_better = gr.Textbox(label="B Better At", lines=3)
    
    async def compare_codes(code_a_val, code_b_val, lbl_a, lbl_b, task, lang):
        """Compare two code samples."""
        if not code_a_val or not code_b_val:
            return "### Winner: N/A", 0, 0, 0, "Please enter both code samples", "", ""
        
        try:
            from respo.evaluation import LLMJudge, JudgeConfig
            
            config = JudgeConfig(model="gpt-4")
            judge = LLMJudge(config)
            
            result = await judge.compare(
                response_a=code_a_val,
                response_b=code_b_val,
                task_description=task or "Code comparison",
                language=lang,
                labels=(lbl_a, lbl_b),
            )
            
            winner_text = f"### Winner: **{lbl_a if result.winner == 'A' else (lbl_b if result.winner == 'B' else 'Tie')}**"
            
            return (
                winner_text,
                result.confidence * 100,
                result.response_a_score,
                result.response_b_score,
                result.summary,
                "\n".join(f"✓ {a}" for a in result.a_better_at),
                "\n".join(f"✓ {b}" for b in result.b_better_at),
            )
        
        except Exception as e:
            return "### Winner: Error", 0, 0, 0, str(e), "", ""
    
    compare_btn.click(
        fn=compare_codes,
        inputs=[code_a, code_b, label_a, label_b, compare_task, compare_lang],
        outputs=[winner_display, confidence, score_a, score_b, compare_summary, a_better, b_better],
    )


def create_benchmark_tab():
    """Create the benchmark tab."""
    
    with gr.Row():
        with gr.Column(scale=1):
            gr.Markdown("### Benchmark Configuration")
            
            benchmark_type = gr.Radio(
                choices=["Standard Suite", "Custom"],
                value="Standard Suite",
                label="Benchmark Type",
            )
            
            custom_file = gr.File(
                label="Custom Benchmark (JSON)",
                visible=False,
            )
            
            model_url = gr.Textbox(
                label="Model URL",
                value="http://localhost:8000/v1",
            )
            
            model_name = gr.Textbox(
                label="Model Name",
                value="codellama/CodeLlama-13b-Instruct-hf",
            )
            
            reference = gr.Dropdown(
                choices=["None", "Copilot", "GPT-4", "Claude"],
                value="None",
                label="Reference Model",
            )
            
            run_btn = gr.Button("Run Benchmark", variant="primary")
        
        with gr.Column(scale=2):
            gr.Markdown("### Results")
            
            progress = gr.Progress()
            status = gr.Textbox(label="Status", value="Ready")
            
            with gr.Row():
                avg_score = gr.Number(label="Average Score", precision=2)
                median_score = gr.Number(label="Median Score", precision=2)
                success_rate = gr.Number(label="Success Rate (%)", precision=1)
            
            category_chart = gr.BarPlot(
                x="category",
                y="score",
                title="Category Scores",
                height=300,
            )
            
            results_table = gr.Dataframe(
                headers=["Task", "Score", "Grade", "Time (ms)"],
                label="Task Results",
            )
            
            export_btn = gr.Button("Export Results")
            export_file = gr.File(label="Download")
    
    def toggle_custom(choice):
        return gr.update(visible=(choice == "Custom"))
    
    benchmark_type.change(fn=toggle_custom, inputs=[benchmark_type], outputs=[custom_file])
    
    async def run_benchmark(bench_type, custom, url, name, ref):
        """Run the benchmark suite."""
        try:
            from respo.evaluation import Benchmark, BenchmarkSuite, VLLMComparator
            
            # Create model
            model = VLLMComparator(name="RESPO", base_url=url, model=name)
            
            # Load benchmark
            if bench_type == "Standard Suite":
                suite = BenchmarkSuite.standard_suite()
                benchmark = suite.benchmarks[0]
            else:
                benchmark = Benchmark.from_file(custom.name)
            
            # Run
            result = await benchmark.run(model)
            
            # Format results
            task_data = [
                [tr.task_id, f"{tr.judge_result.overall_score:.1f}" if tr.judge_result else "N/A",
                 tr.judge_result.grade if tr.judge_result else "N/A",
                 f"{tr.generation_time_ms:.0f}"]
                for tr in result.task_results
            ]
            
            category_data = [
                {"category": cat, "score": score}
                for cat, score in result.category_scores.items()
            ]
            
            success_pct = (result.successful_tasks / result.total_tasks) * 100 if result.total_tasks > 0 else 0
            
            return (
                "Complete",
                result.average_score,
                result.median_score,
                success_pct,
                category_data,
                task_data,
            )
        
        except Exception as e:
            return f"Error: {str(e)}", 0, 0, 0, [], []
    
    run_btn.click(
        fn=run_benchmark,
        inputs=[benchmark_type, custom_file, model_url, model_name, reference],
        outputs=[status, avg_score, median_score, success_rate, category_chart, results_table],
    )


def create_search_tab():
    """Create the code search tab."""
    
    with gr.Row():
        search_query = gr.Textbox(
            label="Search Query",
            placeholder="Search for code...",
            scale=4,
        )
        search_btn = gr.Button("Search", variant="primary", scale=1)
    
    with gr.Row():
        search_lang = gr.Dropdown(
            choices=["all", "python", "javascript", "typescript", "go", "rust"],
            value="all",
            label="Language",
        )
        search_limit = gr.Slider(
            minimum=5,
            maximum=50,
            value=10,
            step=5,
            label="Max Results",
        )
    
    search_results = gr.Dataframe(
        headers=["File", "Language", "Score", "Preview"],
        label="Search Results",
    )
    
    selected_code = gr.Code(
        label="Selected Code",
        language="python",
        lines=15,
    )
    
    async def search_code(query, lang, limit):
        """Search the vector store."""
        if not query:
            return [], ""
        
        try:
            from respo.config import get_settings
            from respo.vectorstore.base import VectorStoreFactory
            from respo.embedding import CodeEmbedder
            
            settings = get_settings()
            
            vector_store = VectorStoreFactory.create(
                settings.vector_store.backend,
                persist_directory=settings.vector_store.chroma_persist_dir,
            )
            
            embedder = CodeEmbedder(
                model_name=settings.embedding.model,
                device=settings.embedding.device,
            )
            
            # Embed query
            query_embedding = await embedder.embed(query)
            
            # Search
            results = await vector_store.search(
                query_embedding=query_embedding,
                limit=int(limit),
                filter_metadata={"language": lang} if lang != "all" else None,
            )
            
            await vector_store.close()
            
            # Format results
            result_data = [
                [r.metadata.get("path", "unknown"), r.metadata.get("language", "?"),
                 f"{r.score:.3f}", r.content[:100] + "..."]
                for r in results
            ]
            
            return result_data, results[0].content if results else ""
        
        except Exception as e:
            return [[str(e), "", "", ""]], ""
    
    search_btn.click(
        fn=search_code,
        inputs=[search_query, search_lang, search_limit],
        outputs=[search_results, selected_code],
    )


def create_settings_tab():
    """Create the settings tab."""
    
    gr.Markdown("### API Configuration")
    
    with gr.Row():
        with gr.Column():
            vllm_url = gr.Textbox(
                label="vLLM URL",
                value="http://localhost:8000",
            )
            vllm_model = gr.Textbox(
                label="vLLM Model",
                value="codellama/CodeLlama-13b-Instruct-hf",
            )
        
        with gr.Column():
            vector_backend = gr.Dropdown(
                choices=["chroma", "qdrant", "weaviate"],
                value="chroma",
                label="Vector Store Backend",
            )
            embed_model = gr.Textbox(
                label="Embedding Model",
                value="microsoft/codebert-base",
            )
    
    gr.Markdown("### LLM-as-Judge")
    
    with gr.Row():
        judge_model = gr.Dropdown(
            choices=["gpt-4", "gpt-4-turbo", "claude-3-opus", "claude-3-sonnet"],
            value="gpt-4",
            label="Judge Model",
        )
        judge_api_key = gr.Textbox(
            label="API Key",
            type="password",
        )
    
    gr.Markdown("### LoRA Training")
    
    with gr.Row():
        lora_r = gr.Slider(
            minimum=4,
            maximum=64,
            value=16,
            step=4,
            label="LoRA Rank",
        )
        lora_alpha = gr.Slider(
            minimum=8,
            maximum=128,
            value=32,
            step=8,
            label="LoRA Alpha",
        )
    
    save_btn = gr.Button("Save Settings", variant="primary")
    status = gr.Textbox(label="Status", value="")
    
    def save_settings(
        vllm_url_val: str,
        vllm_model_val: str,
        vector_backend_val: str,
        embed_model_val: str,
        judge_model_val: str,
        lora_r_val: int,
        lora_alpha_val: int,
    ) -> str:
        """
        Save settings to configuration.
        
        Note: This is a placeholder. Full implementation would persist
        settings to a config file or environment variables.
        """
        # TODO: Implement actual settings persistence
        # For now, just validate and return success
        _ = (vllm_url_val, vllm_model_val, vector_backend_val, 
             embed_model_val, judge_model_val, lora_r_val, lora_alpha_val)
        return "Settings saved successfully (note: persistence not yet implemented)"
    
    save_btn.click(
        fn=save_settings,
        inputs=[vllm_url, vllm_model, vector_backend, embed_model, judge_model, lora_r, lora_alpha],
        outputs=[status],
    )


if __name__ == "__main__":
    app = create_app()
    app.launch()
