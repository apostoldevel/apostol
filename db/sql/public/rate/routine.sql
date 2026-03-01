--------------------------------------------------------------------------------
-- public.update_rate ----------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * Обновляет курс валют.
 * @param {uuid} pCurrencyFrom - Ссылка на первую валюту
 * @param {uuid} pCurrencyTo - Ссылка на вторую валюту
 * @param {numeric} pValue - Значение - соотношение между первой и второй валютой
 * @param {timestamptz} pDateFrom - Дата
 * @return {void}
 */
CREATE OR REPLACE FUNCTION public.update_rate (
  pCurrencyFrom uuid,
  pCurrencyTo   uuid,
  pValue        numeric,
  pDateFrom     timestamptz DEFAULT Now()
) RETURNS       void
AS $$
DECLARE
  dtDateFrom    timestamptz;
  dtDateTo 	    timestamptz;
BEGIN
  -- получим дату значения в текущем диапозоне дат
  SELECT validFromDate, validToDate INTO dtDateFrom, dtDateTo
    FROM public.rate
   WHERE currencyFrom = pCurrencyFrom
     AND currencyTo = pCurrencyTo
     AND validFromDate <= pDateFrom
     AND validToDate > pDateFrom;

  IF FOUND AND pDateFrom = dtDateFrom THEN
    -- обновим значение в текущем диапозоне дат
    UPDATE public.rate SET value = pValue
     WHERE currencyFrom = pCurrencyFrom
       AND currencyTo = pCurrencyTo
       AND validFromDate <= pDateFrom
       AND validToDate > pDateFrom;
  ELSE
    -- обновим дату значения в текущем диапозоне дат
    UPDATE public.rate SET validToDate = pDateFrom
     WHERE currencyFrom = pCurrencyFrom
       AND currencyTo = pCurrencyTo
       AND validFromDate <= pDateFrom
       AND validToDate > pDateFrom;

    INSERT INTO public.rate (currencyfrom, currencyto, value, validfromdate, validToDate)
    VALUES (pCurrencyFrom, pCurrencyTo, pValue, pDateFrom, coalesce(dtDateTo, public.max_date()));
  END IF;
END;
$$ LANGUAGE plpgsql
   SECURITY DEFINER
   SET search_path = public, pg_temp;

--------------------------------------------------------------------------------
-- public.get_rate -------------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * Возвращает курс валют.
 * @param {uuid} pCurrencyFrom - Ссылка на первую валюту
 * @param {uuid} pCurrencyTo - Ссылка на вторую валюту
 * @param {timestamptz} pDateFrom - Дата
* @return {numeric} Значение - соотношение между первой и второй валютой
 */
CREATE OR REPLACE FUNCTION public.get_rate (
  pCurrencyFrom uuid,
  pCurrencyTo   uuid,
  pDateFrom     timestamptz DEFAULT Now()
) RETURNS       numeric
AS $$
  SELECT value
    FROM public.rate
   WHERE currencyFrom = pCurrencyFrom
     AND currencyTo = pCurrencyTo
     AND validFromDate <= pDateFrom
     AND validToDate > pDateFrom
$$ LANGUAGE sql
   SECURITY DEFINER
   SET search_path = public, pg_temp;
