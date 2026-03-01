--------------------------------------------------------------------------------
-- public.exchange_rate_done ---------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION public.exchange_rate_done (
  pRequest      uuid
) RETURNS       void
AS $$
DECLARE
  f             record;
  r             record;
  e             record;

  uCurrencyFrom uuid;
  uCurrencyTo   uuid;

  reply         jsonb;

  vMessage      text;
  vContext      text;
BEGIN
  SELECT agent, profile, command, resource, status, status_text, response, message INTO f FROM http.fetch WHERE id = pRequest;

  IF coalesce(f.status, 0) = 200 THEN

    IF f.agent = 'api.exchangerate.host' AND f.command = 'latest' THEN

      reply := convert_from(f.response, 'utf8')::jsonb;

      SELECT * INTO r FROM jsonb_to_record(reply) AS x(success bool, base text, date date, rates jsonb);

      IF NOT r.success THEN
        RETURN;
      END IF;

      SELECT id INTO uCurrencyFrom FROM public.currency WHERE code = r.base;

      IF FOUND THEN
        FOR e IN SELECT * FROM jsonb_each_text(r.rates)
        LOOP
          SELECT id INTO uCurrencyTo FROM public.currency WHERE code = e.key;
          IF FOUND THEN
            PERFORM public.update_rate(uCurrencyFrom, uCurrencyTo, to_number(e.value, '999999990.00999999'));
          END IF;
        END LOOP;
      END IF;
    END IF;

  ELSE

    RAISE NOTICE '% % %', f.status, coalesce(f.response, f.status_text), f.agent;

  END IF;
EXCEPTION
WHEN others THEN
  GET STACKED DIAGNOSTICS vMessage = MESSAGE_TEXT, vContext = PG_EXCEPTION_CONTEXT;
  RAISE NOTICE '% %', vMessage, vContext;
END
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = public, public, pg_temp;

--------------------------------------------------------------------------------
-- public.exchange_rate_fail ---------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION public.exchange_rate_fail (
  pRequest  uuid
) RETURNS   void
AS $$
DECLARE
  r         record;
BEGIN
  SELECT method, resource, agent, error INTO r
    FROM http.request
   WHERE id = pRequest;

  RAISE NOTICE '% % ', r.error, r.agent;
END
$$ LANGUAGE plpgsql
  SECURITY DEFINER
  SET search_path = public, public, pg_temp;
