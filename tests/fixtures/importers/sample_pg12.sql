--
-- PostgreSQL database dump
-- Dumped from database version 12.14
-- pg_dump -F p --no-acl --no-owner -d sample_db
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET xmloption = content;

--
-- Name: public; Type: SCHEMA; Schema: -; Owner: -
--
CREATE SCHEMA public;

COMMENT ON SCHEMA public IS 'standard public schema';

--
-- Name: inventory; Type: TABLE
--
CREATE TABLE public.inventory (
    item_id integer NOT NULL,
    sku character varying(32) NOT NULL,
    quantity integer DEFAULT 0,
    unit_cost numeric(12,4),
    last_updated timestamp without time zone,
    notes text,
    CONSTRAINT inventory_pkey PRIMARY KEY (item_id)
);

--
-- Name: events; Type: TABLE
--
CREATE TABLE public.events (
    event_id bigint NOT NULL,
    event_type character varying(64),
    payload text,
    occurred_at timestamp without time zone,
    CONSTRAINT events_pkey PRIMARY KEY (event_id)
);

--
-- Data for Name: inventory; Type: TABLE DATA; Schema: public; Owner: -
--
COPY public.inventory (item_id, sku, quantity, unit_cost, last_updated, notes) FROM stdin;
1	SKU-001	100	9.9900	2024-01-01 00:00:00	First item
2	SKU-002	0	49.9950	2024-01-02 12:00:00	\N
3	SKU-003	250	1.2500	2024-01-03 08:30:00	Bulk item
\.

--
-- Data for Name: events; Type: TABLE DATA; Schema: public; Owner: -
--
COPY public.events (event_id, event_type, payload, occurred_at) FROM stdin;
1001	user.login	{"user_id":1,"ip":"10.0.0.1"}	2024-02-01 09:00:00
1002	user.logout	{"user_id":1}	2024-02-01 17:00:00
1003	item.update	{"item_id":2,"delta":-5}	2024-02-02 10:15:00
\.

--
-- PostgreSQL database dump complete
--
