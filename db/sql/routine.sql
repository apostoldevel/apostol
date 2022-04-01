--------------------------------------------------------------------------------
-- HTTP GET --------------------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * GET запрос.
 * @param {text} pPath - Путь
 * @param {jsonb} pHeader - HTTP заголовок
 * @return {SETOF json} - Записи в JSON
 */
CREATE OR REPLACE FUNCTION http.get (
  pPath     text,
  pHeader   jsonb
) RETURNS   SETOF json
AS $$
BEGIN
  CASE lower(pPath)
  WHEN '/api/v1/ping' THEN

	RETURN NEXT json_build_object('code', 200, 'message', 'OK');

  WHEN '/api/v1/time' THEN

	RETURN NEXT json_build_object('serverTime', trunc(extract(EPOCH FROM Now())));

  WHEN '/api/v1/header' THEN

	RETURN NEXT pHeader;

  ELSE

    RAISE EXCEPTION 'Route "%" not found.', pPath;

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
 * @param {text} pPath - Путь
 * @param {jsonb} pHeader - HTTP заголовок
 * @param {jsonb} pBody - Тело запроса
 * @return {SETOF json} - Записи в JSON
 */
CREATE OR REPLACE FUNCTION http.post (
  pPath     text,
  pHeader   jsonb,
  pBody     jsonb
) RETURNS   SETOF json
AS $$
DECLARE
  r         record;
BEGIN
  FOR r IN SELECT * FROM jsonb_to_record(pHeader) AS x("Authorization" text)
  LOOP
    -- parse authorization
  END LOOP;

  CASE lower(pPath)
  WHEN '/api/v1/header' THEN

	RETURN NEXT pHeader;

  WHEN '/api/v1/body' THEN

	RETURN NEXT pBody;

  ELSE

    RAISE EXCEPTION 'Route "%" not found.', pPath;

  END CASE;

  RETURN;
END;
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = kernel, pg_temp;
