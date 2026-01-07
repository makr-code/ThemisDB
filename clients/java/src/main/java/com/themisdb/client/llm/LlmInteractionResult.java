package com.themisdb.client.llm;

/**
 * Result of creating an LLM interaction
 */
public class LlmInteractionResult {
    private String id;
    private boolean success;

    public LlmInteractionResult() {
    }

    public LlmInteractionResult(String id, boolean success) {
        this.id = id;
        this.success = success;
    }

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public boolean isSuccess() {
        return success;
    }

    public void setSuccess(boolean success) {
        this.success = success;
    }
}
