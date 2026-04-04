--
-- PostgreSQL database dump
-- Dumped from database version 14.9
-- pg_dump -F p --no-acl --no-owner -d sample_db
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET xmloption = content;
SET default_table_access_method = heap;

--
-- Name: public; Type: SCHEMA
--
CREATE SCHEMA public;

--
-- Name: sensors; Type: TABLE
--
CREATE TABLE public.sensors (
    sensor_id integer NOT NULL,
    name character varying(128) NOT NULL,
    location text,
    active boolean DEFAULT true,
    CONSTRAINT sensors_pkey PRIMARY KEY (sensor_id)
);

--
-- Name: readings; Type: TABLE
--
CREATE TABLE public.readings (
    reading_id bigint NOT NULL,
    sensor_id integer,
    value double precision,
    unit character varying(16),
    recorded_at timestamp with time zone,
    metadata jsonb,
    CONSTRAINT readings_pkey PRIMARY KEY (reading_id),
    CONSTRAINT readings_sensor_id_fkey FOREIGN KEY (sensor_id) REFERENCES public.sensors(sensor_id)
);

--
-- Data for Name: sensors; Type: TABLE DATA; Schema: public; Owner: postgres
--
COPY public.sensors (sensor_id, name, location, active) FROM stdin;
1	Temp-A	Building 1, Room 101	t
2	Pressure-B	Building 2, Roof	t
3	Humidity-C	Basement	f
\.

--
-- Data for Name: readings; Type: TABLE DATA; Schema: public; Owner: postgres
--
COPY public.readings (reading_id, sensor_id, value, unit, recorded_at, metadata) FROM stdin;
5001	1	22.5	C	2024-06-01 12:00:00+00	{"quality":"good"}
5002	2	1013.25	hPa	2024-06-01 12:01:00+00	{"quality":"good"}
5003	3	68.0	%RH	2024-06-01 12:02:00+00	\N
5004	1	\N	C	2024-06-01 12:03:00+00	{"quality":"sensor_error"}
\.

--
-- PostgreSQL database dump complete
--
