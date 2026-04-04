-- Full minimal pg_dump (selector byte 0x00 – COPY pipeline)
-- PostgreSQL database dump
-- Dumped from database version 15.3

CREATE TABLE public.items (
    id integer NOT NULL,
    name text,
    value numeric(8,2),
    active boolean DEFAULT true
);

COPY public.items (id, name, value, active) FROM stdin;
1	widget	9.99	t
2	gadget	\N	f
3	it''s a test	3.14	t
\.
