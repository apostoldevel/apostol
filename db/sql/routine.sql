--------------------------------------------------------------------------------
-- HTTP GET --------------------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * GET запрос.
 * @param {text} patch - Путь
 * @param {jsonb} headers - HTTP заголовки
 * @param {jsonb} params - Параметры запроса
 * @return {SETOF json}
 */
CREATE OR REPLACE FUNCTION http.get (
  patch     text,
  headers   jsonb,
  params    jsonb DEFAULT null
) RETURNS   SETOF json
AS $$
DECLARE
  r         record;
BEGIN
  IF split_part(patch, '/', 3) != 'v1' THEN
    RAISE EXCEPTION 'Invalid API version.';
  END IF;

  FOR r IN SELECT * FROM jsonb_each(headers)
  LOOP
    -- parse headers here
    RAISE NOTICE '%: %', r.key, r.value;
  END LOOP;

  CASE split_part(patch, '/', 4)
  WHEN 'ping' THEN

	RETURN NEXT json_build_object('code', 200, 'message', 'OK');

  WHEN 'time' THEN

	RETURN NEXT json_build_object('serverTime', trunc(extract(EPOCH FROM Now())));

  WHEN 'headers' THEN

	RETURN NEXT coalesce(headers, jsonb_build_object());

  WHEN 'params' THEN

	RETURN NEXT coalesce(params, jsonb_build_object());

  ELSE

    RAISE EXCEPTION 'Patch "%" not found.', patch;

  END CASE;

  RETURN;
END;
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = kernel, pg_temp;

--------------------------------------------------------------------------------
-- HTTP POST -------------------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * POST запрос.
 * @param {text} patch - Путь
 * @param {jsonb} headers - HTTP заголовки
 * @param {jsonb} params - Параметры запроса
 * @param {jsonb} body - Тело запроса
 * @return {SETOF json}
 */
CREATE OR REPLACE FUNCTION http.post (
  patch     text,
  headers   jsonb,
  params    jsonb DEFAULT null,
  body      jsonb DEFAULT null
) RETURNS   SETOF json
AS $$
DECLARE
  r         record;
BEGIN
  IF split_part(patch, '/', 3) != 'v1' THEN
    RAISE EXCEPTION 'Invalid API version.';
  END IF;

  FOR r IN SELECT * FROM jsonb_each(headers)
  LOOP
    -- parse headers here
    RAISE NOTICE '%: %', r.key, r.value;
  END LOOP;

  CASE split_part(patch, '/', 4)
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

    RAISE EXCEPTION 'Patch "%" not found.', patch;

  END CASE;

  RETURN;
END;
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = kernel, pg_temp;
