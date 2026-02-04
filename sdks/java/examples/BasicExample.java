package com.themisdb.examples;

import java.time.Duration;

/**
 * ThemisDB Java SDK - Basic Usage Example
 * 
 * This example demonstrates basic operations with the ThemisDB Java client.
 * Note: This is a placeholder example. Actual implementation pending.
 */
public class BasicExample {
    
    public static void main(String[] args) {
        System.out.println("ThemisDB Java SDK - Basic Example");
        System.out.println("==================================\n");

        try {
            // 1. Initialize the client
            System.out.println("1. Initializing ThemisDB client...");
            ClientConfig config = new ClientConfig(
                "http://localhost:8080",
                "your-jwt-token-here",
                Duration.ofSeconds(30)
            );
            System.out.println("   Connected to: " + config.baseUrl + "\n");

            // 2. Perform a simple query
            System.out.println("2. Executing AQL query...");
            String query = "FOR doc IN myCollection LIMIT 10 RETURN doc";
            System.out.println("   Query: " + query);
            System.out.println("   Result: (Placeholder - will execute when SDK is implemented)\n");

            // 3. LLM inference example
            System.out.println("3. LLM Inference...");
            InferRequest inferRequest = new InferRequest(
                "What is ThemisDB?",
                "mistral-7b",
                100
            );
            System.out.println("   Prompt: \"" + inferRequest.prompt + "\"");
            System.out.println("   Model: " + inferRequest.model);
            System.out.println("   Response: (Placeholder - will execute when SDK is implemented)\n");

            // 4. Check health status
            System.out.println("4. Checking server health...");
            System.out.println("   Status: (Placeholder - will execute when SDK is implemented)\n");

            System.out.println("Example completed! SDK structure in place, implementation pending.");

        } catch (Exception e) {
            System.err.println("Error running example: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }

    // Placeholder classes for demonstration
    static class ClientConfig {
        final String baseUrl;
        final String bearerToken;
        final Duration timeout;

        ClientConfig(String baseUrl, String bearerToken, Duration timeout) {
            this.baseUrl = baseUrl;
            this.bearerToken = bearerToken;
            this.timeout = timeout;
        }
    }

    static class InferRequest {
        final String prompt;
        final String model;
        final int maxTokens;

        InferRequest(String prompt, String model, int maxTokens) {
            this.prompt = prompt;
            this.model = model;
            this.maxTokens = maxTokens;
        }
    }
}
