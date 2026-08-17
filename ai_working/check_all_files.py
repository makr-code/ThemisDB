#!/usr/bin/env python3

import os
import re

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

def count_braces_simple(content):
    return content.count('{'), content.count('}')

def count_braces_clean(content):
    # Remove raw strings
    clean = re.sub(r'R"\([^)]*\)"', '', content, flags=re.DOTALL)
    # Remove regular strings
    clean = re.sub(r'"[^"]*"', '', clean)
    # Remove character literals
    clean = re.sub(r"'[^']*'", '', clean)
    # Remove // comments
    clean = re.sub(r'//.*$', '', clean, flags=re.MULTILINE)
    # Remove /* */ comments
    clean = re.sub(r'/\*.*?\*/', '', clean, flags=re.DOTALL)
    
    return clean.count('{'), clean.count('}')

print("File                                    Simple Balance  Clean Balance")
print("=" * 70)

for fname in files:
    fpath = os.path.join(base_path, fname)
    if os.path.exists(fpath):
        with open(fpath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        simple_opens, simple_closes = count_braces_simple(content)
        clean_opens, clean_closes = count_braces_clean(content)
        
        simple_balance = simple_opens - simple_closes
        clean_balance = clean_opens - clean_closes
        
        status = "✓" if clean_balance == 0 else "✗"
        print(f"{fname:35s}  {simple_balance:+3d}              {clean_balance:+3d}         {status}")
    else:
        print(f"{fname:35s}  FILE NOT FOUND")
