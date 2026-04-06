# themis_help_lora User Guide

**Version:** 1.0  
**Date:** 2026-01-11  
**Status:** Complete

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [How to Use](#how-to-use)
4. [Providing Feedback](#providing-feedback)
5. [Tips for Best Results](#tips-for-best-results)
6. [FAQ](#faq)
7. [Troubleshooting](#troubleshooting)

---

## Introduction

### What is themis_help_lora?

**themis_help_lora** is ThemisDB's intelligent documentation assistant powered by Large Language Models (LLMs) and Low-Rank Adaptation (LoRA) fine-tuning. It helps you find answers to ThemisDB-related questions quickly and accurately.

### Key Features

✅ **Domain-Specific Knowledge**: Trained on ThemisDB documentation  
✅ **Continuous Learning**: Improves from user feedback  
✅ **Accurate Answers**: Reduced hallucinations compared to generic LLMs  
✅ **Fast Responses**: Optimized for documentation queries  
✅ **Context-Aware**: Understands ThemisDB-specific terminology

### What Can It Help With?

- **Configuration**: How to enable sharding, replication, backups
- **Query Language**: AQL syntax, functions, and examples
- **Features**: Explanations of ThemisDB features
- **Troubleshooting**: Common issues and solutions
- **Best Practices**: Recommended approaches for specific tasks
- **API Usage**: How to use ThemisDB APIs

### What It's NOT For

❌ **Bug Fixes**: Cannot fix bugs in your code  
❌ **Custom Development**: Cannot write custom application code  
❌ **Database Administration**: Cannot perform admin operations  
❌ **Real-Time Data**: Cannot query your actual database

---

## Getting Started

### Prerequisites

- Access to a ThemisDB instance (v1.3.5 or later)
- API credentials (JWT token) for authentication
- Network access to the ThemisDB server

### Quick Start (Web UI)

1. **Navigate** to the ThemisDB Admin UI
2. **Click** on "Documentation Assistant" in the sidebar
3. **Type** your question in the input box
4. **Press** Enter or click "Ask"
5. **Review** the answer provided

### Quick Start (REST API)

```bash
# Set your API token
export THEMIS_TOKEN="your_jwt_token_here"

# Ask a question
curl -X POST https://your-themisdb-server.com/api/v1/llm/docs/query \
  -H "Authorization: Bearer $THEMIS_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "question": "How do I enable sharding in ThemisDB?"
  }'
```

### Quick Start (AQL)

```aql
// Query from AQL
RETURN LORA_QUERY(
  "llama-2-7b",
  "themis_help_lora",
  "How do I configure replication?",
  {}
)
```

---

## How to Use

### Asking Questions

#### Good Questions

✅ **Specific and Clear**
```
"How do I enable sharding on a collection?"
"What is the syntax for creating a graph in AQL?"
"How do I configure automatic backups?"
```

✅ **Include Context**
```
"I want to replicate my data across 3 nodes. How do I configure this?"
"I'm getting an error when creating a collection. What could be wrong?"
```

✅ **Request Examples**
```
"Show me an example of creating a sharded collection"
"Give me an AQL query example for finding shortest paths"
```

#### Less Effective Questions

❌ **Too Vague**
```
"How does ThemisDB work?"
"Tell me about databases"
```

❌ **Too Broad**
```
"Explain everything about sharding"
"What are all the features?"
```

❌ **Unrelated to ThemisDB**
```
"How do I write Python code?"
"What's the best database?"
```

### Understanding Answers

#### Answer Structure

Most answers include:

1. **Direct Answer**: The main response to your question
2. **Steps or Examples**: Practical instructions or code
3. **Related Information**: Additional context or related topics
4. **References**: Links to detailed documentation (if available)

#### Example Interaction

**Question:**
```
How do I enable sharding on a collection?
```

**Answer:**
```
To enable sharding in ThemisDB:

1. Configure the shard key in your collection definition
2. Set the number of shards using the `shards` parameter
3. Ensure your cluster has sufficient nodes
4. Monitor shard distribution using the Admin UI

Example:
CREATE COLLECTION mydata SHARD BY user_id SHARDS 8;

This creates a collection sharded by `user_id` across 8 shards.

For more details, see: Sharding Guide in the documentation.
```

### Using Answers

#### Copy-Paste Code

Most code examples are ready to use:

```aql
// Example: Create sharded collection
CREATE COLLECTION users 
  SHARD BY user_id 
  SHARDS 8
  REPLICATION 3;
```

**Before using**:
- Replace example names with your actual names
- Adjust parameters to your needs
- Test in a development environment first

#### Follow-Up Questions

You can ask related follow-up questions:

```
Q: "How do I enable sharding?"
A: [Answer about sharding]

Q: "What's the recommended number of shards?"
A: [Answer about shard count]

Q: "Can I change the shard count later?"
A: [Answer about resharding]
```

---

## Providing Feedback

Your feedback helps improve the assistant!

### When to Give Feedback

✅ **Give Positive Feedback** when:
- The answer was correct and helpful
- The code example worked perfectly
- You found exactly what you needed

✅ **Give Negative Feedback** when:
- The answer was incorrect or misleading
- The code example didn't work
- The answer was incomplete or unclear
- Important information was missing

### How to Give Feedback

#### Web UI

1. **Click** the 👍 (thumbs up) or 👎 (thumbs down) button below the answer
2. If negative, **provide** a correction or explanation in the text box
3. **Click** "Submit Feedback"

#### REST API

**Positive Feedback:**
```bash
curl -X POST https://your-server.com/api/v1/llm/docs/feedback \
  -H "Authorization: Bearer $THEMIS_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "question": "How do I enable sharding?",
    "answer": "To enable sharding...",
    "feedback_type": "positive"
  }'
```

**Negative Feedback with Correction:**
```bash
curl -X POST https://your-server.com/api/v1/llm/docs/feedback \
  -H "Authorization: Bearer $THEMIS_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "question": "How do I enable sharding?",
    "answer": "Incorrect answer here...",
    "feedback_type": "negative",
    "correction": "The correct answer is: CREATE COLLECTION ... SHARD BY ..."
  }'
```

### Impact of Your Feedback

📊 **Continuous Improvement**:
- Feedback is collected and analyzed
- When enough feedback is gathered (typically 100+ items), the system retrains
- The new version incorporates corrections and improvements
- You'll see better answers over time

🎯 **Your Feedback Matters**:
- Corrections help prevent the same mistakes
- Positive feedback reinforces good answers
- Patterns in feedback guide development priorities

---

## Tips for Best Results

### 1. Be Specific

❌ **Vague**: "How do I use collections?"  
✅ **Specific**: "How do I create a collection with 3 replicas?"

### 2. Use ThemisDB Terminology

✅ Use correct terms:
- "Collection" not "table"
- "Document" not "row"
- "AQL" not "SQL"
- "Shard" not "partition"

### 3. Ask One Question at a Time

❌ **Multiple questions**: "How do I enable sharding, configure replication, and set up backups?"  
✅ **Single question**: "How do I enable sharding?"  
(Then ask about replication and backups separately)

### 4. Provide Context When Needed

✅ **Good context**:
- "I'm running ThemisDB v1.3.5..."
- "I have a 3-node cluster..."
- "I'm using the Python client..."

### 5. Request Examples

✅ Add phrases like:
- "Show me an example"
- "Give me a code snippet"
- "What's the syntax for..."

### 6. Check Existing Documentation First

For simple questions, the official documentation might be faster:
- Configuration reference
- AQL syntax guide
- API documentation

Use themis_help_lora for:
- Complex scenarios
- Combining multiple features
- Troubleshooting specific issues
- Understanding best practices

---

## FAQ

### General Questions

**Q: Is themis_help_lora always available?**  
A: Yes, it's available 24/7 as long as your ThemisDB server is running.

**Q: Does it require internet access?**  
A: No, it runs entirely within your ThemisDB instance.

**Q: Can it access my data?**  
A: No, it only provides documentation help. It cannot read or modify your database.

**Q: How accurate is it?**  
A: It's trained on official ThemisDB documentation with continuous improvements from feedback. Accuracy is typically 85%+ and improves over time.

### Usage Questions

**Q: Can I use it programmatically?**  
A: Yes, via REST API or AQL functions. See the [Integration Examples](LORA_INTEGRATION_EXAMPLES.md).

**Q: Is there a rate limit?**  
A: Default limit is 100 queries per minute per user. Contact your administrator if you need more.

**Q: Can I see the query history?**  
A: Yes, in the Web UI under "Documentation Assistant" → "History"

**Q: Can I share answers with my team?**  
A: Yes, use the "Share" button to get a permalink to a specific Q&A.

### Feedback Questions

**Q: How many people need to give feedback before retraining?**  
A: Typically 100+ feedback items trigger a retraining cycle.

**Q: How long does retraining take?**  
A: Usually 15-30 minutes, depending on the amount of feedback.

**Q: Will I be notified of improvements?**  
A: Yes, version updates are announced in the UI and release notes.

**Q: Can I see what feedback others provided?**  
A: No, feedback is anonymous and aggregated for privacy.

### Troubleshooting Questions

**Q: Why is the answer slow?**  
A: 
- High server load
- First query after restart (model loading)
- Complex question requiring more processing

**Q: Why did I get an irrelevant answer?**  
A:
- Question was too vague
- Used non-ThemisDB terminology
- Asked about unsupported topic
→ Try rephrasing more specifically

**Q: The code example didn't work. What should I do?**  
A:
1. Check for typos or missing parameters
2. Verify your ThemisDB version supports the feature
3. Provide negative feedback with details
4. Check official documentation for the feature

---

## Troubleshooting

### Common Issues

#### Issue: "Service Unavailable"

**Cause**: themis_help_lora service is not running  
**Solution**:
1. Check ThemisDB server status
2. Verify LLM components are enabled in configuration
3. Contact your administrator

#### Issue: "Authentication Failed"

**Cause**: Invalid or expired JWT token  
**Solution**:
1. Refresh your JWT token
2. Verify token permissions
3. Check token expiration date

#### Issue: "Timeout Error"

**Cause**: Query taking too long  
**Solution**:
1. Simplify your question
2. Try again (server might be under load)
3. Check network connection

#### Issue: Incorrect or Outdated Answers

**Cause**: Training data may be outdated  
**Solution**:
1. Provide negative feedback with correction
2. Check official documentation for latest info
3. Notify administrators if consistent problem

### Getting Help

If you continue to experience issues:

1. **Check System Status**: Visit the Admin UI status page
2. **Review Logs**: Check your query history for patterns
3. **Contact Support**: Reach out to your ThemisDB administrator
4. **Report Bug**: If you found a bug, file an issue on GitHub

### Best Practices for Problem Solving

1. **Simplify**: Break complex questions into smaller parts
2. **Rephrase**: Try asking the same question differently
3. **Add Context**: Provide version numbers and configuration details
4. **Use Examples**: Show what you tried and what went wrong
5. **Check Documentation**: Official docs are always the authoritative source

---

## Next Steps

### Learn More

- [LoRA Framework Developer Guide](LORA_FRAMEWORK_DEVELOPER_GUIDE.md) - For developers
- [Training Guide](LORA_TRAINING_GUIDE.md) - How the system learns
- [Integration Examples](LORA_INTEGRATION_EXAMPLES.md) - Code examples
- [AQL Reference](../../../LORA_AQL_REFERENCE.md) - Query language functions

### Provide Feedback

Help us improve themis_help_lora:
- Rate answers (👍 / 👎)
- Provide corrections when answers are wrong
- Suggest missing topics
- Report bugs or issues

### Stay Updated

- Check release notes for new versions
- Follow ThemisDB blog for feature announcements
- Join the community forum for discussions

---

**Thank you for using themis_help_lora!**

Your questions and feedback help make this tool better for everyone in the ThemisDB community.

---

**Last Updated**: 2026-04-06  
**Version**: 1.0
