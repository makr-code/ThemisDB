# HELP() Function - Unified Documentation Assistant

## Overview

This document describes the new unified `HELP()` function that consolidates all documentation assistant functions into a single intelligent interface.

## Motivation

Previously, users had to choose between 5 different functions:
- `DOCS_QUERY()` - For general questions
- `DOCS_SEARCH()` - For finding documents
- `DOCS_CONFIG_HELP()` - For configuration assistance
- `DOCS_TROUBLESHOOT()` - For troubleshooting errors
- `DOCS_STATS()` - For database statistics

This required users to know which function to use, which added cognitive overhead and made the system less intuitive.

## Solution

The new `HELP()` function automatically determines user intent from the query and routes to the appropriate underlying function. This provides:

1. **Simplicity** - Single function to remember
2. **Intelligence** - Automatic intent detection
3. **Flexibility** - Handles all use cases
4. **Backward Compatibility** - Original functions still available

## How It Works

The `HELP()` function analyzes the query text for keywords to determine intent:

### Configuration Intent
**Triggers**: "config", "configure", "setting", "setup"
**Routes to**: Configuration help with topic extraction
**Example**: `HELP('Configure security settings')` → Configuration guidance

### Troubleshooting Intent
**Triggers**: "error", "fail", "problem", "issue", "hang", "crash", "not work"
**Routes to**: Troubleshooting help
**Example**: `HELP('Server hangs at startup')` → Troubleshooting guidance

### Search Intent
**Triggers**: "search", "find", "look for", "documentation about"
**Routes to**: Document search with formatted results
**Example**: `HELP('Search for RAID documentation')` → List of relevant documents

### General Query (Default)
**Routes to**: RAG-powered query with LLM
**Example**: `HELP('How do I enable sharding?')` → AI-generated answer

## Usage Examples

```sql
-- Configuration
SELECT HELP('Configure security') AS guide;
-- Routes to: getConfigHelp('security')

-- Troubleshooting
SELECT HELP('Server hangs at startup') AS solution;
-- Routes to: getTroubleshootingHelp('Server hangs at startup')

-- Search
SELECT HELP('Search for vector documentation') AS results;
-- Routes to: searchDocs('vector documentation', 5) with formatting

-- General questions (default)
SELECT HELP('How do I enable sharding?') AS answer;
-- Routes to: query('How do I enable sharding?')
```

## Implementation Details

### Intent Detection Algorithm

```cpp
1. Convert query to lowercase
2. Check for configuration keywords → getConfigHelp()
3. Check for troubleshooting keywords → getTroubleshootingHelp()
4. Check for search keywords → searchDocs()
5. Default: query() (RAG-powered general query)
```

### Topic Extraction (for configuration)

When configuration intent is detected, the function extracts the topic:
- "security" → security topic
- "shard" → sharding topic
- "replica" → replication topic
- "cache" → caching topic
- "network" → networking topic
- "storage" → storage topic
- Default: "general" topic

### Search Query Extraction

When search intent is detected, the function extracts the actual search query by removing trigger words:
- "search for X" → "X"
- "find X" → "X"
- "look for X" → "X"

## Backward Compatibility

All original functions remain available for explicit control:
- `DOCS_QUERY()` - Direct RAG query
- `DOCS_SEARCH()` - Direct search
- `DOCS_CONFIG_HELP()` - Direct configuration help
- `DOCS_TROUBLESHOOT()` - Direct troubleshooting
- `DOCS_STATS()` - Database statistics

## Testing

Added comprehensive tests covering:
1. General query routing
2. Configuration intent detection
3. Troubleshooting intent detection
4. Search intent detection
5. Edge cases and error handling

## Future Enhancements

Potential improvements for future versions:
1. Machine learning-based intent classification
2. Multi-language support
3. Context-aware routing (based on conversation history)
4. Confidence scores for intent detection
5. Customizable routing rules

## Migration Guide

### Before (explicit function selection)
```sql
-- User had to choose correct function
SELECT DOCS_CONFIG_HELP('security') AS guide;
SELECT DOCS_TROUBLESHOOT('Server error') AS solution;
SELECT DOCS_QUERY('What is sharding?') AS answer;
```

### After (unified interface)
```sql
-- HELP() automatically routes to appropriate function
SELECT HELP('Configure security') AS guide;
SELECT HELP('Server error') AS solution;
SELECT HELP('What is sharding?') AS answer;
```

Both approaches work, but `HELP()` is simpler and more intuitive.

---

**Date**: 2026-01-11  
**Author**: GitHub Copilot  
**Status**: Implemented
