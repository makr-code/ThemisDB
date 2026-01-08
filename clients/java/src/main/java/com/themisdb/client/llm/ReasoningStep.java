package com.themisdb.client.llm;

import java.util.List;

/**
 * Represents a reasoning step in LLM interaction
 */
public class ReasoningStep {
    private String type;
    private List<String> content;

    public ReasoningStep() {
    }

    public ReasoningStep(String type, List<String> content) {
        this.type = type;
        this.content = content;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public List<String> getContent() {
        return content;
    }

    public void setContent(List<String> content) {
        this.content = content;
    }
}
