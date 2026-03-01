import os
import time

import asyncpg
from fastapi import FastAPI
from fastapi.responses import Response

app = FastAPI()

pool = None

DB_HOST = os.getenv("DB_HOST", "bench-postgres")
DB_PORT = int(os.getenv("DB_PORT", "5432"))


@app.on_event("startup")
async def startup():
    global pool
    pool = await asyncpg.create_pool(
        host=DB_HOST,
        port=DB_PORT,
        database="bench",
        user="bench",
        password="bench",
        min_size=5,
        max_size=15,
    )


@app.on_event("shutdown")
async def shutdown():
    global pool
    if pool:
        await pool.close()


PING_RESPONSE = b'{"ok":true,"message":"OK"}'
CONTENT_TYPE = "application/json; charset=utf-8"


@app.get("/api/v2/ping")
async def ping():
    return Response(content=PING_RESPONSE, media_type=CONTENT_TYPE)


@app.get("/api/v2/time")
async def get_time():
    body = f'{{"serverTime":{int(time.time())}}}'
    return Response(content=body.encode(), media_type=CONTENT_TYPE)


@app.get("/api/v2/db/ping")
async def db_ping():
    async with pool.acquire() as conn:
        row = await conn.fetchval(
            "SELECT json_build_object('ok', true, 'message', 'OK')::text"
        )
    return Response(content=row.encode(), media_type=CONTENT_TYPE)


@app.get("/api/v2/db/time")
async def db_time():
    async with pool.acquire() as conn:
        row = await conn.fetchval(
            "SELECT json_build_object('serverTime', extract(epoch from now())::integer)::text"
        )
    return Response(content=row.encode(), media_type=CONTENT_TYPE)
