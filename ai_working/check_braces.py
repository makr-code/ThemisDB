#!/usr/bin/env python3

import sys
import os

def check_braces(filepath):
    """Check if braces are balanced in a file and report issues."""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception as e:
        return f"ERROR: {e}"
    
    # Simple brace counting (opens - closes)
    open_count = content.count('{')
    close_count = content.count('}')
    
    if open_count == close_count:
        return "BALANCED"
    else:
        delta = open_count - close_count
        if delta > 0:
            return f"MISSING {delta} CLOSING BRACE(S)"
        else:
            return f"MISSING {abs(delta)} OPENING BRACE(S)"

files = [
    "active_vram_allocator.cpp",
    "adapter_registry.cpp",
    "async_inference_engine.cpp",
    "block_table.cpp",
    "ethics_aware_confidence_detector.cpp",
    "gguf_loader.cpp",
    "grafana_metrics.cpp",
    "inference_engine_enhanced.cpp",
    "llama_wrapper.cpp",
    "llm_model_storage.cpp",
    "llm_prefix_cache.cpp",
    "meta_prompt_generator.cpp",
    "model_downloader.cpp",
    "model_loader.cpp",
    "multi_lora_manager.cpp",
    "multi_perspective_generator.cpp",
    "prompt_evaluator.cpp",
    "prompt_optimizer.cpp",
    "streaming_handler.cpp",
    "token_quota_manager.cpp",
]

base_path = "/home/runner/work/ThemisDB/ThemisDB/src/llm/"

for fname in files:
    fpath = os.path.join(base_path, fname)
    if os.path.exists(fpath):
        result = check_braces(fpath)
        print(f"{fname}: {result}")
    else:
        print(f"{fname}: FILE NOT FOUND")
