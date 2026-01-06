package com.themisdb.client.llm;

import com.google.gson.annotations.SerializedName;

/**
 * Represents a message in an LLM conversation
 */
public class LlmMessage {
    private String role;
    private String content;
    @SerializedName("image_url")
    private String imageUrl;

    public LlmMessage() {
    }

    public LlmMessage(String role, String content) {
        this.role = role;
        this.content = content;
    }

    public String getRole() {
        return role;
    }

    public void setRole(String role) {
        this.role = role;
    }

    public String getContent() {
        return content;
    }

    public void setContent(String content) {
        this.content = content;
    }

    public String getImageUrl() {
        return imageUrl;
    }

    public void setImageUrl(String imageUrl) {
        this.imageUrl = imageUrl;
    }
}
