--------------------------------------------------------------------------------
-- HTTP GET --------------------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * GET запрос.
 * @param {text} pRoute - Маршрут
 * @param {jsonb} pHeader - HTTP заголовок
 * @param {jsonb} pParams - Параметры запроса
 * @return {SETOF json} - Записи в JSON
 */
CREATE OR REPLACE FUNCTION http.get (
  pRoute    text,
  pHeader   jsonb,
  pParams   jsonb
) RETURNS   SETOF json
AS $$
BEGIN
  CASE pRoute
  WHEN '/api/v1/ping' THEN

	RETURN NEXT json_build_object('code', 200, 'message', 'OK');

  WHEN '/api/v1/time' THEN

	RETURN NEXT json_build_object('serverTime', trunc(extract(EPOCH FROM Now())));

  WHEN '/api/v1/header' THEN

	RETURN NEXT pHeader;

  WHEN '/api/v1/params' THEN

	RETURN NEXT pParams;

  ELSE

    RAISE EXCEPTION 'Route "%" not found.', pRoute;

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
 * @param {text} pRoute - Маршрут
 * @param {jsonb} pHeader - HTTP заголовок
 * @param {jsonb} pParams - Параметры запроса
 * @param {jsonb} pBody - Тело запроса
 * @return {SETOF json} - Записи в JSON
 */
CREATE OR REPLACE FUNCTION http.post (
  pRoute    text,
  pHeader   jsonb,
  pParams   jsonb,
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

  CASE pRoute
  WHEN '/api/v1/header' THEN

	RETURN NEXT pHeader;

  WHEN '/api/v1/params' THEN

	RETURN NEXT pParams;

  WHEN '/api/v1/body' THEN

	RETURN NEXT pBody;

  ELSE

    RAISE EXCEPTION 'Route "%" not found.', pRoute;

  END CASE;

  RETURN;
END;
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = kernel, pg_temp;
