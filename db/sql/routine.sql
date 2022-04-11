--------------------------------------------------------------------------------
-- HTTP LOG --------------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION http.write_to_log (
  pPath         text,
  pHeaders      jsonb,
  pParams       jsonb DEFAULT null,
  pBody         jsonb DEFAULT null,
  pMethod       text DEFAULT 'GET',
  pMessage      text DEFAULT null,
  pContext      text DEFAULT null
) RETURNS       bigint
AS $$
DECLARE
  nId           bigint;
BEGIN
  INSERT INTO http.log (method, path, headers, params, body, message, context)
  VALUES (pMethod, pPath, pHeaders, pParams, pBody, pMessage, pContext)
  RETURNING id INTO nId;

  RETURN nId;
END;
$$ LANGUAGE plpgsql
   SECURITY DEFINER
   SET search_path = http, pg_temp;

--------------------------------------------------------------------------------
-- HTTP REQUEST ----------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION http.create_request (
  pResource     text,
  pMethod       text DEFAULT 'GET',
  pHeaders      jsonb DEFAULT null,
  pContent      text DEFAULT null,
  pDone         text DEFAULT null,
  pFail         text DEFAULT null
) RETURNS       uuid
AS $$
DECLARE
  uId           uuid;
BEGIN
  INSERT INTO http.request (state, method, resource, headers, content, done, fail)
  VALUES (1, pMethod, pResource, pHeaders, pContent, pDone, pFail)
  RETURNING id INTO uId;

  RETURN uId;
END;
$$ LANGUAGE plpgsql
   SECURITY DEFINER
   SET search_path = http, pg_temp;

--------------------------------------------------------------------------------
-- HTTP RESPONSE ---------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION http.create_response (
  pRequest      uuid,
  pStatus       integer,
  pStatusText   text,
  pHeaders      jsonb,
  pContent      text DEFAULT null
) RETURNS       uuid
AS $$
DECLARE
  uId           uuid;
  cBegin        timestamptz;
BEGIN
  SELECT datetime INTO cBegin FROM http.request WHERE id = pRequest;

  INSERT INTO http.response (id, request, status, status_text, headers, content, runtime)
  VALUES (pRequest, pRequest, pStatus, pStatusText, pHeaders, pContent, age(clock_timestamp(), cBegin))
  RETURNING id INTO uId;

  UPDATE http.request SET state = 2 WHERE id = pRequest;

  RETURN uId;
END;
$$ LANGUAGE plpgsql
   SECURITY DEFINER
   SET search_path = http, pg_temp;

--------------------------------------------------------------------------------
-- HTTP GET --------------------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * Обрабатывает GET запрос.
 * @param {text} path - Путь
 * @param {jsonb} headers - HTTP заголовки
 * @param {jsonb} params - Параметры запроса
 * @return {SETOF json}
 */
CREATE OR REPLACE FUNCTION http.get (
  path      text,
  headers   jsonb,
  params    jsonb DEFAULT null
) RETURNS   SETOF json
AS $$
DECLARE
  r         record;

  nId       bigint;

  cBegin    timestamptz;

  vMessage  text;
  vContext  text;
BEGIN
  nId := http.write_to_log(path, headers, params);

  IF split_part(path, '/', 3) != 'v1' THEN
    RAISE EXCEPTION 'Invalid API version.';
  END IF;

  cBegin := clock_timestamp();

  FOR r IN SELECT * FROM jsonb_each(headers)
  LOOP
    -- parse headers here
  END LOOP;

  CASE split_part(path, '/', 4)
  WHEN 'ping' THEN

	RETURN NEXT json_build_object('code', 200, 'message', 'OK');

  WHEN 'time' THEN

	RETURN NEXT json_build_object('serverTime', trunc(extract(EPOCH FROM Now())));

  WHEN 'headers' THEN

	RETURN NEXT coalesce(headers, jsonb_build_object());

  WHEN 'params' THEN

	RETURN NEXT coalesce(params, jsonb_build_object());

  WHEN 'log' THEN

	FOR r IN SELECT * FROM http.log ORDER BY id DESC
	LOOP
	  RETURN NEXT row_to_json(r);
	END LOOP;

  ELSE

    RAISE EXCEPTION 'Patch "%" not found.', path;

  END CASE;

  UPDATE http.log SET runtime = age(clock_timestamp(), cBegin) WHERE id = nId;

  RETURN;
EXCEPTION
WHEN others THEN
  GET STACKED DIAGNOSTICS vMessage = MESSAGE_TEXT, vContext = PG_EXCEPTION_CONTEXT;

  PERFORM http.write_to_log(path, headers, params, null, 'GET', vMessage, vContext);

  RETURN NEXT json_build_object('error', json_build_object('code', 400, 'message', vMessage));

  RETURN;
END;
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = http, pg_temp;

--------------------------------------------------------------------------------
-- HTTP POST -------------------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * Обрабатывает POST запрос.
 * @param {text} path - Путь
 * @param {jsonb} headers - HTTP заголовки
 * @param {jsonb} params - Параметры запроса
 * @param {jsonb} body - Тело запроса
 * @return {SETOF json}
 */
CREATE OR REPLACE FUNCTION http.post (
  path      text,
  headers   jsonb,
  params    jsonb DEFAULT null,
  body      jsonb DEFAULT null
) RETURNS   SETOF json
AS $$
DECLARE
  r         record;

  nId       bigint;

  cBegin    timestamptz;

  vMessage  text;
  vContext  text;
BEGIN
  nId := http.write_to_log(path, headers, params, body, 'POST');

  IF split_part(path, '/', 3) != 'v1' THEN
    RAISE EXCEPTION 'Invalid API version.';
  END IF;

  FOR r IN SELECT * FROM jsonb_each(headers)
  LOOP
    -- parse headers here
  END LOOP;

  cBegin := clock_timestamp();

  CASE split_part(path, '/', 4)
  WHEN 'ping' THEN

	RETURN NEXT json_build_object('code', 200, 'message', 'OK');

  WHEN 'time' THEN

	RETURN NEXT json_build_object('serverTime', trunc(extract(EPOCH FROM Now())));

  WHEN 'headers' THEN

	RETURN NEXT coalesce(headers, jsonb_build_object());

  WHEN 'params' THEN

	RETURN NEXT coalesce(params, jsonb_build_object());

  WHEN 'body' THEN

	RETURN NEXT coalesce(body, jsonb_build_object());

  ELSE

    RAISE EXCEPTION 'Patch "%" not found.', path;

  END CASE;

  UPDATE http.log SET runtime = age(clock_timestamp(), cBegin) WHERE id = nId;

  RETURN;
EXCEPTION
WHEN others THEN
  GET STACKED DIAGNOSTICS vMessage = MESSAGE_TEXT, vContext = PG_EXCEPTION_CONTEXT;

  PERFORM http.write_to_log(path, headers, params, body, 'POST', vMessage, vContext);

  RETURN NEXT json_build_object('error', json_build_object('code', 400, 'message', vMessage));

  RETURN;
END;
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = http, pg_temp;

--------------------------------------------------------------------------------
-- HTTP FETCH ------------------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * Выполняет HTTP запрос.
 * @param {text} resource - Ресурс
 * @param {text} method - Метод
 * @param {jsonb} headers - HTTP заголовки
 * @param {text} content - Содержание запроса
 * @param {text} done - Имя функции обратного вызова в случае успешного ответа
 * @param {text} fail - Имя функции обратного вызова в случае сбоя
 * @return {uuid}
 */
CREATE OR REPLACE FUNCTION http.fetch (
  resource  text,
  method    text DEFAULT 'GET',
  headers   jsonb DEFAULT null,
  content   text DEFAULT null,
  done      text DEFAULT null,
  fail      text DEFAULT null
) RETURNS   uuid
AS $$
BEGIN
  IF done IS NOT NULL THEN
    PERFORM FROM pg_namespace n JOIN pg_proc p ON n.oid = p.pronamespace WHERE n.nspname = split_part(done, '.', 1) AND p.proname = split_part(done, '.', 2);

    IF NOT FOUND THEN
	  RAISE EXCEPTION 'Not found function: %', done;
    END IF;
  END IF;

  IF fail IS NOT NULL THEN
    PERFORM FROM pg_namespace n JOIN pg_proc p ON n.oid = p.pronamespace WHERE n.nspname = split_part(fail, '.', 1) AND p.proname = split_part(fail, '.', 2);

    IF NOT FOUND THEN
	  RAISE EXCEPTION 'Not found function: %', fail;
    END IF;
  END IF;

  RETURN http.create_request(resource, method, headers, content, done, fail);
END;
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = http, pg_temp;

--------------------------------------------------------------------------------
-- http.done -------------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION http.done (
  pRequest  uuid
) RETURNS   void
AS $$
DECLARE
  r         record;
BEGIN
  SELECT q.method, q.resource, a.status, a.status_text, a.content INTO r
    FROM http.request q INNER JOIN http.response a ON q.id = a.request
   WHERE q.id = pRequest;

  RAISE NOTICE '% % % %', r.method, r.resource, r.status, r.status_text;
END;
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = http, pg_temp;

--------------------------------------------------------------------------------
-- http.fail -------------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION http.fail (
  pRequest  uuid
) RETURNS   void
AS $$
DECLARE
  r         record;
BEGIN
  SELECT method, resource, error INTO r
    FROM http.request
   WHERE id = pRequest;

  RAISE NOTICE 'ERROR: % % %', r.method, r.resource, r.error;
END;
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = http, pg_temp;
