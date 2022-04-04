--------------------------------------------------------------------------------
-- http.log --------------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE TABLE http.log (
  id            bigserial PRIMARY KEY,
  datetime      timestamptz DEFAULT clock_timestamp() NOT NULL,
  username      text NOT NULL DEFAULT session_user,
  method        text NOT NULL DEFAULT 'GET' CHECK (method = ANY (ARRAY['GET', 'POST'])),
  path          text NOT NULL,
  headers       jsonb,
  params        jsonb,
  body          jsonb,
  message       text,
  context       text,
  runtime       interval
);

COMMENT ON TABLE http.log IS 'Журнал входящих HTTP запросов.';

COMMENT ON COLUMN http.log.id IS 'Идентификатор';
COMMENT ON COLUMN http.log.datetime IS 'Дата и время';
COMMENT ON COLUMN http.log.username IS 'Пользователь СУБД';
COMMENT ON COLUMN http.log.method IS 'Метод';
COMMENT ON COLUMN http.log.path IS 'Путь';
COMMENT ON COLUMN http.log.headers IS 'Заголовки';
COMMENT ON COLUMN http.log.params IS 'Параметры';
COMMENT ON COLUMN http.log.body IS 'Тело';
COMMENT ON COLUMN http.log.message IS 'Текст основного сообщения исключения';
COMMENT ON COLUMN http.log.context IS 'Строки текста, описывающие стек вызовов в момент исключения';
COMMENT ON COLUMN http.log.runtime IS 'Время выполнения запроса';

CREATE INDEX ON http.log (method);
CREATE INDEX ON http.log (path);

--------------------------------------------------------------------------------
-- http.request ----------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE TABLE http.request (
  id            uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  datetime      timestamptz DEFAULT clock_timestamp() NOT NULL,
  state         integer NOT NULL DEFAULT 0 CHECK (state BETWEEN 0 AND 3),
  method        text NOT NULL DEFAULT 'GET' CHECK (method = ANY (ARRAY['GET', 'POST'])),
  resource      text NOT NULL,
  headers       jsonb,
  content       text,
  done          text,
  fail          text,
  error         text
);

COMMENT ON TABLE http.request IS 'HTTP запрос.';

COMMENT ON COLUMN http.request.id IS 'Идентификатор';
COMMENT ON COLUMN http.request.datetime IS 'Дата и время';
COMMENT ON COLUMN http.request.state IS 'Состояние: 0 - создан, 1 - выполняется, 2 - выполнен, 3 - неудача';
COMMENT ON COLUMN http.request.method IS 'Метод';
COMMENT ON COLUMN http.request.resource IS 'Ресурс';
COMMENT ON COLUMN http.request.headers IS 'Заголовки';
COMMENT ON COLUMN http.request.content IS 'Содержание запроса';
COMMENT ON COLUMN http.request.done IS 'Имя функции обратного вызова в случае успешного ответа';
COMMENT ON COLUMN http.request.fail IS 'Имя функции обратного вызова в случае сбоя';
COMMENT ON COLUMN http.request.error IS 'Текст описания ошибки';

CREATE INDEX ON http.request (state);
CREATE INDEX ON http.request (method);
CREATE INDEX ON http.request (resource);

--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION http.ft_request_after_insert()
RETURNS     trigger
AS $$
BEGIN
  PERFORM pg_notify(TG_TABLE_SCHEMA, row_to_json(NEW)::text);
  RETURN NULL;
END;
$$ LANGUAGE plpgsql
   SECURITY DEFINER
   SET search_path = kernel, pg_temp;

--------------------------------------------------------------------------------

CREATE TRIGGER t_request_after_insert
  AFTER INSERT ON http.request
  FOR EACH ROW
  EXECUTE PROCEDURE http.ft_request_after_insert();

--------------------------------------------------------------------------------
-- http.response ---------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE TABLE http.response (
  id            uuid PRIMARY KEY,
  request       uuid REFERENCES http.request ON DELETE CASCADE,
  datetime      timestamptz DEFAULT clock_timestamp() NOT NULL,
  status        integer NOT NULL,
  status_text   text NOT NULL,
  headers       jsonb NOT NULL,
  content       text,
  runtime       interval
);

COMMENT ON TABLE http.response IS 'HTTP ответ.';

COMMENT ON COLUMN http.response.id IS 'Идентификатор';
COMMENT ON COLUMN http.response.datetime IS 'Дата и время';
COMMENT ON COLUMN http.response.status IS 'Статус ответа на запрос';
COMMENT ON COLUMN http.response.status_text IS 'Статус ответа на запрос';
COMMENT ON COLUMN http.response.headers IS 'Заголовки';
COMMENT ON COLUMN http.response.content IS 'Содержание ответа';
COMMENT ON COLUMN http.response.runtime IS 'Время выполнения запроса';

CREATE INDEX ON http.response (request);
CREATE INDEX ON http.response (status);
