--
-- PostgreSQL database dump
-- Dumped from database version 15.3
-- pg_dump -F p --no-acl --no-owner -d sample_db
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;

--
-- Name: public; Type: SCHEMA;
--
CREATE SCHEMA public;

--
-- Name: users; Type: TABLE;
--
CREATE TABLE public.users (
    id integer NOT NULL,
    username character varying(64) NOT NULL,
    email text,
    age smallint,
    score double precision,
    balance numeric(10,2),
    is_active boolean DEFAULT true,
    created_at timestamp without time zone,
    profile jsonb,
    ip_address inet,
    CONSTRAINT users_pkey PRIMARY KEY (id)
);

--
-- Name: products; Type: TABLE;
--
CREATE TABLE public.products (
    id bigint NOT NULL,
    name text NOT NULL,
    description text,
    price double precision,
    in_stock boolean,
    tags text[],
    metadata jsonb,
    CONSTRAINT products_pkey PRIMARY KEY (id)
);

--
-- Name: orders; Type: TABLE;
--
CREATE TABLE public.orders (
    id integer NOT NULL,
    user_id integer,
    product_id bigint,
    quantity integer,
    total_price numeric(10,2),
    order_date timestamp without time zone,
    CONSTRAINT orders_pkey PRIMARY KEY (id),
    CONSTRAINT orders_user_id_fkey FOREIGN KEY (user_id) REFERENCES public.users(id)
);

--
-- Data for Name: users; Type: TABLE DATA; Schema: public;
--
COPY public.users (id, username, email, age, score, balance, is_active, created_at, profile, ip_address) FROM stdin;
1	alice	alice@example.com	30	99.5	1234.56	t	2024-01-15 10:00:00	{"bio":"admin"}	192.168.1.1
2	bob	bob@example.com	25	75.0	99.00	t	2024-02-01 08:30:00	{"bio":"user"}	10.0.0.5
3	charlie	\N	40	\N	0.00	f	2023-12-01 00:00:00	\N	\N
\.

--
-- Data for Name: products; Type: TABLE DATA; Schema: public;
--
COPY public.products (id, name, description, price, in_stock, tags, metadata) FROM stdin;
100	Widget A	A small widget	9.99	t	{electronics,small}	{"sku":"WA-001"}
101	Gadget B	A medium gadget	49.99	t	{electronics,medium}	{"sku":"GB-002"}
102	Thing C	\N	\N	f	{}	\N
\.

--
-- Data for Name: orders; Type: TABLE DATA; Schema: public;
--
COPY public.orders (id, user_id, product_id, quantity, total_price, order_date) FROM stdin;
1001	1	100	2	19.98	2024-03-01 14:00:00
1002	2	101	1	49.99	2024-03-02 09:15:00
1003	1	102	5	0.00	2024-03-03 16:30:00
\.

--
-- PostgreSQL database dump complete
--
