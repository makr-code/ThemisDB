#!/bin/bash

echo "=== SYNTAX CHECK REPORT ==="
echo ""
echo "1. Checking training_service_registry.h..."
clang++ -std=c++20 -fsyntax-only -I./include include/llm/lora_framework/training_service_registry.h 2>&1
if [ $? -eq 0 ]; then
    echo "✓ training_service_registry.h syntax OK"
else
    echo "✗ training_service_registry.h has syntax errors"
    exit 1
fi

echo ""
echo "2. Checking lora_training_service.h..."
clang++ -std=c++20 -fsyntax-only -I./include include/llm/lora_framework/lora_training_service.h 2>&1
if [ $? -eq 0 ]; then
    echo "✓ lora_training_service.h syntax OK"
else
    echo "✗ lora_training_service.h has syntax errors"
    exit 1
fi

echo ""
echo "3. Checking distributed_training_coordinator.h..."
clang++ -std=c++20 -fsyntax-only -I./include include/llm/distributed_training_coordinator.h 2>&1
if [ $? -eq 0 ]; then
    echo "✓ distributed_training_coordinator.h syntax OK"
else
    echo "✗ distributed_training_coordinator.h has syntax errors"
    exit 1
fi

echo ""
echo "=== MANUAL CODE REVIEW ==="
