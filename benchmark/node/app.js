'use strict';

const fastify = require('fastify')({ logger: false });
const { Pool } = require('pg');

const pool = new Pool({
  host: process.env.DB_HOST || 'bench-postgres',
  port: parseInt(process.env.DB_PORT || '5432', 10),
  database: 'bench',
  user: 'bench',
  password: 'bench',
  min: 5,
  max: 15,
});

const PING_BODY = JSON.stringify({ ok: true, message: 'OK' });
const CONTENT_TYPE = 'application/json; charset=utf-8';

fastify.get('/api/v3/ping', async (request, reply) => {
  return reply.type(CONTENT_TYPE).send(PING_BODY);
});

fastify.get('/api/v3/time', async (request, reply) => {
  return reply.type(CONTENT_TYPE).send(`{"serverTime":${Math.floor(Date.now() / 1000)}}`);
});

fastify.get('/api/v3/db/ping', async (request, reply) => {
  const { rows } = await pool.query(
    "SELECT json_build_object('ok', true, 'message', 'OK')::text AS json"
  );
  return reply.type(CONTENT_TYPE).send(rows[0].json);
});

fastify.get('/api/v3/db/time', async (request, reply) => {
  const { rows } = await pool.query(
    "SELECT json_build_object('serverTime', extract(epoch from now())::integer)::text AS json"
  );
  return reply.type(CONTENT_TYPE).send(rows[0].json);
});

const start = async () => {
  try {
    await fastify.listen({ host: '0.0.0.0', port: 3000, backlog: 4096 });
    console.log(`Worker ${process.pid} listening on port 3000`);
  } catch (err) {
    console.error(err);
    process.exit(1);
  }
};

start();
