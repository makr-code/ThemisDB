--
-- PostgreSQL database dump – Foreign Key Preservation fixture (v2.0)
--
-- Tests:
--   1. Table-level FOREIGN KEY in CREATE TABLE body (with CONSTRAINT name)
--   2. Inline column-level REFERENCES clause
--   3. Multi-column FOREIGN KEY
--   4. ON DELETE / ON UPDATE actions (CASCADE, SET NULL)
--   5. ALTER TABLE … ADD CONSTRAINT … FOREIGN KEY (deferred FK outside CREATE TABLE)
--   6. Data rows to verify entities carry _foreign_keys metadata
--

SET statement_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;

--
-- Name: users; Type: TABLE
--
CREATE TABLE users (
    id integer NOT NULL,
    username character varying(64) NOT NULL,
    email character varying(255) NOT NULL,
    CONSTRAINT users_pkey PRIMARY KEY (id),
    CONSTRAINT users_email_unique UNIQUE (email)
);

--
-- Name: categories; Type: TABLE
--
CREATE TABLE categories (
    id integer NOT NULL,
    name character varying(128) NOT NULL,
    CONSTRAINT categories_pkey PRIMARY KEY (id)
);

--
-- Name: orders; Type: TABLE
-- Tests: table-level FOREIGN KEY with ON DELETE CASCADE + ON UPDATE RESTRICT
--
CREATE TABLE orders (
    id integer NOT NULL,
    user_id integer NOT NULL,
    status character varying(32) DEFAULT 'pending',
    created_at timestamp without time zone DEFAULT NOW(),
    CONSTRAINT orders_pkey PRIMARY KEY (id),
    CONSTRAINT fk_orders_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE ON UPDATE RESTRICT
);

--
-- Name: order_items; Type: TABLE
-- Tests: multiple FK constraints, ON DELETE SET NULL
--
CREATE TABLE order_items (
    id integer NOT NULL,
    order_id integer NOT NULL,
    category_id integer,
    product_sku character varying(64) NOT NULL,
    quantity integer DEFAULT 1,
    unit_price numeric(12,4) NOT NULL,
    CONSTRAINT order_items_pkey PRIMARY KEY (id),
    CONSTRAINT fk_items_order FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE CASCADE,
    CONSTRAINT fk_items_category FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE SET NULL
);

--
-- Name: tag_assignments; Type: TABLE
-- Tests: multi-column FK (composite key reference)
--
CREATE TABLE tag_assignments (
    order_id integer NOT NULL,
    tag_id integer NOT NULL,
    CONSTRAINT tag_assignments_pkey PRIMARY KEY (order_id, tag_id),
    CONSTRAINT fk_tag_order FOREIGN KEY (order_id, tag_id) REFERENCES orders(id, id)
);

--
-- Name: profiles; Type: TABLE
-- Tests: inline column-level REFERENCES clause (no table-level FOREIGN KEY)
--
CREATE TABLE profiles (
    user_id integer NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    bio text,
    avatar_url text,
    CONSTRAINT profiles_pkey PRIMARY KEY (user_id)
);

--
-- ALTER TABLE: add FK constraint after table definition (deferred FK)
-- Tests: ALTER TABLE … ADD CONSTRAINT … FOREIGN KEY
--
ALTER TABLE ONLY orders ADD CONSTRAINT fk_orders_deferred FOREIGN KEY (user_id) REFERENCES users(id);

--
-- Data section
--

COPY users (id, username, email) FROM stdin;
1	alice	alice@example.com
2	bob	bob@example.org
\.

COPY categories (id, name) FROM stdin;
10	Electronics
20	Clothing
\.

COPY orders (id, user_id, status, created_at) FROM stdin;
100	1	pending	2025-01-10 08:00:00
101	2	shipped	2025-02-14 12:00:00
\.

COPY order_items (id, order_id, category_id, product_sku, quantity, unit_price) FROM stdin;
1001	100	10	SKU-A1	2	49.9900
1002	100	20	SKU-B2	1	29.9900
1003	101	10	SKU-A1	3	49.9900
\.

COPY profiles (user_id, bio, avatar_url) FROM stdin;
1	Alice's bio	https://cdn.example.com/alice.png
2	Bob's bio	\N
\.

--
-- PostgreSQL database dump complete
--
