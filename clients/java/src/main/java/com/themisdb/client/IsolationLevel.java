package com.themisdb.client;

/**
 * Isolation levels for transactions.
 *
 * <p>Listed in increasing strictness order.</p>
 */
public enum IsolationLevel {
    /**
     * Read Committed – transactions see only committed data (default).
     * Non-repeatable reads and phantom reads are possible.
     */
    READ_COMMITTED,

    /**
     * Snapshot – transactions work with a consistent snapshot of the database
     * as of the transaction start time.
     *
     * <p><b>Warning:</b> Write-skew and phantom-read anomalies are possible at
     * SNAPSHOT isolation. Two concurrent SNAPSHOT transactions that each read
     * the same data and write disjoint keys can both commit even when their
     * combined effect violates an application invariant (e.g. double-booking,
     * over-withdrawal). Use {@link #SERIALIZABLE} when strict correctness is
     * required.</p>
     */
    SNAPSHOT,

    /**
     * Serializable – full serializability via Snapshot Isolation plus
     * write-conflict detection (SSI / predicate locking).
     *
     * <p>Prevents write skew and phantom reads. May abort more transactions
     * and has higher latency than {@link #SNAPSHOT}.</p>
     */
    SERIALIZABLE;

    @Override
    public String toString() {
        return name();
    }
}
