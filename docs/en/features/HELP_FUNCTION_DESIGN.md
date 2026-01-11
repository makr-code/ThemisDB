# HELP() Function - Unified Documentation Assistant

## Overview

This document describes the new unified `HELP()` function that consolidates all documentation assistant functions into a single intelligent interface with LLM-based intent detection.

## Key Features

1. **LLM-Based Intent Detection** - Uses embedded LLM for intelligent classification (primary method)
2. **Regex Fallback** - Falls back to pattern matching if LLM unavailable
3. **SSE Support** - Compatible with Server-Sent Events for streaming responses
4. **MCP Integration** - Works with Model Context Protocol
5. **User Feedback** - Can incorporate user feedback for continuous improvement

## Motivation

Previously, users had to choose between 5 different functions:
- `DOCS_QUERY()` - For general questions
- `DOCS_SEARCH()` - For finding documents
- `DOCS_CONFIG_HELP()` - For configuration assistance
- `DOCS_TROUBLESHOOT()` - For troubleshooting errors
- `DOCS_STATS()` - For database statistics

This required users to know which function to use, which added cognitive overhead and made the system less intuitive.

## Solution

The new `HELP()` function automatically determines user intent using LLM-based classification and routes to the appropriate underlying function. This provides:

1. **Simplicity** - Single function to remember
2. **Intelligence** - LLM-powered intent detection
3. **Robustness** - Regex fallback when LLM unavailable
4. **Flexibility** - Handles all use cases
5. **Adaptability** - Can learn from user feedback
6. **Backward Compatibility** - Original functions still available

## How It Works

The `HELP()` function uses a two-tier approach for intent detection:

### Primary Method: LLM-Based Classification

When an LLM is available, the function uses it to classify user intent:

1. **Send classification prompt to LLM** with the user's query
2. **LLM analyzes** the semantic meaning and context
3. **Returns intent category**: configuration, troubleshooting, search, or general
4. **Route to appropriate function** based on LLM's decision

**Advantages:**
- Understands semantic meaning and context
- Handles ambiguous or complex queries
- Works across multiple languages
- Can adapt to different phrasings
- More accurate than pattern matching

### Fallback Method: Regex-Based Detection

If LLM is unavailable or fails, falls back to pattern matching:

1. **Convert query to lowercase**
2. **Search for keyword patterns**
3. **Match against predefined rules**
4. **Route based on matched patterns**

### Intent Categories

#### Configuration Intent
**LLM Detection**: Semantic analysis of setup/configuration requests
**Regex Triggers**: "config", "configure", "setting", "setup"
**Routes to**: Configuration help with topic extraction
**Example**: `HELP('Configure security settings')` → Configuration guidance

#### Troubleshooting Intent
**LLM Detection**: Identifies problem descriptions and error reports
**Regex Triggers**: "error", "fail", "problem", "issue", "hang", "crash", "not work"
**Routes to**: Troubleshooting help
**Example**: `HELP('Server hangs at startup')` → Troubleshooting guidance

#### Search Intent
**LLM Detection**: Recognizes information retrieval requests
**Regex Triggers**: "search", "find", "look for", "documentation about"
**Routes to**: Document search with formatted results
**Example**: `HELP('Search for RAID documentation')` → List of relevant documents

#### General Query (Default)
**LLM Detection**: All other queries
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
1. Try LLM-based classification:
   a. Create classification prompt with query
   b. Send to embedded LLM (max 20 tokens)
   c. Parse LLM response (configuration/troubleshooting/search/general)
   d. Validate response
   e. If valid, use LLM classification
   
2. If LLM unavailable or returns "unknown":
   a. Fall back to regex pattern matching
   b. Check configuration keywords
   c. Check troubleshooting keywords
   d. Check search keywords
   e. Default to "general"
   
3. Route based on detected intent:
   - configuration → getConfigHelp()
   - troubleshooting → getTroubleshootingHelp()
   - search → searchDocs() with formatting
   - general → query() (RAG-powered)
```

### LLM Classification Prompt

```
Classify the following user query into exactly ONE category:
- configuration: User wants to configure or set up something
- troubleshooting: User has an error, problem, or issue to solve
- search: User wants to find or search for documentation
- general: General question about ThemisDB

User query: "<user_input>"

Respond with ONLY the category name. No explanation, just the single word.
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

## Integration Considerations

### SSE (Server-Sent Events)

The HELP() function is compatible with SSE for streaming responses:
- LLM-based intent detection can stream classification reasoning
- Final responses can be streamed for better UX
- Compatible with existing SSE infrastructure

### MCP (Model Context Protocol)

Supports MCP for:
- Cross-model intent classification
- Context sharing between LLM calls
- Protocol-level optimizations

### User Feedback

The system can incorporate user feedback:
- Track classification accuracy
- Learn from corrections
- Adjust confidence thresholds
- Improve intent detection over time

**Future Enhancement**: Store feedback in database for training data

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

### Short-term (Next Release)
1. **Enhanced LLM prompts** - Multi-shot examples for better classification
2. **Confidence scoring** - Return confidence level with classification
3. **User feedback collection** - Store user corrections for improvement
4. **Multi-language support** - Extend beyond English/German

### Medium-term
1. **Fine-tuned classification model** - Custom model trained on user queries
2. **Context-aware routing** - Use conversation history for better intent detection
3. **A/B testing framework** - Compare LLM vs regex performance
4. **Telemetry integration** - Track accuracy metrics

### Long-term
1. **Active learning pipeline** - Continuously improve from user feedback
2. **Personalized routing** - Learn user preferences over time
3. **Multi-modal input** - Support images, code snippets in queries
4. **Distributed classification** - Use multiple models for ensemble voting

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
