-- selector: parseCreateTable (selector byte 0x02)
-- First byte 0x02 routes to CREATE TABLE DDL parser
id integer NOT NULL,
name character varying(255),
score numeric(10,4) DEFAULT 0.0,
created_at timestamp with time zone DEFAULT NOW(),
tags text[],
meta jsonb,
CONSTRAINT users_pkey PRIMARY KEY (id)
