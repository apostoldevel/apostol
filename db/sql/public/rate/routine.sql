--------------------------------------------------------------------------------
-- public.update_rate ----------------------------------------------------------
--------------------------------------------------------------------------------
/**
 * Updates the exchange rate.
 * @param {uuid} pCurrencyFrom - Reference to the first currency
 * @param {uuid} pCurrencyTo - Reference to the second currency
 * @param {numeric} pValue - Value: the ratio between the first and the second currency
 * @param {timestamptz} pDateFrom - Date
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
  -- get the validity dates of the value in effect at the given date
  SELECT validFromDate, validToDate INTO dtDateFrom, dtDateTo
    FROM public.rate
   WHERE currencyFrom = pCurrencyFrom
     AND currencyTo = pCurrencyTo
     AND validFromDate <= pDateFrom
     AND validToDate > pDateFrom;

  IF FOUND AND pDateFrom = dtDateFrom THEN
    -- update the value of the current date range
    UPDATE public.rate SET value = pValue
     WHERE currencyFrom = pCurrencyFrom
       AND currencyTo = pCurrencyTo
       AND validFromDate <= pDateFrom
       AND validToDate > pDateFrom;
  ELSE
    -- update the end date of the current date range
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
 * Returns the exchange rate.
 * @param {uuid} pCurrencyFrom - Reference to the first currency
 * @param {uuid} pCurrencyTo - Reference to the second currency
 * @param {timestamptz} pDateFrom - Date
* @return {numeric} Value: the ratio between the first and the second currency
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
