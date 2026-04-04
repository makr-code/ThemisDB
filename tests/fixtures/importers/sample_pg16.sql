--
-- PostgreSQL database dump
--
-- Dumped from database version 16.2
-- Dumped by pg_dump version 16.2

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;

--
-- Name: order_status; Type: TYPE
--

CREATE TYPE order_status AS ENUM (
    'pending',
    'processing',
    'shipped',
    'delivered',
    'cancelled'
);

--
-- Name: address; Type: TYPE (composite)
--

CREATE TYPE address AS (
    street character varying(200),
    city character varying(100),
    zip character varying(20),
    country character varying(60)
);

--
-- Name: customers; Type: TABLE
-- Tests: nested parens in column type (varchar(255)), DEFAULT with function call
--        (NOW()), CHECK constraint with nested parens, multi-column UNIQUE
--

CREATE TABLE customers (
    id integer NOT NULL,
    email character varying(255) NOT NULL,
    full_name character varying(100),
    phone character varying(30) DEFAULT NULL,
    score numeric(10,4) DEFAULT 0.0,
    status order_status DEFAULT 'pending',
    metadata jsonb DEFAULT '{}',
    created_at timestamp without time zone DEFAULT NOW(),
    updated_at timestamp without time zone DEFAULT NOW(),
    CONSTRAINT customers_pkey PRIMARY KEY (id),
    CONSTRAINT customers_email_unique UNIQUE (email),
    CONSTRAINT customers_score_check CHECK ((score >= (0)::numeric))
);

--
-- Name: order_items; Type: TABLE
-- Tests: column with type that contains comma in precision: numeric(12,4)
--

CREATE TABLE order_items (
    id bigint NOT NULL,
    order_id integer NOT NULL,
    product_sku character varying(64),
    quantity integer DEFAULT 1,
    unit_price numeric(12,4) NOT NULL,
    discount numeric(5,2) DEFAULT 0.00,
    CONSTRAINT order_items_pkey PRIMARY KEY (id),
    CONSTRAINT order_items_quantity_check CHECK ((quantity > 0)),
    CONSTRAINT order_items_price_check CHECK ((unit_price >= (0)::numeric))
);

--
-- ALTER TABLE: add column after table definition (pg_dump --schema-only variant)
--

ALTER TABLE customers ADD COLUMN loyalty_points integer DEFAULT 0;
ALTER TABLE order_items ADD COLUMN notes text;

--
-- Data section
--

COPY customers (id, email, full_name, phone, score, status, metadata, created_at, updated_at, loyalty_points) FROM stdin;
1	alice@example.com	Alice Wonder	+1-555-0101	9.7500	delivered	{"tier":"gold"}	2025-01-10 08:00:00	2025-11-20 12:34:56	1500
2	bob@example.org	Bob Builder	\N	3.2500	pending	{}	2025-03-15 09:30:00	2025-11-18 10:00:00	0
3	carol@test.io	Carol Smith	+44-7700-900123	7.1250	processing	{"tier":"silver","notes":"vip"}	2025-06-01 14:00:00	2025-11-19 16:45:00	320
\.

COPY order_items (id, order_id, product_sku, quantity, unit_price, discount, notes) FROM stdin;
101	1	SKU-001	2	49.9900	5.00	Fast shipping requested
102	1	SKU-007	1	199.9900	0.00	\N
103	2	SKU-003	5	9.9900	2.50	\N
104	3	SKU-001	3	49.9900	10.00	Gift wrap
\.

--
-- PostgreSQL database dump complete
--
