package com.themisdb.client;

import java.time.Duration;
import java.time.Instant;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Circuit Breaker implementation for fault tolerance
 * 
 * Implements the circuit breaker pattern to prevent cascading failures
 * by temporarily blocking requests when a service is experiencing issues.
 */
public class CircuitBreaker {
    
    /**
     * Circuit breaker states
     */
    public enum State {
        CLOSED,      // Normal operation
        OPEN,        // Blocking requests due to failures
        HALF_OPEN    // Testing if service has recovered
    }
    
    private final AtomicReference<State> state = new AtomicReference<>(State.CLOSED);
    private final AtomicInteger failureCount = new AtomicInteger(0);
    private final AtomicInteger successCount = new AtomicInteger(0);
    private final AtomicReference<Instant> nextAttemptTime = new AtomicReference<>(Instant.now());
    
    private final int failureThreshold;
    private final Duration resetTimeout;
    private final int halfOpenMaxRequests;
    
    /**
     * Create a new circuit breaker
     * 
     * @param failureThreshold Number of failures before opening circuit
     * @param resetTimeout How long to wait before attempting reset
     * @param halfOpenMaxRequests Max requests in half-open state
     */
    public CircuitBreaker(int failureThreshold, Duration resetTimeout, int halfOpenMaxRequests) {
        this.failureThreshold = failureThreshold;
        this.resetTimeout = resetTimeout;
        this.halfOpenMaxRequests = halfOpenMaxRequests;
    }
    
    /**
     * Check if a request can be executed
     * 
     * @return true if request should proceed, false if blocked
     */
    public boolean canExecute() {
        State currentState = state.get();
        
        if (currentState == State.CLOSED) {
            return true;
        }
        
        if (currentState == State.OPEN) {
            if (Instant.now().isAfter(nextAttemptTime.get())) {
                transitionToHalfOpen();
                return true;
            }
            return false;
        }
        
        // HALF_OPEN state
        return successCount.get() < halfOpenMaxRequests;
    }
    
    /**
     * Record a successful request
     */
    public void recordSuccess() {
        State currentState = state.get();
        
        if (currentState == State.HALF_OPEN) {
            int count = successCount.incrementAndGet();
            if (count >= halfOpenMaxRequests) {
                state.set(State.CLOSED);
                failureCount.set(0);
            }
        } else if (currentState == State.CLOSED) {
            failureCount.set(0);
        }
    }
    
    /**
     * Record a failed request
     */
    public void recordFailure() {
        int count = failureCount.incrementAndGet();
        if (count >= failureThreshold) {
            state.set(State.OPEN);
            nextAttemptTime.set(Instant.now().plus(resetTimeout));
        }
    }
    
    /**
     * Transition to half-open state
     */
    private void transitionToHalfOpen() {
        state.set(State.HALF_OPEN);
        successCount.set(0);
    }
    
    /**
     * Get the current circuit breaker state
     * 
     * @return Current state
     */
    public State getState() {
        return state.get();
    }
}
