package com.themisdb.client.llm;

import com.google.gson.annotations.SerializedName;
import java.util.List;
import java.util.Map;

/**
 * Represents a stored LLM interaction
 */
public class LlmInteraction {
    private String id;
    @SerializedName("created_at")
    private String createdAt;
    private String model;
    private List<LlmMessage> messages;
    @SerializedName("reasoning_steps")
    private List<ReasoningStep> reasoningSteps;
    private Map<String, Object> metadata;

    public LlmInteraction() {
    }

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public String getCreatedAt() {
        return createdAt;
    }

    public void setCreatedAt(String createdAt) {
        this.createdAt = createdAt;
    }

    public String getModel() {
        return model;
    }

    public void setModel(String model) {
        this.model = model;
    }

    public List<LlmMessage> getMessages() {
        return messages;
    }

    public void setMessages(List<LlmMessage> messages) {
        this.messages = messages;
    }

    public List<ReasoningStep> getReasoningSteps() {
        return reasoningSteps;
    }

    public void setReasoningSteps(List<ReasoningStep> reasoningSteps) {
        this.reasoningSteps = reasoningSteps;
    }

    public Map<String, Object> getMetadata() {
        return metadata;
    }

    public void setMetadata(Map<String, Object> metadata) {
        this.metadata = metadata;
    }
}
