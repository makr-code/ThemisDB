```markdown
---
title: Changefeed OpenAPI Snippets
---

Zweck: Minimaler OpenAPI‑Ausschnitt zur Spezifikation der Changefeed‑MVP Endpoints. Diese Snippets sind für die Dokumentation und zum Einfügen in `openapi.yaml` gedacht.

1) GET /changefeed

paths:
  /changefeed:
    get:
      summary: Liste von Changefeed‑Events
      parameters:
        - name: from_seq
          in: query
          schema:
            type: integer
        - name: limit
          in: query
          schema:
            type: integer
            default: 100
        - name: long_poll_ms
          in: query
          schema:
            type: integer
            default: 0
        - name: key_prefix
          in: query
          schema:
            type: string
      responses:
        '200':
          description: Array of change events
          content:
            application/json:
              schema:
                type: object
                properties:
                  events:
                    type: array
                    items:
                      $ref: '#/components/schemas/ChangeEvent'
                  last_seq:
                    type: integer

2) GET /changefeed/stream (SSE)

  /changefeed/stream:
    get:
      summary: Server‑Sent Events stream of changes (at-least-once delivery supported)
      parameters:
        - name: from_seq
          in: query
          schema:
            type: integer
        - name: key_prefix
          in: query
          schema:
            type: string
        - name: consumer_id
          in: query
          description: >
            Opaque consumer identifier for at-least-once delivery tracking.
            When provided, unacknowledged events from previous requests are
            redelivered before new events. Max length: 128 characters.
          schema:
            type: string
        - name: ack_timeout_ms
          in: query
          description: >
            Per-request override for the ACK timeout in milliseconds (default: 30000).
            Events not acknowledged within this window are eligible for redelivery.
          schema:
            type: integer
      responses:
        '200':
          description: text/event-stream (SSE) with id/data lines per event
          content:
            text/event-stream:
              schema:
                type: string

3) POST /changefeed/stream/ack (SSE at-least-once acknowledgement)

  /changefeed/stream/ack:
    post:
      summary: Acknowledge receipt of SSE events for at-least-once delivery
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              required: [consumer_id, up_to_sequence]
              properties:
                consumer_id:
                  type: string
                  description: Same consumer identifier used in GET /changefeed/stream
                up_to_sequence:
                  type: integer
                  format: uint64
                  description: Highest sequence number to acknowledge (inclusive, cumulative)
      responses:
        '200':
          description: Acknowledgement result
          content:
            application/json:
              schema:
                type: object
                properties:
                  consumer_id:
                    type: string
                  up_to_sequence:
                    type: integer
                  acknowledged:
                    type: integer
                    description: Number of in-flight events removed from tracking

4) GET /changefeed/stats

  /changefeed/stats:
    get:
      summary: Changefeed statistics
      responses:
        '200':
          description: Stats
          content:
            application/json:
              schema:
                type: object
                properties:
                  last_seq:
                    type: integer
                  total_events:
                    type: integer

5) POST /changefeed/retention

  /changefeed/retention:
    post:
      summary: Admin: delete events up to seq
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              properties:
                up_to_seq:
                  type: integer
      responses:
        '204':
          description: Deleted

Components:

components:
  schemas:
    ChangeEvent:
      type: object
      properties:
        seq:
          type: integer
        ts:
          type: string
          format: date-time
        key:
          type: string
        op:
          type: string
          description: one of 'insert' | 'update' | 'delete'
        value:
          type: object

Hinweis: Die OpenAPI‑Snippets sind minimal — Werte für Auth/Zugriffssteuerung sollten projektintern ergänzt werden.

```
